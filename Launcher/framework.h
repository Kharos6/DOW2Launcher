// header for storing generic functions and operations, not including functions specific to the game or its files

#pragma once
#pragma comment(lib, "Version.lib")
#pragma comment(lib, "vulkan-1.lib")

#define WIN32_LEAN_AND_MEAN
#define BOOST_DISABLE_CURRENT_LOCATION
#define TIMEOUT_PROCESS 60000 // absolute timeout for the entire process
#define CONSOLE_MESSAGE(msg) \
    if (consoleShown) { \
        std::wcout << msg << std::endl; \
    }

// global includes
#include <windows.h>
#include <tlhelp32.h>
#include <thread>
#include <wincrypt.h>
#include <fstream>
#include <sstream>
#include <iostream>
#include <string>
#include <vector>
#include <regex>
#include <map>
#include <set>
#include <cstring>
#include <codecvt>
#include <locale>
#include <unordered_map>
#include <algorithm>
#include <cctype>
#include <chrono>
#include <cstdlib>
#include <sys/stat.h>
#include <shlobj.h>
#include <cwctype>
#include <iomanip>
#include <psapi.h>
#include <boost/process.hpp>
#include <boost/interprocess/sync/named_mutex.hpp>
#include <boost/interprocess/exceptions.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <boost/locale.hpp>
#include <boost/dll/runtime_symbol_info.hpp>
#include <boost/predef/os.h>

// local includes
#include "vulkan/vulkan.h"

namespace bp = boost::process;



//
//
//
// WINDOWS-EXCLUSIVE FUNCTIONS
//
//
//



// function to display a bitmap while launching
HWND ShowBitmap(HINSTANCE hInstance, const std::wstring& bitmapFileName, int bitmapWidth, int bitmapHeight)
{
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);

    int xPos = (screenWidth - bitmapWidth) / 2;
    int yPos = (screenHeight - bitmapHeight) / 2;

    HWND hwnd = CreateWindowEx(
        0,
        L"STATIC",
        NULL,
        WS_VISIBLE | WS_POPUP | SS_BITMAP,
        xPos, yPos,
        bitmapWidth, bitmapHeight,
        NULL, NULL, hInstance, NULL);

    if (!hwnd)
    {
        return NULL;
    }

    HBITMAP hBitmap = (HBITMAP)LoadImage(NULL, bitmapFileName.c_str(), IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
    if (!hBitmap)
    {
        DestroyWindow(hwnd); // ensure the window is destroyed if bitmap loading fails
        return NULL;
    }

    SendMessage(hwnd, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)hBitmap);

    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);

    return hwnd;
}

// function to get the command line of a process
std::string GetCommandLineOfProcess(DWORD processID) 
{
    std::string commandLine;
    boost::process::ipstream pipe_stream;
    boost::process::child c("wmic process where processid=" + std::to_string(processID) + " get CommandLine", boost::process::std_out > pipe_stream);

    std::string line;
    while (pipe_stream && std::getline(pipe_stream, line) && !line.empty()) 
    {
        if (line.find("CommandLine") == std::string::npos) // skip the header
        {
            commandLine += line;
        }
    }

    c.wait();

    return commandLine;
}

// function to find a process ID
DWORD FindProcessId(const std::wstring& processName) 
{
    PROCESSENTRY32 processInfo = {};
    processInfo.dwSize = sizeof(processInfo);

    HANDLE hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hProcessSnap == INVALID_HANDLE_VALUE) return 0;

    Process32First(hProcessSnap, &processInfo);
    if (!processName.compare(processInfo.szExeFile)) 
    {
        CloseHandle(hProcessSnap);
        return processInfo.th32ProcessID;
    }

    while (Process32Next(hProcessSnap, &processInfo)) 
    {
        if (!processName.compare(processInfo.szExeFile)) 
        {
            CloseHandle(hProcessSnap);
            return processInfo.th32ProcessID;
        }
    }

    CloseHandle(hProcessSnap);
    return 0;
}

// function to strip the executable path from a string
std::string StripExecutablePath(const std::string& commandLine) 
{
    size_t pos = commandLine.find_last_of("\\/");
    std::string fileName;
    if (pos != std::string::npos) 
    {
        fileName = commandLine.substr(pos + 1);
    }
    else 
    {
        fileName = commandLine;
    }

    // remove all quotation marks from the resulting string
    fileName.erase(std::remove(fileName.begin(), fileName.end(), '"'), fileName.end());

    return fileName;
}

