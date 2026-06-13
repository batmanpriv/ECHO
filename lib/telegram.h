// telegram.h
#ifndef AUTO_HOP_TELEGRAM_H
#define AUTO_HOP_TELEGRAM_H

#include <string>
#include <random>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>

#pragma comment(lib, "ws2_32.lib")

#define TELEGRAM_BOT_TOKEN "Token Bot"
#define TELEGRAM_CHAT_ID "Chat ID"
#define MIN_PORT 4000
#define MAX_PORT 60000

class AutoHopTelegram {
private:
    std::string bot_token;
    std::string chat_id;
    
    std::string url_encode(const std::string& value) {
        std::string encoded;
        for (char c : value) {
            if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
                encoded += c;
            } else if (c == ' ') {
                encoded += '+';
            } else {
                char hex[4];
                sprintf_s(hex, "%%%02X", (unsigned char)c);
                encoded += hex;
            }
        }
        return encoded;
    }
    
    std::string get_public_ip() {
        std::string ip = "Unable to get public IP";
        
        WSADATA wsaData;
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
            return ip;
        }
        
        SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock == INVALID_SOCKET) {
            WSACleanup();
            return ip;
        }
        
        struct hostent* host = gethostbyname("api.ipify.org");
        if (!host) {
            closesocket(sock);
            WSACleanup();
            return ip;
        }
        
        struct sockaddr_in server_addr;
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(80);
        server_addr.sin_addr = *((struct in_addr*)host->h_addr);
        
        if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
            closesocket(sock);
            WSACleanup();
            return ip;
        }
        
        std::string request = "GET / HTTP/1.1\r\nHost: api.ipify.org\r\nConnection: close\r\n\r\n";
        send(sock, request.c_str(), request.size(), 0);
        
        char buffer[1024];
        std::string response;
        int bytes;
        while ((bytes = recv(sock, buffer, sizeof(buffer) - 1, 0)) > 0) {
            buffer[bytes] = '\0';
            response += buffer;
        }
        
        closesocket(sock);
        WSACleanup();
        
        size_t pos = response.find("\r\n\r\n");
        if (pos != std::string::npos) {
            ip = response.substr(pos + 4);
            while (!ip.empty() && (ip.back() == '\n' || ip.back() == '\r')) {
                ip.pop_back();
            }
        }
        
        return ip;
    }
    
    std::string get_local_ip() {
        std::string local_ip = "127.0.0.1";
        
        WSADATA wsaData;
        WSAStartup(MAKEWORD(2, 2), &wsaData);
        
        char hostname[256];
        if (gethostname(hostname, sizeof(hostname)) == 0) {
            struct hostent* host = gethostbyname(hostname);
            if (host) {
                for (int i = 0; host->h_addr_list[i] != NULL; i++) {
                    struct in_addr addr;
                    memcpy(&addr, host->h_addr_list[i], sizeof(struct in_addr));
                    std::string ip = inet_ntoa(addr);
                    if (ip != "127.0.0.1" && ip != "::1") {
                        local_ip = ip;
                        break;
                    }
                }
            }
        }
        
        WSACleanup();
        return local_ip;
    }
    
    int generate_random_port() {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(MIN_PORT, MAX_PORT);
        return dis(gen);
    }
    
    bool send_telegram_message(const std::string& message) {
        WSADATA wsaData;
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
            return false;
        }
        
        SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock == INVALID_SOCKET) {
            WSACleanup();
            return false;
        }
        
        struct hostent* host = gethostbyname("api.telegram.org");
        if (!host) {
            closesocket(sock);
            WSACleanup();
            return false;
        }
        
        struct sockaddr_in server_addr;
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(443); 
        server_addr.sin_addr = *((struct in_addr*)host->h_addr);
        
        if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
            closesocket(sock);
            WSACleanup();
            return false;
        }
        
        std::string url = "/bot" + bot_token + "/sendMessage";
        std::string post_data = "chat_id=" + chat_id + "&text=" + url_encode(message) + "&parse_mode=Markdown";
        
        std::string request = "POST " + url + " HTTP/1.1\r\n"
                              "Host: api.telegram.org\r\n"
                              "Content-Type: application/x-www-form-urlencoded\r\n"
                              "Content-Length: " + std::to_string(post_data.length()) + "\r\n"
                              "Connection: close\r\n\r\n" +
                              post_data;
        
        int sent = send(sock, request.c_str(), request.size(), 0);
        closesocket(sock);
        WSACleanup();
        
        return sent > 0;
    }
    
public:
    AutoHopTelegram() {
        bot_token = TELEGRAM_BOT_TOKEN;
        chat_id = TELEGRAM_CHAT_ID;
    }
    
    int get_random_port() {
        return generate_random_port();
    }
    
    void send_startup_message(int port) {
        std::string public_ip = get_public_ip();
        std::string local_ip = get_local_ip();
        
        std::string message = "🔔 *Server Started!* 🔔\n\n";
        message += "🌐 *Public IP:* `" + public_ip + "`\n";
        message += "💻 *Local IP:* `" + local_ip + "`\n";
        message += "🔌 *Port:* `" + std::to_string(port) + "`\n";
        message += "📡 *Status:* Listening for connections\n";
        
        send_telegram_message(message);
    }
};

#endif
