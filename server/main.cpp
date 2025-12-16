#include "Server.hpp"
#include <iostream>
#include <csignal>

Server* server_instance = nullptr;

void signal_handler(int signum) {
    std::cout << "\nShutting down server..." << std::endl;
    delete server_instance;
    exit(0);
}

int main() {
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    try {
        Server server;
        server_instance = &server;
        server.run();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
