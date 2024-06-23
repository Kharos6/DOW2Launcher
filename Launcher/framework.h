#pragma once
#pragma comment(lib, "Version.lib")

#define WIN32_LEAN_AND_MEAN
#define APP_NAME L"DOW2.exe"
#define TIMEOUT_PROCESS 60000 // absolute timeout for the entire process
#define CONSOLE_MESSAGE(msg) \
    if (consoleShown) { \
        std::wcout << msg << std::endl; \
    }

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

// helper function to convert a wide string to a narrow string
std::string WStringToString(const std::wstring& wstr)
{
    int sizeNeeded = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
    std::string str(sizeNeeded, 0);
    WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &str[0], sizeNeeded, NULL, NULL);
    return str;
}

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
        } while (Process32Next(hSnapshot, &pe32));
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
            } while (Process32Next(hSnapshot, &pe32));
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
std::map<std::wstring, std::wstring> GetFileVersionStrings(const std::wstring& filePath, bool verboseDebug)
{
    std::map<std::wstring, std::wstring> versionInfoStrings;
    DWORD dummy = 0;
    DWORD size = GetFileVersionInfoSizeW(filePath.c_str(), &dummy);

    if (size == 0) {
        versionInfoStrings[L"Error"] = L"GetFileVersionInfoSizeW failed";
        if (verboseDebug) {
            MessageBox(NULL, L"GetFileVersionInfoSizeW failed", L"Debug", MB_OK | MB_ICONINFORMATION);
        }
        return versionInfoStrings;
    }

    std::vector<char> buffer(size);
    if (!GetFileVersionInfoW(filePath.c_str(), 0, size, buffer.data())) {
        versionInfoStrings[L"Error"] = L"GetFileVersionInfoW failed";
        if (verboseDebug) {
            MessageBox(NULL, L"GetFileVersionInfoW failed", L"Debug", MB_OK | MB_ICONINFORMATION);
        }
        return versionInfoStrings;
    }

    void* verData = nullptr;
    UINT verDataLen = 0;

    struct LANGANDCODEPAGE {
        WORD wLanguage;
        WORD wCodePage;
    } *lpTranslate = nullptr;

    UINT cbTranslate = 0;
    if (VerQueryValueW(buffer.data(), L"\\VarFileInfo\\Translation", (LPVOID*)&lpTranslate, &cbTranslate)) {
        for (UINT i = 0; i < (cbTranslate / sizeof(struct LANGANDCODEPAGE)); i++) {
            wchar_t subBlock[50];
            swprintf_s(subBlock, 50, L"\\StringFileInfo\\%04x%04x\\", lpTranslate[i].wLanguage, lpTranslate[i].wCodePage);

            std::wstring keys[] = { L"ProductName", L"CompanyName", L"FileDescription", L"FileVersion" };

            for (const auto& key : keys) {
                if (VerQueryValueW(buffer.data(), (std::wstring(subBlock) + key).c_str(), &verData, &verDataLen)) {
                    versionInfoStrings[key] = std::wstring(static_cast<wchar_t*>(verData));
                    if (verboseDebug) {
                        MessageBox(NULL, (L"Found " + key + L": " + versionInfoStrings[key]).c_str(), L"Debug", MB_OK | MB_ICONINFORMATION);
                    }
                }
                else {
                    if (verboseDebug) {
                        MessageBox(NULL, (L"VerQueryValueW failed for " + key).c_str(), L"Debug", MB_OK | MB_ICONINFORMATION);
                    }
                }
            }
        }
    }
    else {
        versionInfoStrings[L"Error"] = L"VerQueryValueW for Translation failed";
        if (verboseDebug) {
            MessageBox(NULL, L"VerQueryValueW for Translation failed", L"Debug", MB_OK | MB_ICONINFORMATION);
        }
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
bool ValidateFileName(const std::wstring& injectorFileName)
{
    size_t pos = injectorFileName.find(L".");
    return (pos != std::wstring::npos) && (pos < injectorFileName.length() - 1);
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