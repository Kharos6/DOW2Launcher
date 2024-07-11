// header for storing generic functions, not including functions specific to the game or its files

#pragma once

#define WIN32_LEAN_AND_MEAN
#define BOOST_DISABLE_CURRENT_LOCATION
#define TIMEOUT_PROCESS 60000 // absolute timeout for the entire process
#define CONSOLE_MESSAGE(msg) \
    if (consoleShown) { \
        std::wcout << msg << std::endl; \
    }

// standard library headers
#include <iostream>
#include <fstream>
#include <sstream>
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
#include <cwctype>
#include <iomanip>
#include <thread>

// windows headers
#include <windows.h>
#include <tlhelp32.h>
#include <wincrypt.h>
#include <shlobj.h>
#include <psapi.h>
#include <gdiplus.h>

// boost headers
#include <boost/process.hpp>
#include <boost/interprocess/sync/named_mutex.hpp>
#include <boost/interprocess/exceptions.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <boost/locale.hpp>
#include <boost/dll/runtime_symbol_info.hpp>
#include <boost/predef/os.h>

// local headers
#include "vulkan/vulkan.h"

using namespace Gdiplus;

namespace bp = boost::process;
namespace fs = boost::filesystem;



//
//
//
// WINDOWS-EXCLUSIVE FUNCTIONS
//
//
//



// function to display a bitmap while launching
HWND ShowBitmap(HINSTANCE hInstance, const std::wstring & bitmapFileName)
{
    // load the bitmap from the file
    HBITMAP hBitmap = (HBITMAP)LoadImage(NULL, bitmapFileName.c_str(), IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
    if (!hBitmap)
    {
        return NULL;
    }

    // retrieve the bitmap's dimensions
    BITMAP bitmap;
    GetObject(hBitmap, sizeof(BITMAP), &bitmap);
    int bitmapWidth = bitmap.bmWidth;
    int bitmapHeight = bitmap.bmHeight;

    // get the screen dimensions
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);

    // calculate the position to center the bitmap on the screen
    int xPos = (screenWidth - bitmapWidth) / 2;
    int yPos = (screenHeight - bitmapHeight) / 2;

    // create the window to display the bitmap
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
        // clean up the bitmap if window creation fails
        DeleteObject(hBitmap);
        return NULL;
    }

    // set the bitmap to the window
    SendMessage(hwnd, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)hBitmap);

    // show and update the window
    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);

    return hwnd;
}

// thread function to run the bitmap display
void BitmapThread(HINSTANCE hInstance, const std::wstring& bitmapFileName)
{
    HWND hwnd = ShowBitmap(hInstance, bitmapFileName);

    if (hwnd != NULL)
    {
        // run a message loop for the bitmap window
        MSG msg;
        while (GetMessage(&msg, NULL, 0, 0))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
}

// global variable for GDI+ token
ULONG_PTR gdiplusToken;

// function to initialize GDI+
void InitializeGDIPlus() 
{
    GdiplusStartupInput gdiplusStartupInput;
    GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);
}

// function to shutdown GDI+
void ShutdownGDIPlus() 
{
    GdiplusShutdown(gdiplusToken);
}