// function to normalize a command line
std::string NormalizeCommandLine(const std::string& commandLine)
{
    std::string normalized = commandLine;
    // convert to lower case
    std::transform(normalized.begin(), normalized.end(), normalized.begin(), ::tolower);
    // remove extra spaces
    normalized.erase(std::unique(normalized.begin(), normalized.end(), [](char a, char b) { return std::isspace(a) && std::isspace(b); }), normalized.end());
    return normalized;
}

// function to calculate the MD5 checksum of a file
bool CalculateMD5(const wchar_t* filepath, std::string& md5String)
{
    HCRYPTPROV hProv = 0;
    HCRYPTHASH hHash = 0;
    HANDLE hFile = CreateFile(filepath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
    {
        return false;
    }

    if (!CryptAcquireContext(&hProv, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT))
    {
        CloseHandle(hFile);
        return false;
    }

    if (!CryptCreateHash(hProv, CALG_MD5, 0, 0, &hHash))
    {
        CloseHandle(hFile);
        CryptReleaseContext(hProv, 0);
        return false;
    }

    BYTE buffer[4096] = { 0 };
    DWORD bytesRead = 0;
    while (ReadFile(hFile, buffer, sizeof(buffer), &bytesRead, NULL) && bytesRead != 0)
    {
        if (!CryptHashData(hHash, buffer, bytesRead, 0))
        {
            CryptDestroyHash(hHash);
            CryptReleaseContext(hProv, 0);
            CloseHandle(hFile);
            return false;
        }
    }

    BYTE rgbHash[16];
    DWORD cbHash = 16;
    if (CryptGetHashParam(hHash, HP_HASHVAL, rgbHash, &cbHash, 0))
    {
        char hexStr[33] = { 0 };
        for (DWORD i = 0; i < cbHash; i++)
        {
            sprintf(&hexStr[i * 2], "%02x", rgbHash[i]);
        }
        hexStr[32] = 0;
        md5String = hexStr;
    }

    CryptDestroyHash(hHash);
    CryptReleaseContext(hProv, 0);
    CloseHandle(hFile);

    return true;
}

// function to check CPU core count
int GetProcessorCoreCount() 
{
    SYSTEM_INFO sysinfo;
    GetSystemInfo(&sysinfo);
    return sysinfo.dwNumberOfProcessors;
}

// function to check if a process is running
bool IsProcessRunning(const wchar_t* processName)
{
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE)
    {
        return false;
    }

    PROCESSENTRY32 pe32 = { 0 };
    pe32.dwSize = sizeof(PROCESSENTRY32);

    if (Process32First(hSnapshot, &pe32))
    {
        do
        {
            if (wcscmp(pe32.szExeFile, processName) == 0)
            {
                CloseHandle(hSnapshot);
                return true;
            }
        } 
        while (Process32Next(hSnapshot, &pe32));
    }

    CloseHandle(hSnapshot);
    return false;
}

// function to monitor the process and set priority, and check if the process terminates
void MonitorAndSetPriority()
{
    HANDLE processHandle = NULL;
    while (true)
    {
        HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (hSnapshot == INVALID_HANDLE_VALUE)
        {
            Sleep(100); // retry if snapshot fails
            continue;
        }

        PROCESSENTRY32 pe32 = { 0 };
        pe32.dwSize = sizeof(PROCESSENTRY32);

        bool processFound = false;

        if (Process32First(hSnapshot, &pe32))
        {
            do
            {
                if (wcscmp(pe32.szExeFile, APP_NAME) == 0)
                {
                    processFound = true;
                    processHandle = OpenProcess(PROCESS_SET_INFORMATION, FALSE, pe32.th32ProcessID);
                    if (processHandle)
                    {
                        if (SetPriorityClass(processHandle, HIGH_PRIORITY_CLASS))
                        {
                            // confirm the priority has been set correctly
                            if (GetPriorityClass(processHandle) == HIGH_PRIORITY_CLASS)
                            {
                                CloseHandle(processHandle);
                                CloseHandle(hSnapshot);
                                return;
                            }
                        }
                        CloseHandle(processHandle);
                        processHandle = NULL;
                    }
                }
            } 
            while (Process32Next(hSnapshot, &pe32));
        }

        CloseHandle(hSnapshot);

        if (!processFound)
        {
            exit(1);
        }

        Sleep(1000); // check every set amount of seconds
    }
}

