#include "Server.hpp"
#include <iostream>
#include <csignal>

Server* server_instance = nullptr;

void signal_handler(int signum) {
    std::cout << "\nShutting down server..." << std::endl;
    if (server_instance) {
        delete server_instance;
        server_instance = nullptr;
    }
    exit(0);
}

int main() {
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    try {
        server_instance = new Server();
        server_instance->run();
        delete server_instance;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        if (server_instance) {
            delete server_instance;
        }
        return 1;
    }
    
    return 0;
}