// function to show the GIF splash screen
HWND ShowGif(HINSTANCE hInstance, const std::wstring& gifFileName)
{
    HWND hwnd = NULL;
    Image* image = new Image(gifFileName.c_str());
    if (image->GetLastStatus() == Ok)
    {
        UINT width = image->GetWidth();
        UINT height = image->GetHeight();

        WNDCLASS wndclass = { 0 };
        wndclass.lpfnWndProc = [](HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) -> LRESULT
            {
                static Image* image = reinterpret_cast<Image*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
                static int frameIndex = 0;
                static UINT frameCount = 0;
                static UINT* delays = nullptr;
                static UINT_PTR timerID = 1;
                static HDC hdcMem = nullptr;
                static HBITMAP hbmMem = nullptr;
                static HBITMAP hbmOld = nullptr;
                static HDC hdcWindow = nullptr;

                switch (message)
                {
                case WM_CREATE:
                {
                    CREATESTRUCT* cs = reinterpret_cast<CREATESTRUCT*>(lParam);
                    image = reinterpret_cast<Image*>(cs->lpCreateParams);
                    SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)image);

                    GUID pageGuid = FrameDimensionTime;
                    frameCount = image->GetFrameCount(&pageGuid);
                    delays = new UINT[frameCount];
                    PropertyItem* propItem = new PropertyItem[frameCount];
                    int size = image->GetPropertyItemSize(PropertyTagFrameDelay);
                    image->GetPropertyItem(PropertyTagFrameDelay, size, propItem);
                    memcpy(delays, propItem->value, frameCount * sizeof(UINT));
                    delete[] propItem;
                    SetTimer(hwnd, timerID, delays[frameIndex] * 10, NULL);

                    hdcWindow = GetDC(hwnd);
                    hdcMem = CreateCompatibleDC(hdcWindow);
                    hbmMem = CreateCompatibleBitmap(hdcWindow, image->GetWidth(), image->GetHeight());
                    hbmOld = (HBITMAP)SelectObject(hdcMem, hbmMem);
                }
                break;

                case WM_TIMER:
                    if (wParam == timerID)
                    {
                        frameIndex = (frameIndex + 1) % frameCount;
                        image->SelectActiveFrame(&FrameDimensionTime, frameIndex);
                        InvalidateRect(hwnd, NULL, FALSE);
                        SetTimer(hwnd, timerID, delays[frameIndex] * 10, NULL);
                    }
                    break;

                case WM_PAINT:
                {
                    PAINTSTRUCT ps;
                    HDC hdc = BeginPaint(hwnd, &ps);
                    Graphics graphics(hdcMem);
                    graphics.Clear(Color(0, 0, 0, 0));
                    graphics.DrawImage(image, 0, 0, image->GetWidth(), image->GetHeight());

                    BLENDFUNCTION blendFunction;
                    blendFunction.BlendOp = AC_SRC_OVER;
                    blendFunction.BlendFlags = 0;
                    blendFunction.SourceConstantAlpha = 255;
                    blendFunction.AlphaFormat = AC_SRC_ALPHA;

                    AlphaBlend(hdc, 0, 0, image->GetWidth(), image->GetHeight(), hdcMem, 0, 0, image->GetWidth(), image->GetHeight(), blendFunction);
                    EndPaint(hwnd, &ps);
                }
                break;

                case WM_DESTROY:
                    KillTimer(hwnd, timerID);
                    delete[] delays;

                    SelectObject(hdcMem, hbmOld);
                    DeleteObject(hbmMem);
                    DeleteDC(hdcMem);
                    ReleaseDC(hwnd, hdcWindow);

                    PostQuitMessage(0);
                    break;

                default:
                    return DefWindowProc(hwnd, message, wParam, lParam);
                }
                return 0;
            };

        wndclass.hInstance = hInstance;
        wndclass.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
        wndclass.lpszClassName = L"GifSplashScreen";

        RegisterClass(&wndclass);

        hwnd = CreateWindowEx(
            WS_EX_LAYERED,
            L"GifSplashScreen",
            NULL,
            WS_VISIBLE | WS_POPUP | SS_BITMAP,
            (GetSystemMetrics(SM_CXSCREEN) - width) / 2,
            (GetSystemMetrics(SM_CYSCREEN) - height) / 2,
            width,
            height,
            NULL,
            NULL,
            hInstance,
            image
        );

        SetLayeredWindowAttributes(hwnd, RGB(255, 255, 255), 0, LWA_COLORKEY);
        ShowWindow(hwnd, SW_SHOW);
        UpdateWindow(hwnd);

        return hwnd;
    }
    else
    {
        delete image;
        return NULL;
    }
}