// function to get the string file info from a file's version info
std::map<std::wstring, std::wstring> GetFileVersionStrings(const std::wstring& filePath)
{
    std::map<std::wstring, std::wstring> versionInfoStrings;
    DWORD dummy = 0;
    DWORD size = GetFileVersionInfoSizeW(filePath.c_str(), &dummy);

    if (size == 0) 
    {
        versionInfoStrings[L"Error"] = L"GetFileVersionInfoSizeW failed";
        return versionInfoStrings;
    }

    std::vector<char> buffer(size);
    if (!GetFileVersionInfoW(filePath.c_str(), 0, size, buffer.data())) 
    {
        versionInfoStrings[L"Error"] = L"GetFileVersionInfoW failed";
        return versionInfoStrings;
    }

    // retrieve the specific version info
    VS_FIXEDFILEINFO* pFileInfo = nullptr;
    UINT fileInfoSize = 0;
    if (VerQueryValueW(buffer.data(), L"\\", (LPVOID*)&pFileInfo, &fileInfoSize))
    {
        if (fileInfoSize)
        {
            versionInfoStrings[L"FileVersion"] = std::to_wstring((pFileInfo->dwFileVersionMS >> 16) & 0xffff) + L"." +
                std::to_wstring((pFileInfo->dwFileVersionMS >> 0) & 0xffff) + L"." +
                std::to_wstring((pFileInfo->dwFileVersionLS >> 16) & 0xffff) + L"." +
                std::to_wstring((pFileInfo->dwFileVersionLS >> 0) & 0xffff);

            versionInfoStrings[L"ProductVersion"] = std::to_wstring((pFileInfo->dwProductVersionMS >> 16) & 0xffff) + L"." +
                std::to_wstring((pFileInfo->dwProductVersionMS >> 0) & 0xffff) + L"." +
                std::to_wstring((pFileInfo->dwProductVersionLS >> 16) & 0xffff) + L"." +
                std::to_wstring((pFileInfo->dwProductVersionLS >> 0) & 0xffff);
        }
    }

    void* verData = nullptr;
    UINT verDataLen = 0;

    struct LANGANDCODEPAGE 
    {
        WORD wLanguage;
        WORD wCodePage;
    } *lpTranslate = nullptr;

    UINT cbTranslate = 0;
    if (VerQueryValueW(buffer.data(), L"\\VarFileInfo\\Translation", (LPVOID*)&lpTranslate, &cbTranslate)) 
    {
        for (UINT i = 0; i < (cbTranslate / sizeof(struct LANGANDCODEPAGE)); i++) 
        {
            wchar_t subBlock[50];
            swprintf_s(subBlock, 50, L"\\StringFileInfo\\%04x%04x\\", lpTranslate[i].wLanguage, lpTranslate[i].wCodePage);

            std::wstring keys[] = { L"ProductName", L"CompanyName", L"FileDescription", L"InternalName", L"OriginalFilename", L"LegalCopyright" };

            for (const auto& key : keys) 
            {
                if (VerQueryValueW(buffer.data(), (std::wstring(subBlock) + key).c_str(), &verData, &verDataLen)) 
                {
                    versionInfoStrings[key] = std::wstring(static_cast<wchar_t*>(verData));
                }
            }
        }
    }
    else 
    {
        versionInfoStrings[L"Error"] = L"VerQueryValueW for Translation failed";
    }
    return versionInfoStrings;
}

// console control handler function
BOOL WINAPI ConsoleHandler(DWORD dwCtrlType)
{
    int msgboxID = IDNO; // default initialization

    switch (dwCtrlType)
    {
    case CTRL_CLOSE_EVENT:
        msgboxID = MessageBox(
            NULL,
            L"The launcher still has ongoing operations. It is advised to wait until the launcher completes its tasks, or automatically times out. Are you sure you want to close the launcher?",
            L"Warning",
            MB_ICONWARNING | MB_YESNO
        );

        if (msgboxID == IDYES)
        {
            return FALSE; // allow the console to close
        }
        else
        {
            return TRUE; // prevent the console from closing
        }
    default:
        return FALSE;
    }
}

// function to query "bitness" of the specified application
bool Is32BitApplication(const std::wstring& filePath)
{
    DWORD binaryType;
    if (GetBinaryTypeW(filePath.c_str(), &binaryType))
    {
        return binaryType == SCS_32BIT_BINARY;
    }
    return false;
}

