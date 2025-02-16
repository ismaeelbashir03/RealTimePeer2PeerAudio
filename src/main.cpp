#include "VoiceChat.hpp"
#include <iostream>
#include <cstdlib>

int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cerr << "usage: " << argv[0] << " [server|client] [ip] [port]\n";
        return 1;
    }

    bool isServer = std::string(argv[1]) == "server";
    const char* ip = isServer ? "0.0.0.0" : argv[2];
    int port = std::atoi(argv[isServer ? 2 : 3]);

    try {
        VoiceChat chat(ip, port, isServer);
        chat.Start();
        
        std::cout << "voice chat running. Press enter to quit...\n";
        std::cin.get();
        
        chat.Stop();
    } catch (const std::exception& e) {
        std::cerr << "error: " << e.what() << "\n";
        return 1;
    }
    
    return 0;
}