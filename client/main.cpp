#include "Client.hpp"
#include <iostream>

int main() {
    try {
        Client client;
        client.run();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