// function to query 4gb patch
bool IsLargeAddressAware(const std::wstring& filePath)
{
    HANDLE hFile = CreateFileW(filePath.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
    {
        return false;
    }

    HANDLE hFileMapping = CreateFileMappingW(hFile, NULL, PAGE_READONLY, 0, 0, NULL);
    if (!hFileMapping)
    {
        CloseHandle(hFile);
        return false;
    }

    LPVOID lpFileBase = MapViewOfFile(hFileMapping, FILE_MAP_READ, 0, 0, 0);
    if (!lpFileBase)
    {
        CloseHandle(hFileMapping);
        CloseHandle(hFile);
        return false;
    }

    PIMAGE_DOS_HEADER pDOSHeader = (PIMAGE_DOS_HEADER)lpFileBase;
    PIMAGE_NT_HEADERS pNTHeaders = (PIMAGE_NT_HEADERS)((DWORD_PTR)lpFileBase + pDOSHeader->e_lfanew);

    bool result = (pNTHeaders->FileHeader.Characteristics & IMAGE_FILE_LARGE_ADDRESS_AWARE) != 0;

    UnmapViewOfFile(lpFileBase);
    CloseHandle(hFileMapping);
    CloseHandle(hFile);

    return result;
}

// function to apply the 4gbpatch
bool ApplyLargeAddressAwarePatch(const std::wstring& filePath)
{
    HANDLE hFile = CreateFileW(filePath.c_str(), GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
    {
        return false;
    }

    HANDLE hFileMapping = CreateFileMappingW(hFile, NULL, PAGE_READWRITE, 0, 0, NULL);
    if (!hFileMapping)
    {
        CloseHandle(hFile);
        return false;
    }

    LPVOID lpFileBase = MapViewOfFile(hFileMapping, FILE_MAP_WRITE, 0, 0, 0);
    if (!lpFileBase)
    {
        CloseHandle(hFileMapping);
        CloseHandle(hFile);
        return false;
    }

    PIMAGE_DOS_HEADER pDOSHeader = (PIMAGE_DOS_HEADER)lpFileBase;
    PIMAGE_NT_HEADERS pNTHeaders = (PIMAGE_NT_HEADERS)((DWORD_PTR)lpFileBase + pDOSHeader->e_lfanew);

    pNTHeaders->FileHeader.Characteristics |= IMAGE_FILE_LARGE_ADDRESS_AWARE;

    UnmapViewOfFile(lpFileBase);
    CloseHandle(hFileMapping);
    CloseHandle(hFile);

    return true;
}

// function to unapply the 4gbpatch
bool UnapplyLargeAddressAwarePatch(const std::wstring& filePath)
{
    HANDLE hFile = CreateFileW(filePath.c_str(), GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
    {
        return false;
    }

    HANDLE hFileMapping = CreateFileMappingW(hFile, NULL, PAGE_READWRITE, 0, 0, NULL);
    if (!hFileMapping)
    {
        CloseHandle(hFile);
        return false;
    }

    LPVOID lpFileBase = MapViewOfFile(hFileMapping, FILE_MAP_WRITE, 0, 0, 0);
    if (!lpFileBase)
    {
        CloseHandle(hFileMapping);
        CloseHandle(hFile);
        return false;
    }

    PIMAGE_DOS_HEADER pDOSHeader = (PIMAGE_DOS_HEADER)lpFileBase;
    PIMAGE_NT_HEADERS pNTHeaders = (PIMAGE_NT_HEADERS)((DWORD_PTR)lpFileBase + pDOSHeader->e_lfanew);

    pNTHeaders->FileHeader.Characteristics &= ~IMAGE_FILE_LARGE_ADDRESS_AWARE;

    UnmapViewOfFile(lpFileBase);
    CloseHandle(hFileMapping);
    CloseHandle(hFile);

    return true;
}

// function to suspend a process
void SuspendProcess(DWORD processId)
{
    HANDLE hThreadSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
    if (hThreadSnapshot != INVALID_HANDLE_VALUE)
    {
        THREADENTRY32 threadEntry = { 0 };
        threadEntry.dwSize = sizeof(THREADENTRY32);
        if (Thread32First(hThreadSnapshot, &threadEntry))
        {
            do
            {
                if (threadEntry.th32OwnerProcessID == processId)
                {
                    HANDLE hThread = OpenThread(THREAD_SUSPEND_RESUME, FALSE, threadEntry.th32ThreadID);
                    if (hThread != NULL)
                    {
                        SuspendThread(hThread);
                        CloseHandle(hThread);
                    }
                }
            } 
            while (Thread32Next(hThreadSnapshot, &threadEntry));
        }
        CloseHandle(hThreadSnapshot);
    }
}

// function to resume a process
void ResumeProcess(DWORD processId)
{
    HANDLE hThreadSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
    if (hThreadSnapshot != INVALID_HANDLE_VALUE)
    {
        THREADENTRY32 threadEntry = { 0 };
        threadEntry.dwSize = sizeof(THREADENTRY32);
        if (Thread32First(hThreadSnapshot, &threadEntry))
        {
            do
            {
                if (threadEntry.th32OwnerProcessID == processId)
                {
                    HANDLE hThread = OpenThread(THREAD_SUSPEND_RESUME, FALSE, threadEntry.th32ThreadID);
                    if (hThread != NULL)
                    {
                        ResumeThread(hThread);
                        CloseHandle(hThread);
                    }
                }
            } 
            while (Thread32Next(hThreadSnapshot, &threadEntry));
        }
        CloseHandle(hThreadSnapshot);
    }
}

struct WindowInfo
{
    DWORD processId;
    HWND hwnd;
};

// function to enumerate windows process
BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam)
{
    DWORD wndProcessId;
    GetWindowThreadProcessId(hwnd, &wndProcessId);

    if (wndProcessId == reinterpret_cast<WindowInfo*>(lParam)->processId)
    {
        WCHAR windowTitle[256];
        GetWindowText(hwnd, windowTitle, sizeof(windowTitle) / sizeof(WCHAR));
        if (wcslen(windowTitle) > 0)
        {
            reinterpret_cast<WindowInfo*>(lParam)->hwnd = hwnd;
            return FALSE; // stop enumeration
        }
    }
    return TRUE; // continue enumeration
}

HWND GetMainWindowHandle(DWORD processId)
{
    WindowInfo info = { processId, NULL };
    EnumWindows(EnumWindowsProc, reinterpret_cast<LPARAM>(&info));
    return info.hwnd;
}

// function to force focus on the window
void ForceFocusOnWindow(HWND hwnd)
{
    // check if the window handle is valid
    if (hwnd == NULL)
    {
        return;
    }

    // bring the window to the foreground
    SetForegroundWindow(hwnd);

    // show the window if it is minimized or hidden
    if (IsIconic(hwnd) || !IsWindowVisible(hwnd))
    {
        ShowWindow(hwnd, SW_RESTORE);
    }
    else
    {
        ShowWindow(hwnd, SW_SHOW);
    }

    // set the focus to the window
    SetFocus(hwnd);
}

// function to check if the OS is Windows 7 or earlier
bool IsWindows7OrEarlier() 
{
    OSVERSIONINFOEX osvi;
    ZeroMemory(&osvi, sizeof(OSVERSIONINFOEX));
    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
    osvi.dwMajorVersion = 6;
    osvi.dwMinorVersion = 1;

    DWORDLONG const dwlConditionMask = VerSetConditionMask(
        VerSetConditionMask(
            0, VER_MAJORVERSION, VER_LESS_EQUAL),
        VER_MINORVERSION, VER_LESS_EQUAL);

    return VerifyVersionInfo(
        &osvi, VER_MAJORVERSION | VER_MINORVERSION,
        dwlConditionMask) != FALSE;
}

// function to get the full path of a file
std::wstring GetFullPath(const std::wstring& path) 
{
    wchar_t fullPath[MAX_PATH];
    if (GetFullPathName(path.c_str(), MAX_PATH, fullPath, NULL) == 0) 
    {
        return L"";
    }
    return std::wstring(fullPath);
}

// function to check the compatibility mode
bool CheckCompatibilityMode(const std::wstring& exePath) 
{
    std::wstring fullPath = GetFullPath(exePath);
    if (fullPath.empty()) 
    {
        return false;
    }

    HKEY hKey;
    std::wstring registryPath = L"Software\\Microsoft\\Windows NT\\CurrentVersion\\AppCompatFlags\\Layers";
    LONG result = RegOpenKeyEx(HKEY_CURRENT_USER, registryPath.c_str(), 0, KEY_READ, &hKey);
    if (result != ERROR_SUCCESS) 
    {
        return false;
    }

    DWORD dataSize = 0;
    result = RegGetValue(hKey, NULL, fullPath.c_str(), RRF_RT_REG_SZ, NULL, NULL, &dataSize);
    if (result == ERROR_SUCCESS) 
    {
        std::wstring value(static_cast<DWORD>(dataSize) / sizeof(wchar_t), L'\0');
        result = RegGetValue(hKey, NULL, fullPath.c_str(), RRF_RT_REG_SZ, NULL, &value[0], &dataSize);
        RegCloseKey(hKey);
        if (result == ERROR_SUCCESS) 
        {
            value.resize((static_cast<DWORD>(dataSize) / sizeof(wchar_t)) - 1);  // adjust the size after reading
            if (value.find(L"WIN7RTM") != std::wstring::npos) 
            {
                return true; // WIN7RTM compatibility mode is already set
            }
        }
    }

    RegCloseKey(hKey);
    return false; // compatibility mode not set to WIN7RTM
}

// function to set the WIN7RTM compatibility mode
bool SetCompatibilityMode(const std::wstring& exePath) 
{
    std::wstring fullPath = GetFullPath(exePath);
    if (fullPath.empty()) 
    {
        return false;
    }

    HKEY hKey;
    std::wstring registryPath = L"Software\\Microsoft\\Windows NT\\CurrentVersion\\AppCompatFlags\\Layers";
    LONG result = RegCreateKeyEx(HKEY_CURRENT_USER, registryPath.c_str(), 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, NULL);
    if (result != ERROR_SUCCESS) 
    {
        return false;
    }

    std::wstring value = L"WIN7RTM";
    result = RegSetValueEx(hKey, fullPath.c_str(), 0, REG_SZ, reinterpret_cast<const BYTE*>(value.c_str()), static_cast<DWORD>((value.size() + 1) * sizeof(wchar_t)));
    RegCloseKey(hKey);

    if (result != ERROR_SUCCESS) 
    {
        return false;
    }
    return true;
}

// function to remove the WIN7RTM compatibility mode
bool RemoveWin7RtmCompatibilityMode(const std::wstring& exePath) 
{
    std::wstring fullPath = GetFullPath(exePath);
    if (fullPath.empty()) 
    {
        return false;
    }

    HKEY hKey;
    std::wstring registryPath = L"Software\\Microsoft\\Windows NT\\CurrentVersion\\AppCompatFlags\\Layers";
    LONG result = RegOpenKeyEx(HKEY_CURRENT_USER, registryPath.c_str(), 0, KEY_READ | KEY_WRITE, &hKey);
    if (result != ERROR_SUCCESS) 
    {
        return false;
    }

    DWORD dataSize = 0;
    result = RegGetValue(hKey, NULL, fullPath.c_str(), RRF_RT_REG_SZ, NULL, NULL, &dataSize);
    if (result == ERROR_SUCCESS) 
    {
        std::wstring value(static_cast<DWORD>(dataSize) / sizeof(wchar_t), L'\0');
        result = RegGetValue(hKey, NULL, fullPath.c_str(), RRF_RT_REG_SZ, NULL, &value[0], &dataSize);
        if (result == ERROR_SUCCESS) 
        {
            value.resize((static_cast<DWORD>(dataSize) / sizeof(wchar_t)) - 1);  // adjust the size after reading

            size_t pos = value.find(L"WIN7RTM");
            if (pos != std::wstring::npos) 
            {
                value.erase(pos, wcslen(L"WIN7RTM"));
                // remove any extra spaces
                value.erase(std::remove(value.begin(), value.end(), L' '), value.end());
                if (value.empty()) 
                {
                    result = RegDeleteValue(hKey, fullPath.c_str());
                    if (result != ERROR_SUCCESS) 
                    {
                        RegCloseKey(hKey);
                        return false;
                    }
                }
                else 
                {
                    result = RegSetValueEx(hKey, fullPath.c_str(), 0, REG_SZ, reinterpret_cast<const BYTE*>(value.c_str()), static_cast<DWORD>((value.size() + 1) * sizeof(wchar_t)));
                    if (result != ERROR_SUCCESS) 
                    {
                        RegCloseKey(hKey);
                        return false;
                    }
                }
            }
        }
    }

    RegCloseKey(hKey);
    return true;
}



//
//
//
//CROSS-PLATFORM FUNCTIONS
//
//
//



// function to get the current process name
std::string get_current_process_name() 
{
    std::string executablePath = boost::dll::program_location().string();
    std::vector<std::string> pathComponents;
    boost::split(pathComponents, executablePath, boost::is_any_of("\\/"));
    std::string executableName = pathComponents.empty() ? "" : pathComponents.back();
    return executableName;
}

// function to check if a process with the given name is already running
bool is_process_running(const std::string& process_name) 
{
    // determine which command to use based on the platform
    std::string command = bp::search_path("tasklist").string();
    std::vector<std::string> args;

    if (!command.empty()) 
    {
        // windows platform
        args = { "/FI", "IMAGENAME eq " + process_name };
    }
    else 
    {
        // unix-like platform
        command = bp::search_path("pgrep").string();
        args = { process_name };
    }

    bp::ipstream pipe_stream;
    bp::child c(command, bp::args(args), bp::std_out > pipe_stream);

    std::string line;
    int count = 0;
    while (pipe_stream && std::getline(pipe_stream, line) && !line.empty()) 
    {
        if (line.find(process_name) != std::string::npos) 
        {
            ++count;
            if (count > 1) 
            {
                c.terminate();
                return true;
            }
        }
    }

    c.wait();
    return false;
}

// simple function to check if a file exists
bool FileExists(const std::string& path) 
{
    return boost::filesystem::exists(path);
}

// function to check for GPU vulkan support
bool HasVulkanSupport()
{
    VkInstance instance;
    VkApplicationInfo appInfo = {};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "VulkanCheck";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;

    VkInstanceCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

    if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) 
    {
        return false;
    }

    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
    vkDestroyInstance(instance, nullptr);

    return deviceCount > 0;
}

// helper function to trim whitespace from a wide string
std::wstring TrimWString(const std::wstring& str)
{
    size_t first = str.find_first_not_of(L' ');
    if (first == std::wstring::npos)
        return L"";
    size_t last = str.find_last_not_of(L' ');
    return str.substr(first, last - first + 1);
}

// helper function to trim whitespace from a string
std::string TrimString(const std::string& str)
{
    size_t first = str.find_first_not_of(' ');
    if (first == std::string::npos)
        return "";
    size_t last = str.find_last_not_of(' ');
    return str.substr(first, last - first + 1);
}

// function to validate launch parameter field formatting
bool ValidateLaunchParams(const std::wstring& launchParams)
{
    std::wistringstream iss(launchParams);
    std::wstring token;
    std::wregex paramRegex(L"^-\\w+|^-\\w+\\s+\\w+$");

    while (iss >> token)
    {
        if (!std::regex_match(token, paramRegex))
        {
            return false;
        }
    }

    return true;
}

// function to validate file name field formatting
bool ValidateFileName(const std::wstring& fileName)
{
    size_t pos = fileName.find(L".");
    return (pos != std::wstring::npos) && (pos < fileName.length() - 1);
}

// function to validate boolean fields
bool ValidateBooleanField(const std::wstring& value)
{
    return value == L"true" || value == L"false";
}

// function to validate integer fields
bool ValidateIntegerField(const std::wstring& value)
{
    std::wregex intRegex(L"^-?\\d+$");
    return std::regex_match(value, intRegex);
}

// function to check if we're running on Windows
bool RunningOnWindows() 
{
    return BOOST_OS_WINDOWS;
}

// function to check if we're running on Linux
bool RunningOnLinux() 
{
    return BOOST_OS_LINUX;
}

// function to convert wide string to string
std::string WStringToString(const std::wstring& wstr) 
{
    return boost::locale::conv::utf_to_utf<char>(wstr);
}

// function to convert string to wide string
std::wstring StringToWString(const std::string& str) {
    return boost::locale::conv::utf_to_utf<wchar_t>(str);
}

// function to copy raw data from source to destination
bool CopyFileRaw(const std::wstring& srcFilePath, const std::wstring& dstFilePath) 
{
    try 
    {
        boost::filesystem::copy_file(boost::locale::conv::utf_to_utf<char>(srcFilePath),
            boost::locale::conv::utf_to_utf<char>(dstFilePath),
            boost::filesystem::copy_options::overwrite_existing);
        return true;
    }
    catch (const boost::filesystem::filesystem_error& e) 
    {
        std::cerr << e.what() << std::endl;
        return false;
    }
}

// function to verify the 16:9 aspect ratio
bool CheckAspectRatio(int width, int height) 
{
    return (width * 9 == height * 16);
}