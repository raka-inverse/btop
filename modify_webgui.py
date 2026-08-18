import re

with open('webgui.html', 'r') as f:
    content = f.read()

# Add Mem Chart
content = content.replace(
    '</div>\n            </div>\n            \n            <!-- Disks Panel -->',
    '</div>\n                <div id="memChart" style="margin-top:15px; background: #111; border: 1px solid #333; border-radius: 4px; padding: 5px;"></div>\n            </div>\n            \n            <!-- Disks Panel -->'
)

# Add Disk Chart
content = content.replace(
    '</table>\n            </div>\n\n            <!-- Network Panel -->',
    '</table>\n                <div id="diskChart" style="margin-top:15px; background: #111; border: 1px solid #333; border-radius: 4px; padding: 5px;"></div>\n            </div>\n\n            <!-- Network Panel -->'
)

# Add Net Interface Info & modify chart bg
content = content.replace(
    '<h2>Network</h2>',
    '<h2>Network</h2>\n                <div style="font-size: 0.9rem; margin-bottom: 5px; color: #8be9fd;">Interface: {{ net.iface || "N/A" }} ({{ getNetType(net.iface) }})</div>'
)
content = content.replace(
    '<div id="cpuChart"></div>',
    '<div id="cpuChart" style="margin-top:15px; background: #111; border: 1px solid #333; border-radius: 4px; padding: 5px;"></div>'
)
content = content.replace(
    '<div id="netChart" style="margin-top:15px;"></div>',
    '<div id="netChart" style="margin-top:15px; background: #111; border: 1px solid #333; border-radius: 4px; padding: 5px;"></div>'
)

# Vue JS changes
content = content.replace(
    'const net = ref({ download: 0, upload: 0 });',
    'const net = ref({ download: 0, upload: 0, iface: "" });\n\n                const getNetType = (iface) => {\n                    if (!iface) return "Unknown";\n                    const lower = iface.toLowerCase();\n                    if (lower.startsWith("wl") || lower.includes("wifi")) return "WiFi";\n                    if (lower.startsWith("e") || lower.startsWith("en")) return "Ethernet";\n                    if (lower === "lo") return "Loopback";\n                    return "Ethernet";\n                };'
)

content = content.replace(
    'return { hostname, os_name, cpu, mem, disks, net, procs, gpus, filter, sortedProcs, sortBy, sortIndicator, formatBytes, formattedUptime, formattedTime };',
    'return { hostname, os_name, cpu, mem, disks, net, procs, gpus, filter, sortedProcs, sortBy, sortIndicator, formatBytes, formattedUptime, formattedTime, getNetType };'
)

# Data for mem and disk
content = content.replace(
    'let cpuChart, netChart;\n                const timeData = [], cpuData = [], dlData = [], ulData = [];',
    'let cpuChart, netChart, memChart, diskChart;\n                const timeData = [], cpuData = [], dlData = [], ulData = [];\n                const memData = [], swapData = [], diskReadData = [], diskWriteData = [];'
)

# Chart initialization style mimicking GNOME System Monitor
gnome_style_js = """
                const initCharts = () => {
                    const makeOpts = (colors, yRange) => ({
                        width: document.querySelector('.panel').clientWidth - 40, height: 160,
                        legend: { show: false }, cursor: { show: false },
                        axes: [
                            { stroke: "#888", grid: { stroke: "#333", width: 1 }, space: 40 },
                            { stroke: "#888", grid: { stroke: "#333", width: 1 }, space: 30, values: (u, vals, space) => vals.map(v => formatBytes(v)) }
                        ],
                        scales: { x: { time: true }, y: { range: yRange || [0, null] } },
                        series: [
                            {},
                            ...colors.map(color => ({ stroke: color, fill: color + "40", width: 2, paths: uPlot.paths.spline() }))
                        ]
                    });

                    const cpuOpts = makeOpts(["#ff5555"], [0, 100]);
                    cpuOpts.axes[1].values = (u, vals, space) => vals.map(v => v + "%");
                    
                    cpuChart = new uPlot(cpuOpts, [timeData, cpuData], document.getElementById("cpuChart"));
                    memChart = new uPlot(makeOpts(["#bd93f9", "#ffb86c"]), [timeData, memData, swapData], document.getElementById("memChart"));
                    diskChart = new uPlot(makeOpts(["#50fa7b", "#ff5555"]), [timeData, diskReadData, diskWriteData], document.getElementById("diskChart"));
                    netChart = new uPlot(makeOpts(["#50fa7b", "#ff5555"]), [timeData, dlData, ulData], document.getElementById("netChart"));
                };
"""

content = re.sub(r'const initCharts = \(\) => \{.*?\};\n', gnome_style_js, content, flags=re.DOTALL)

# Data pushing
data_push_js = """
                            const t = server_time.value;
                            timeData.push(t); cpuData.push(cpu.value.total);
                            dlData.push(net.value.download); ulData.push(net.value.upload);
                            memData.push(mem.value.used); swapData.push(mem.value.swap_used);
                            
                            let dR = 0, dW = 0;
                            if (disks.value && disks.value.length > 0) {
                                disks.value.forEach(d => { dR += d.io_read; dW += d.io_write; });
                            }
                            diskReadData.push(dR); diskWriteData.push(dW);
                            
                            if (timeData.length > 60) {
                                timeData.shift(); cpuData.shift(); dlData.shift(); ulData.shift();
                                memData.shift(); swapData.shift(); diskReadData.shift(); diskWriteData.shift();
                            }

                            cpuChart.setData([timeData, cpuData]);
                            netChart.setData([timeData, dlData, ulData]);
                            memChart.setData([timeData, memData, swapData]);
                            diskChart.setData([timeData, diskReadData, diskWriteData]);
"""

content = re.sub(r'const t = server_time\.value;.*?netChart\.setData\(\[timeData, dlData, ulData\]\);', data_push_js, content, flags=re.DOTALL)

with open('webgui.html', 'w') as f:
    f.write(content)

