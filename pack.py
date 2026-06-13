import base64
import sys
import os

def embed_dll(dll_path, output_header):
    if not os.path.exists(dll_path):
        print(f"[-] DLL not found: {dll_path}")
        return False
    
    with open(dll_path, 'rb') as f:
        dll_data = f.read()
    
    b64_data = base64.b64encode(dll_data).decode('ascii')
    
    header = f"""// Auto-generated DLL embedder
#pragma once
#include <windows.h>
#include <vector>
#include <string>

static const char EMBEDDED_DLL_B64[] = R"RAW({b64_data})RAW";

std::vector<BYTE> GetDLLData() {{
    std::vector<BYTE> result;
    const char* b64chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    
    int val = 0, valb = -8;
    for (size_t i = 0; i < sizeof(EMBEDDED_DLL_B64) - 1; i++) {{
        const char* p = strchr(b64chars, EMBEDDED_DLL_B64[i]);
        if (!p) continue;
        val = (val << 6) + (p - b64chars);
        valb += 6;
        if (valb >= 0) {{
            result.push_back((val >> valb) & 0xFF);
            valb -= 8;
        }}
    }}
    
    return result;
}}
"""
    
    with open(output_header, 'w') as f:
        f.write(header)
    
    print(f"[+] DLL embedded into {output_header} (size: {len(dll_data)} bytes)")
    return True

if __name__ == "__main__":
    if len(sys.argv) < 3:
        print("Usage: python embed_dll.py <dll_file> <output_header>")
        sys.exit(1)
    
    embed_dll(sys.argv[1], sys.argv[2])
