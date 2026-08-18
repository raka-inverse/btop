// SPDX-License-Identifier: Apache-2.0

#include "btop.hpp"
#include "web_server.hpp"
#include <iterator>
#include <ranges>
#include <string_view>
#include <vector>
#include <string>
#include <cstdlib>
#include <iostream>

auto main(int argc, const char* argv[]) -> int {
    std::vector<const char*> new_argv;
    bool is_web = false;

    for (int i = 0; i < argc; ++i) {
        if (std::string(argv[i]) == "--web") {
            is_web = true;
        } else {
            new_argv.push_back(argv[i]);
        }
    }

    if (is_web) {
        setenv("BTOP_WEB", "1", 1);
        std::cout << "\n\033[1;32m✅ Aplikasi btop berjalan di browser.\033[0m\n";
        std::cout << "Silakan buka: \033[1;34mhttp://localhost:8081\033[0m\n";
        std::cout << "Tekan Ctrl+C untuk berhenti.\n\n";
        WebServer::start(8081);
    }

    int ret = btop_main(std::views::counted(std::next(new_argv.data()), new_argv.size() - 1) | std::ranges::to<std::vector<std::string_view>>());
    
    if (is_web) WebServer::stop();
    return ret;
}