// thread function to run the gif splash screen
void GifThread(HINSTANCE hInstance, const std::wstring& bitmapFileName)
{
    HWND hwnd = ShowGif(hInstance, bitmapFileName);

    if (hwnd != NULL)
    {
        // run a message loop for the bitmap window
        MSG msg;
        while (GetMessage(&msg, NULL, 0, 0))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
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

// function to terminate a process by ID
bool TerminateProcessById(DWORD processId)
{
    HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, processId);
    if (hProcess == NULL)
        return FALSE;

    BOOL result = TerminateProcess(hProcess, 1);
    CloseHandle(hProcess);
    return result;
}

// helper function to confirm for process termination
bool WaitForProcessTermination(DWORD processId, DWORD timeoutMillis)
{
    HANDLE hProcess = OpenProcess(SYNCHRONIZE, FALSE, processId);
    if (hProcess == NULL)
        return false;

    DWORD waitResult = WaitForSingleObject(hProcess, timeoutMillis);
    CloseHandle(hProcess);

    return waitResult == WAIT_OBJECT_0; // return true if the process terminated, false otherwise
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

bool CheckAndConvertToWindowsCRLF(const std::wstring& fileName)
{
    // read the BOM to determine the encoding
    std::ifstream file(fileName, std::ios::binary);
    if (!file.is_open())
    {
        return false;
    }

    // read entire file content into a string
    std::string raw_content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();

    // check and determine the encoding
    std::wstring content;
    if (raw_content.size() >= 2 && raw_content[0] == char(0xFF) && raw_content[1] == char(0xFE))
    {
        // UTF-16 LE
        content = std::wstring(reinterpret_cast<const wchar_t*>(raw_content.data() + 2), (raw_content.size() - 2) / 2);
    }
    else if (raw_content.size() >= 2 && raw_content[0] == char(0xFE) && raw_content[1] == char(0xFF))
    {
        // UTF-16 BE
        std::wstring utf16be_content(reinterpret_cast<const wchar_t*>(raw_content.data() + 2), (raw_content.size() - 2) / 2);
        // swap bytes to convert from BE to LE
        for (size_t i = 0; i < utf16be_content.size(); ++i)
        {
            wchar_t ch = utf16be_content[i];
            utf16be_content[i] = (ch >> 8) | (ch << 8);
        }
        content = utf16be_content;
    }
    else if (raw_content.size() >= 3 && raw_content[0] == char(0xEF) && raw_content[1] == char(0xBB) && raw_content[2] == char(0xBF))
    {
        // UTF-8 with BOM
        std::string utf8_content = raw_content.substr(3);
        content = boost::locale::conv::utf_to_utf<wchar_t>(utf8_content);
    }
    else
    {
        // assume UTF-8 without BOM
        content = boost::locale::conv::utf_to_utf<wchar_t>(raw_content);
    }

    // check if conversion to CRLF is needed
    bool needsConversion = false;
    for (size_t i = 0; i < content.size(); ++i)
    {
        if ((content[i] == L'\n' && (i == 0 || content[i - 1] != L'\r')) || content[i] == L'\r')
        {
            if (content[i] == L'\r' && (i + 1 >= content.size() || content[i + 1] != L'\n'))
            {
                // found CR without LF, needs conversion
                needsConversion = true;
                break;
            }
            if (content[i] == L'\n' && (i == 0 || content[i - 1] != L'\r'))
            {
                // found LF without preceding CR, needs conversion
                needsConversion = true;
                break;
            }
        }
    }

    if (!needsConversion)
    {
        return true; // file is already in CRLF format
    }

    // convert content to CRLF format
    std::wstring convertedContent;
    for (size_t i = 0; i < content.size(); ++i)
    {
        if (content[i] == L'\r')
        {
            if (i + 1 < content.size() && content[i + 1] == L'\n')
            {
                // handle already existing CRLF
                convertedContent += L"\r\n";
                ++i; // Skip the '\n'
            }
            else
            {
                // handle Macintosh CR
                convertedContent += L"\r\n";
            }
        }
        else if (content[i] == L'\n')
        {
            if (i == 0 || content[i - 1] != L'\r')
            {
                // handle LF not preceded by CR (Unix LF)
                convertedContent += L"\r\n";
            }
        }
        else
        {
            convertedContent += content[i];
        }
    }

    // write the converted content back to the file
    std::wofstream outFile(fileName, std::ios::binary | std::ios::trunc);
    if (!outFile.is_open())
    {
        return false;
    }

    if (raw_content.size() >= 2 && raw_content[0] == char(0xFF) && raw_content[1] == char(0xFE))
    {
        // UTF-16 LE
        outFile.imbue(std::locale(outFile.getloc(), new std::codecvt_utf16<wchar_t, 0x10FFFF, std::little_endian>));
        outFile.write(L"\uFEFF", 1); // write BOM
    }
    else if (raw_content.size() >= 2 && raw_content[0] == char(0xFE) && raw_content[1] == char(0xFF))
    {
        // UTF-16 BE
        outFile.imbue(std::locale(outFile.getloc(), new std::codecvt_utf16<wchar_t, 0x10FFFF, std::consume_header>));
        outFile.write(L"\uFEFF", 1); // write BOM
        for (size_t i = 0; i < convertedContent.size(); ++i)
        {
            wchar_t ch = convertedContent[i];
            wchar_t be_ch = (ch >> 8) | (ch << 8);
            outFile.write(&be_ch, 1);
        }
        outFile.close();
        return true;
    }
    else
    {
        // UTF-8
        outFile.imbue(std::locale(outFile.getloc(), new std::codecvt_utf8<wchar_t>));
        if (raw_content.size() >= 3 && raw_content[0] == char(0xEF) && raw_content[1] == char(0xBB) && raw_content[2] == char(0xBF))
        {
            outFile << L'\uFEFF'; // write BOM if it was present in the original file
        }
    }

    outFile.write(convertedContent.c_str(), convertedContent.size());
    outFile.close();

    return true;
}

typedef VkResult(VKAPI_PTR* PFN_vkCreateInstance)(const VkInstanceCreateInfo*, const VkAllocationCallbacks*, VkInstance*);
typedef void (VKAPI_PTR* PFN_vkDestroyInstance)(VkInstance, const VkAllocationCallbacks*);
typedef VkResult(VKAPI_PTR* PFN_vkEnumeratePhysicalDevices)(VkInstance, uint32_t*, VkPhysicalDevice*);
typedef void (VKAPI_PTR* PFN_vkGetPhysicalDeviceProperties)(VkPhysicalDevice, VkPhysicalDeviceProperties*);

HMODULE LoadVulkanLibrary()
{
    return LoadLibrary(L"vulkan-1.dll");
}

void UnloadVulkanLibrary(HMODULE vulkanLib)
{
    if (vulkanLib)
    {
        FreeLibrary(vulkanLib);
    }
}

PFN_vkCreateInstance GetVkCreateInstanceFunction(HMODULE vulkanLib)
{
    return (PFN_vkCreateInstance)GetProcAddress(vulkanLib, "vkCreateInstance");
}

PFN_vkDestroyInstance GetVkDestroyInstanceFunction(HMODULE vulkanLib)
{
    return (PFN_vkDestroyInstance)GetProcAddress(vulkanLib, "vkDestroyInstance");
}

PFN_vkEnumeratePhysicalDevices GetVkEnumeratePhysicalDevicesFunction(HMODULE vulkanLib)
{
    return (PFN_vkEnumeratePhysicalDevices)GetProcAddress(vulkanLib, "vkEnumeratePhysicalDevices");
}

PFN_vkGetPhysicalDeviceProperties GetVkGetPhysicalDevicePropertiesFunction(HMODULE vulkanLib)
{
    return (PFN_vkGetPhysicalDeviceProperties)GetProcAddress(vulkanLib, "vkGetPhysicalDeviceProperties");
}

// function to check for GPU vulkan support
bool HasVulkanSupport()
{
    // load Vulkan library
    HMODULE vulkanLib = LoadVulkanLibrary();
    if (!vulkanLib)
    {
        return false;
    }

    // get function pointers
    PFN_vkCreateInstance vkCreateInstance = GetVkCreateInstanceFunction(vulkanLib);
    PFN_vkDestroyInstance vkDestroyInstance = GetVkDestroyInstanceFunction(vulkanLib);
    PFN_vkEnumeratePhysicalDevices vkEnumeratePhysicalDevices = GetVkEnumeratePhysicalDevicesFunction(vulkanLib);
    PFN_vkGetPhysicalDeviceProperties vkGetPhysicalDeviceProperties = GetVkGetPhysicalDevicePropertiesFunction(vulkanLib);

    if (!vkCreateInstance || !vkDestroyInstance || !vkEnumeratePhysicalDevices || !vkGetPhysicalDeviceProperties)
    {
        UnloadVulkanLibrary(vulkanLib);
        return false;
    }

    // initialize Vulkan
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
        UnloadVulkanLibrary(vulkanLib);
        return false;
    }

    // check for Vulkan-compatible devices
    uint32_t deviceCount = 0;
    if (vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr) != VK_SUCCESS || deviceCount == 0)
    {
        vkDestroyInstance(instance, nullptr);
        UnloadVulkanLibrary(vulkanLib);
        return false;
    }

    // clean up
    vkDestroyInstance(instance, nullptr);
    UnloadVulkanLibrary(vulkanLib);

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