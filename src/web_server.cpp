#include "web_server.hpp"
#include <httplib.h>
#include "json.hpp"
#include <thread>
#include <string>
#include <fstream>
#include "btop_shared.hpp"
#include "btop_tools.hpp"
#include <ctime>

using json = nlohmann::json;

namespace WebServer {
    httplib::Server svr;
    std::thread server_thread;

    const char* html_content = R"====(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <title>Btop Web Dashboard</title>
    <script src="https://unpkg.com/vue@3/dist/vue.global.js"></script>
    <link rel="stylesheet" href="https://unpkg.com/uplot/dist/uPlot.min.css">
    <script src="https://unpkg.com/uplot/dist/uPlot.iife.min.js"></script>
    <style>
        :root { --bg: #1e1e2e; --panel: #282a36; --text: #f8f8f2; --border: #44475a; --accent: #ff79c6; }
        body { background: var(--bg); color: var(--text); font-family: -apple-system, sans-serif; margin: 0; padding: 20px; }
        .grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(450px, 1fr)); gap: 20px; }
        .panel { background: var(--panel); border: 1px solid var(--border); border-radius: 12px; padding: 20px; box-shadow: 0 4px 15px rgba(0,0,0,0.2); }
        .header-panel { background: #282a36; border-bottom: 2px solid var(--accent); padding: 15px 20px; border-radius: 12px; margin-bottom: 20px; display: flex; justify-content: space-between; align-items: center; box-shadow: 0 4px 15px rgba(0,0,0,0.2); }
        .header-panel h1 { margin: 0; font-size: 1.5rem; color: #50fa7b; }
        .header-panel .info { font-size: 0.95rem; color: #f1fa8c; text-align: right; }
        h2 { margin-top: 0; color: var(--accent); font-size: 1.2rem; }
        table { width: 100%; border-collapse: collapse; text-align: left; font-size: 0.9rem; }
        th, td { padding: 8px 4px; border-bottom: 1px solid var(--border); }
        th { cursor: pointer; user-select: none; color: #8be9fd; }
        th:hover { color: #50fa7b; }
        input.search { width: 100%; padding: 10px; margin-bottom: 15px; background: #1e1e2e; color: #fff; border: 1px solid var(--border); border-radius: 6px; box-sizing: border-box; }
        .bar-bg { background: #44475a; height: 8px; border-radius: 4px; overflow: hidden; margin-top: 4px; }
        .bar-fg { height: 100%; transition: width 0.3s; }
        .flex { display: flex; justify-content: space-between; align-items: center; margin-bottom: 5px; }
        .core-grid { display: grid; grid-template-columns: repeat(auto-fill, minmax(140px, 1fr)); gap: 12px; margin-bottom: 15px; }
        .stat-box { font-size: 0.85rem; }
    </style>
</head>
<body>
    <div id="app">
        <!-- HEADER -->
        <div class="header-panel">
            <div>
                <h1>{{ hostname }}</h1>
                <div style="color: #6272a4; margin-top: 5px;">{{ os_name }}</div>
            </div>
            <div class="info">
                <div><strong>Uptime:</strong> {{ formattedUptime }}</div>
                <div style="margin-top: 5px;"><strong>Waktu Server:</strong> {{ formattedTime }}</div>
            </div>
        </div>

        <div class="grid">
            <!-- CPU Panel -->
            <div class="panel">
                <h2>CPU Usage <span style="float:right; font-size:0.9rem; color:#f8f8f2;">{{ cpu.hz }} | {{ cpu.watts }}W</span></h2>
                <h3 style="margin:0 0 10px 0;">Total: {{ cpu.total }}%</h3>
                <div class="core-grid">
                    <div v-for="(c, i) in cpu.cores" :key="i" class="stat-box">
                        <div class="flex"><span>Core {{i}}</span> <span>{{c}}% / {{cpu.temps[i+1] || 0}}°C</span></div>
                        <div class="bar-bg"><div class="bar-fg" :style="{width: c + '%', background: '#ff79c6'}"></div></div>
                    </div>
                </div>
                <div id="cpuChart"></div>
            </div>

            <!-- Memory Panel -->
            <div class="panel">
                <h2>Memory & Swap</h2>
                <div class="stat-box">
                    <div class="flex"><span>RAM Used</span> <span>{{ formatBytes(mem.used) }} / {{ formatBytes(mem.total) }}</span></div>
                    <div class="bar-bg" style="margin-bottom:15px;"><div class="bar-fg" :style="{width: (mem.used / mem.total * 100) + '%', background: '#bd93f9'}"></div></div>
                    
                    <div class="flex"><span>Available: {{ formatBytes(mem.available) }}</span> <span>Cached: {{ formatBytes(mem.cached) }}</span> <span>Free: {{ formatBytes(mem.free) }}</span></div>
                    
                    <div class="flex" style="margin-top:20px;"><span>Swap Used</span> <span>{{ formatBytes(mem.swap_used) }} / {{ formatBytes(mem.swap_total) }}</span></div>
                    <div class="bar-bg"><div class="bar-fg" :style="{width: (mem.swap_used / (mem.swap_total||1) * 100) + '%', background: '#ffb86c'}"></div></div>
                </div>
            </div>
            
            <!-- Disks Panel -->
            <div class="panel">
                <h2>Disks & IO</h2>
                <table>
                    <tr><th>Mount</th><th>Used</th><th>Free</th><th>R/s</th><th>W/s</th></tr>
                    <tr v-for="d in disks">
                        <td>{{d.mount}}<br><small style="color:#6272a4">{{d.name}}</small></td>
                        <td>
                            {{formatBytes(d.used)}} ({{d.used_percent}}%)
                            <div class="bar-bg"><div class="bar-fg" :style="{width: d.used_percent + '%', background: '#f1fa8c'}"></div></div>
                        </td>
                        <td>{{formatBytes(d.free)}}</td>
                        <td>{{formatBytes(d.io_read)}}</td>
                        <td>{{formatBytes(d.io_write)}}</td>
                    </tr>
                </table>
            </div>

            <!-- Network Panel -->
            <div class="panel">
                <h2>Network</h2>
                <div class="flex">
                    <span style="color: #50fa7b;">▼ DL: {{ formatBytes(net.download) }}/s</span>
                    <span style="color: #ff5555;">▲ UL: {{ formatBytes(net.upload) }}/s</span>
                </div>
                <div id="netChart" style="margin-top:15px;"></div>
            </div>
        </div>

        <!-- Processes Panel -->
        <div class="panel" style="margin-top: 20px;">
            <h2>Processes</h2>
            <input class="search" v-model="filter" placeholder="Search by name or PID..." />
            <table>
                <tr>
                    <th @click="sortBy('pid')">PID {{sortIndicator('pid')}}</th>
                    <th @click="sortBy('name')">Name {{sortIndicator('name')}}</th>
                    <th @click="sortBy('user')">User {{sortIndicator('user')}}</th>
                    <th @click="sortBy('mem')">Mem {{sortIndicator('mem')}}</th>
                    <th @click="sortBy('cpu_p')">CPU% {{sortIndicator('cpu_p')}}</th>
                </tr>
                <tr v-for="p in sortedProcs" :key="p.pid">
                    <td>{{ p.pid }}</td>
                    <td>{{ p.name }}</td>
                    <td>{{ p.user }}</td>
                    <td>{{ formatBytes(p.mem) }}</td>
                    <td>
                        {{ p.cpu_p.toFixed(1) }}%
                        <div class="bar-bg" style="width:50px; display:inline-block; margin-left:10px;"><div class="bar-fg" :style="{width: p.cpu_p + '%', background: '#8be9fd'}"></div></div>
                    </td>
                </tr>
            </table>
        </div>
    </div>

    <script>
        const { createApp, ref, computed, onMounted } = Vue;

        createApp({
            setup() {
                const hostname = ref("Loading...");
                const os_name = ref("Loading...");
                const uptime_sec = ref(0);
                const server_time = ref(Date.now() / 1000);

                const cpu = ref({ total: 0, hz: "", watts: 0, cores: [], temps: [] });
                const mem = ref({ total: 1, used: 0, available: 0, cached: 0, free: 0, swap_used: 0, swap_total: 1 });
                const disks = ref([]);
                const net = ref({ download: 0, upload: 0 });
                const procs = ref([]);
                
                const filter = ref("");
                const sortKey = ref("cpu_p");
                const sortAsc = ref(false);

                let cpuChart, netChart;
                const timeData = [], cpuData = [], dlData = [], ulData = [];

                const formatBytes = (bytes) => {
                    if (bytes === 0) return '0 B';
                    const k = 1024, sizes = ['B', 'KB', 'MB', 'GB', 'TB'], i = Math.floor(Math.log(bytes) / Math.log(k));
                    return parseFloat((bytes / Math.pow(k, i)).toFixed(1)) + ' ' + sizes[i];
                };

                const sortIndicator = (key) => sortKey.value === key ? (sortAsc.value ? '▲' : '▼') : '';

                const sortedProcs = computed(() => {
                    let filtered = procs.value.filter(p => p.name.toLowerCase().includes(filter.value.toLowerCase()) || p.pid.toString().includes(filter.value));
                    return filtered.sort((a, b) => {
                        let modifier = sortAsc.value ? 1 : -1;
                        if (a[sortKey.value] < b[sortKey.value]) return -1 * modifier;
                        if (a[sortKey.value] > b[sortKey.value]) return 1 * modifier;
                        return 0;
                    }).slice(0, 50);
                });

                const sortBy = (key) => {
                    if (sortKey.value === key) sortAsc.value = !sortAsc.value;
                    else { sortKey.value = key; sortAsc.value = false; }
                };

                const formattedUptime = computed(() => {
                    let totalSeconds = Math.floor(uptime_sec.value);
                    const d = Math.floor(totalSeconds / 86400); totalSeconds %= 86400;
                    const h = Math.floor(totalSeconds / 3600); totalSeconds %= 3600;
                    const m = Math.floor(totalSeconds / 60);
                    const s = totalSeconds % 60;
                    let out = "";
                    if (d > 0) out += d + " Hari ";
                    out += `${h.toString().padStart(2, '0')}:${m.toString().padStart(2, '0')}:${s.toString().padStart(2, '0')}`;
                    return out;
                });

                const formattedTime = computed(() => {
                    const date = new Date(server_time.value * 1000);
                    const days = ["Minggu", "Senin", "Selasa", "Rabu", "Kamis", "Jumat", "Sabtu"];
                    const months = ["Januari", "Februari", "Maret", "April", "Mei", "Juni", "Juli", "Agustus", "September", "Oktober", "November", "Desember"];
                    
                    const dayName = days[date.getDay()];
                    const day = date.getDate();
                    const month = months[date.getMonth()];
                    const year = date.getFullYear();
                    const h = date.getHours().toString().padStart(2, '0');
                    const m = date.getMinutes().toString().padStart(2, '0');
                    const s = date.getSeconds().toString().padStart(2, '0');

                    return `${dayName}, ${day} ${month} ${year} - ${h}:${m}:${s}`;
                });

                const initCharts = () => {
                    const makeOpts = (color1, color2) => ({
                        width: document.querySelector('.panel').clientWidth - 40, height: 150,
                        legend: { show: false }, // Hide the Time/Value legend
                        axes: [
                            { stroke: "#f8f8f2", grid: { stroke: "#44475a" } },
                            { stroke: "#f8f8f2", grid: { stroke: "#44475a" } }
                        ],
                        scales: { x: { time: true }, y: { range: [0, null] } },
                        series: [
                            {},
                            { stroke: color1, fill: color1 + "33", width: 2, paths: uPlot.paths.spline() },
                            ...(color2 ? [{ stroke: color2, fill: color2 + "33", width: 2, paths: uPlot.paths.spline() }] : [])
                        ]
                    });

                    cpuChart = new uPlot(makeOpts("#ff79c6"), [timeData, cpuData], document.getElementById("cpuChart"));
                    netChart = new uPlot(makeOpts("#50fa7b", "#ff5555"), [timeData, dlData, ulData], document.getElementById("netChart"));
                };

                onMounted(() => {
                    initCharts();
                    
                    setInterval(async () => {
                        try {
                            const res = await fetch('/api/data');
                            const data = await res.json();
                            
                            hostname.value = data.sys.hostname || hostname.value;
                            os_name.value = data.sys.os || os_name.value;
                            uptime_sec.value = data.sys.uptime || uptime_sec.value;
                            server_time.value = data.sys.time || server_time.value;

                            cpu.value = data.cpu || cpu.value;
                            mem.value = data.mem || mem.value;
                            disks.value = data.disks || disks.value;
                            net.value = data.net || net.value;
                            procs.value = data.procs || procs.value;

                            const t = server_time.value;
                            timeData.push(t); cpuData.push(cpu.value.total);
                            dlData.push(net.value.download); ulData.push(net.value.upload);
                            
                            if (timeData.length > 60) {
                                timeData.shift(); cpuData.shift(); dlData.shift(); ulData.shift();
                            }

                            cpuChart.setData([timeData, cpuData]);
                            netChart.setData([timeData, dlData, ulData]);
                        } catch (e) { console.error(e); }
                    }, 1000);
                });

                return { hostname, os_name, cpu, mem, disks, net, filter, sortedProcs, sortBy, sortIndicator, formatBytes, formattedUptime, formattedTime };
            }
        }).mount('#app');
    </script>
</body>
</html>
)====";

    std::string get_os_name() {
        std::ifstream f("/etc/os-release");
        std::string line;
        while (std::getline(f, line)) {
            if (line.rfind("PRETTY_NAME=", 0) == 0) {
                std::string name = line.substr(12);
                if (name.size() > 0 && name[0] == '"') name = name.substr(1, name.size() - 2);
                return name;
            }
        }
        return "Unknown OS";
    }

    void start(int port) {
        svr.Get("/", [](const httplib::Request &, httplib::Response &res) {
            res.set_content(html_content, "text/html");
        });

        svr.Get("/api/data", [](const httplib::Request &, httplib::Response &res) {
            json j;
            try {
                j["sys"]["hostname"] = Tools::hostname();
                j["sys"]["os"] = get_os_name();
                j["sys"]["uptime"] = Tools::system_uptime();
                j["sys"]["time"] = std::time(nullptr);

                auto& cpu = Cpu::collect(true);
                j["cpu"]["total"] = cpu.cpu_percent.count("total") && !cpu.cpu_percent.at("total").empty() ? cpu.cpu_percent.at("total").back() : 0;
                j["cpu"]["hz"] = Cpu::cpuHz;
                j["cpu"]["watts"] = cpu.usage_watts;
                
                auto& cores = j["cpu"]["cores"] = json::array();
                for (size_t i = 0; i < cpu.core_percent.size(); ++i) {
                    cores.push_back(cpu.core_percent[i].empty() ? 0 : cpu.core_percent[i].back());
                }

                auto& temps = j["cpu"]["temps"] = json::array();
                for (size_t i = 0; i < cpu.temp.size(); ++i) {
                    temps.push_back(cpu.temp[i].empty() ? 0 : cpu.temp[i].back());
                }

                auto& mem = Mem::collect(true);
                j["mem"]["total"] = Mem::get_totalMem();
                j["mem"]["used"] = mem.stats.count("used") ? mem.stats.at("used") : 0;
                j["mem"]["available"] = mem.stats.count("available") ? mem.stats.at("available") : 0;
                j["mem"]["cached"] = mem.stats.count("cached") ? mem.stats.at("cached") : 0;
                j["mem"]["free"] = mem.stats.count("free") ? mem.stats.at("free") : 0;

                j["mem"]["swap_total"] = mem.stats.count("swap_total") ? mem.stats.at("swap_total") : 0;
                j["mem"]["swap_used"] = mem.stats.count("swap_used") ? mem.stats.at("swap_used") : 0;
                j["mem"]["swap_free"] = mem.stats.count("swap_free") ? mem.stats.at("swap_free") : 0;

                auto& disks = j["disks"] = json::array();
                for (const auto& disk_name : mem.disks_order) {
                    if (mem.disks.count(disk_name)) {
                        const auto& d = mem.disks.at(disk_name);
                        json dj;
                        dj["name"] = d.name;
                        dj["mount"] = d.stat.string();
                        dj["total"] = d.total;
                        dj["used"] = d.used;
                        dj["free"] = d.free;
                        dj["used_percent"] = d.used_percent;
                        dj["io_read"] = d.io_read.empty() ? 0 : d.io_read.back();
                        dj["io_write"] = d.io_write.empty() ? 0 : d.io_write.back();
                        disks.push_back(dj);
                    }
                }

                auto& net = Net::collect(true);
                j["net"]["download"] = net.bandwidth.count("download") && !net.bandwidth.at("download").empty() ? net.bandwidth.at("download").back() : 0;
                j["net"]["upload"] = net.bandwidth.count("upload") && !net.bandwidth.at("upload").empty() ? net.bandwidth.at("upload").back() : 0;

                auto& procs = Proc::collect(true);
                auto& p_list = j["procs"] = json::array();
                for (const auto& p : procs) {
                    json pj;
                    pj["pid"] = p.pid;
                    pj["name"] = p.name;
                    pj["user"] = p.user;
                    pj["mem"] = p.mem;
                    pj["cpu_p"] = p.cpu_p;
                    p_list.push_back(pj);
                }
            } catch (...) {}

            res.set_content(j.dump(), "application/json");
        });

        server_thread = std::thread([port]() {
            svr.listen("0.0.0.0", port);
        });
    }

    void stop() {
        svr.stop();
        if (server_thread.joinable()) {
            server_thread.join();
        }
    }
}
