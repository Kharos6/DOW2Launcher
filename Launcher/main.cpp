#include "framework.h"

// structure to hold the launch configuration
struct LaunchConfig
{
    bool Injector = false;
    std::string ExpectedInjectorFileMD5Checksum;
    std::wstring LaunchParams;
    int BitmapWidth = 600;  // default width
    int BitmapHeight = 300; // default height
    std::wstring InjectorFileName;
    bool IsRetribution = false;
    bool IsSteam = false;
    bool VerboseDebug = false;
    bool IsDXVK = false;
    bool FirstTimeLaunchCheck = false;
    std::wstring FirstTimeLaunchMessage;
};

// simple function to copy raw data from source to destination
bool CopyFileRaw(const std::wstring& srcFilePath, const std::wstring& dstFilePath)
{
    std::ifstream srcFile(srcFilePath, std::ios::binary);
    if (!srcFile.is_open())
    {
        MessageBox(NULL, (L"Failed to find or open the source file: " + srcFilePath + L". Reacquire it from the mod package, or try again.").c_str(), L"Error", MB_OK | MB_ICONERROR);
        return false;
    }

    std::ofstream dstFile(dstFilePath, std::ios::binary);
    if (!dstFile.is_open())
    {
        MessageBox(NULL, (L"Failed to find or open the destination file: " + dstFilePath + L". Reacquire it from the mod package, or try again.").c_str(), L"Error", MB_OK | MB_ICONERROR);
        srcFile.close();
        return false;
    }

    dstFile << srcFile.rdbuf();

    srcFile.close();
    dstFile.close();

    return true;
}

// function to read the launch configuration
LaunchConfig ReadLaunchConfig()
{
    wchar_t launcherPath[MAX_PATH];
    GetModuleFileName(NULL, launcherPath, MAX_PATH);

    std::wstring configFilePath = launcherPath;
    size_t lastDotPos = configFilePath.find_last_of(L".");
    if (lastDotPos != std::wstring::npos)
    {
        configFilePath.replace(lastDotPos, std::wstring::npos, L".launchconfig");
    }

    std::wifstream configFile(configFilePath);
    if (!configFile.is_open())
    {
        MessageBox(NULL, L"Failed to find or open the launch configuration file. Reacquire it from the mod package, or try again.", L"Error", MB_OK | MB_ICONERROR);
        exit(1);
    }

    LaunchConfig config;
    std::wstring line;
    std::set<std::wstring> requiredKeys = {
       L"Injector", L"ExpectedInjectorFileMD5Checksum", L"BitmapWidth", L"BitmapHeight", L"InjectorFileName", L"LaunchParams", L"IsRetribution", L"IsSteam", L"VerboseDebug", L"IsDXVK", L"FirstTimeLaunchCheck", L"FirstTimeLaunchMessage"
    };
    std::map<std::wstring, int> lineNumbers;
    int lineNumber = 0;

    while (std::getline(configFile, line))
    {
        lineNumber++;
        size_t pos = line.find(L"=");
        if (pos == std::wstring::npos) continue;

        std::wstring key = line.substr(0, pos);
        std::wstring value = line.substr(pos + 1);

        lineNumbers[key] = lineNumber;

        if (key == L"Injector")
        {
            if (!ValidateBooleanField(value))
            {
                MessageBox(NULL, L"Invalid formatting for the [Injector] field of the launch configuration file. It must be true or false.", L"Error", MB_OK | MB_ICONERROR);
                exit(1);
            }
            config.Injector = (value == L"true");
        }
        else if (key == L"ExpectedInjectorFileMD5Checksum")
        {
            config.ExpectedInjectorFileMD5Checksum = WStringToString(value);
        }
        else if (key == L"BitmapWidth")
        {
            if (!ValidateIntegerField(value))
            {
                MessageBox(NULL, L"Invalid formatting for the [BitmapWidth] field of the launch configuration file. It must be a number.", L"Error", MB_OK | MB_ICONERROR);
                exit(1);
            }
            config.BitmapWidth = std::stoi(value);
        }
        else if (key == L"BitmapHeight")
        {
            if (!ValidateIntegerField(value))
            {
                MessageBox(NULL, L"Invalid formatting for the [BitmapHeight] field of the launch configuration file. It must be a number.", L"Error", MB_OK | MB_ICONERROR);
                exit(1);
            }
            config.BitmapHeight = std::stoi(value);
        }
        else if (key == L"InjectorFileName")
        {
            config.InjectorFileName = value;
            if (!ValidateFileName(config.InjectorFileName))
            {
                MessageBox(NULL, L"Invalid formatting for the [InjectorFileName] field of the launch configuration file. The full file name must include the file extension.", L"Error", MB_OK | MB_ICONERROR);
                exit(1);
            }
        }
        else if (key == L"LaunchParams")
        {
            config.LaunchParams = value;
            if (!ValidateLaunchParams(config.LaunchParams))
            {
                MessageBox(NULL, L"Invalid formatting for the [LaunchParams] field of the launch configuration file. Each parameter must be separated by a space, and there must be no space between the first launch parameter and the [=] sign.", L"Error", MB_OK | MB_ICONERROR);
                exit(1);
            }
        }
        else if (key == L"IsRetribution")
        {
            if (!ValidateBooleanField(value))
            {
                MessageBox(NULL, L"Invalid formatting for the [IsRetribution] field of the launch configuration file. It must be true or false.", L"Error", MB_OK | MB_ICONERROR);
                exit(1);
            }
            config.IsRetribution = (value == L"true");
        }
        else if (key == L"IsSteam")
        {
            if (!ValidateBooleanField(value))
            {
                MessageBox(NULL, L"Invalid formatting for the [IsSteam] field of the launch configuration file. It must be true or false.", L"Error", MB_OK | MB_ICONERROR);
                exit(1);
            }
            config.IsSteam = (value == L"true");
        }
        else if (key == L"VerboseDebug")
        {
            if (!ValidateBooleanField(value))
            {
                MessageBox(NULL, L"Invalid formatting for the [VerboseDebug] field of the launch configuration file. It must be true or false.", L"Error", MB_OK | MB_ICONERROR);
                exit(1);
            }
            config.VerboseDebug = (value == L"true");
        }
        else if (key == L"IsDXVK")
        {
            if (!ValidateBooleanField(value))
            {
                MessageBox(NULL, L"Invalid formatting for the [IsDXVK] field of the launch configuration file. It must be true or false.", L"Error", MB_OK | MB_ICONERROR);
                exit(1);
            }
            config.IsDXVK = (value == L"true");
        }
        else if (key == L"FirstTimeLaunchCheck")
        {
            if (!ValidateBooleanField(value))
            {
                MessageBox(NULL, L"Invalid formatting for the [FirstTimeLaunchCheck] field of the launch configuration file. It must be true or false.", L"Error", MB_OK | MB_ICONERROR);
                exit(1);
            }
            config.FirstTimeLaunchCheck = (value == L"true");
        }
        else if (key == L"FirstTimeLaunchMessage")
        {
            config.FirstTimeLaunchMessage = value;
        }
        else
        {
            MessageBox(NULL, (L"Unexpected configuration key: " + key + L" on line " + std::to_wstring(lineNumber) + L". Reacquire the launch configuration file from the mod package, or try again.").c_str(), L"Error", MB_OK | MB_ICONERROR);
            exit(1);
        }

        requiredKeys.erase(key);
    }
    configFile.close();

    if (!requiredKeys.empty())
    {
        std::wstring errorMsg = L"Missing or misspelled configuration keys: ";
        for (const auto& key : requiredKeys)
        {
            errorMsg += key + L" (line " + std::to_wstring(lineNumbers[key]) + L"), ";
        }
        errorMsg.pop_back();
        errorMsg.pop_back();
        MessageBox(NULL, errorMsg.c_str(), L"Error", MB_OK | MB_ICONERROR);
        exit(1);
    }

    return config;
}

// function to write the launch configuration
void WriteLaunchConfig(const LaunchConfig& config)
{
    wchar_t launcherPath[MAX_PATH];
    GetModuleFileName(NULL, launcherPath, MAX_PATH);

    std::wstring configFilePath = launcherPath;
    size_t lastDotPos = configFilePath.find_last_of(L".");
    if (lastDotPos != std::wstring::npos)
    {
        configFilePath.replace(lastDotPos, std::wstring::npos, L".launchconfig");
    }

    std::wofstream configFile(configFilePath);
    if (!configFile.is_open())
    {
        MessageBox(NULL, L"Failed to find or open the launch configuration file. Reacquire it from the mod package, or try again.", L"Error", MB_OK | MB_ICONERROR);
        exit(1);
    }

    configFile << L"IsRetribution=" << (config.IsRetribution ? L"true" : L"false") << L"\n";
    configFile << L"IsSteam=" << (config.IsSteam ? L"true" : L"false") << L"\n";
    configFile << L"IsDXVK=" << (config.IsDXVK ? L"true" : L"false") << L"\n";
    configFile << L"LaunchParams=" << config.LaunchParams << L"\n";
    configFile << L"BitmapWidth=" << config.BitmapWidth << L"\n";
    configFile << L"BitmapHeight=" << config.BitmapHeight << L"\n";
    configFile << L"Injector=" << (config.Injector ? L"true" : L"false") << L"\n";
    configFile << L"InjectorFileName=" << config.InjectorFileName << L"\n";
    configFile << L"ExpectedInjectorFileMD5Checksum=" << std::wstring(config.ExpectedInjectorFileMD5Checksum.begin(), config.ExpectedInjectorFileMD5Checksum.end()) << L"\n";
    configFile << L"FirstTimeLaunchCheck=" << (config.FirstTimeLaunchCheck ? L"true" : L"false") << L"\n";
    configFile << L"FirstTimeLaunchMessage=" << config.FirstTimeLaunchMessage << L"\n";
    configFile << L"VerboseDebug=" << (config.VerboseDebug ? L"true" : L"false") << L"\n";

    configFile.close();
}

// function to handle the binary processing with a timeout
DWORD WINAPI BinaryProcessingThread(LPVOID lpParam)
{
    LaunchConfig* config = (LaunchConfig*)lpParam;
    std::string actualChecksum;

    if (!CalculateMD5(config->InjectorFileName.c_str(), actualChecksum) || actualChecksum != config->ExpectedInjectorFileMD5Checksum)
    {
        if (config->VerboseDebug) {
            MessageBox(NULL, (L"" + config->InjectorFileName + L" file checksum mismatch. Attempting to replace the file with the valid injector version for this mod.").c_str(), L"Debug", MB_OK | MB_ICONINFORMATION);
        }

        std::wstring binFileName = config->InjectorFileName.substr(0, config->InjectorFileName.find_last_of(L".")) + L".bin";

        if (!CopyFileRaw(binFileName, config->InjectorFileName))
        {
            MessageBox(NULL, (L"Failed to replace the " + config->InjectorFileName + L" file with the valid injector version for this mod. Reacquire it from the mod package, or try again.").c_str(), L"Error", MB_OK | MB_ICONERROR);
            return 1;
        }

        // verify the checksum again after replacement
        if (!CalculateMD5(config->InjectorFileName.c_str(), actualChecksum) || actualChecksum != config->ExpectedInjectorFileMD5Checksum)
        {
            MessageBox(NULL, (L"" + config->InjectorFileName + L" file checksum still mismatched after attempted replacement with the valid injector version for this mod. Reacquire it from the mod package, or try again.").c_str(), L"Error", MB_OK | MB_ICONERROR);
            return 1;
        }
    }
    return 0;
}

// function to check integrity of the required archives
bool CheckModuleFile(const std::wstring& moduleFileName, const LaunchConfig& config)
{
    std::wifstream moduleFile(moduleFileName);
    if (!moduleFile.is_open())
    {
        MessageBox(NULL, (L"Failed to find or open the mod's " + moduleFileName + L" module file. Reacquire it from the mod package, or try again.").c_str(), L"Error", MB_OK | MB_ICONERROR);
        return false;
    }

    std::wstring line;
    std::wregex archiveRegex(L"archive\\.\\d{2} = (.+\\.sga)");
    std::wsmatch match;
    wchar_t launcherPath[MAX_PATH];
    GetModuleFileName(NULL, launcherPath, MAX_PATH);
    std::wstring rootDir = std::wstring(launcherPath).substr(0, std::wstring(launcherPath).find_last_of(L"\\/"));

    std::set<std::wstring> localeFoldersWithUcs;
    std::wregex localeRegex(L"^GameAssets\\\\Locale\\\\([^\\\\]+)\\\\");
    std::wsmatch localeMatch;

    // first pass to detect language folders with DOW2.ucs
    while (std::getline(moduleFile, line))
    {
        if (std::regex_search(line, match, archiveRegex))
        {
            std::wstring relativePath = match[1].str();
            std::wstring fullPath = rootDir + L"\\" + relativePath;

            if (std::regex_search(relativePath, localeMatch, localeRegex))
            {
                std::wstring localeFolder = rootDir + L"\\GameAssets\\Locale\\" + localeMatch[1].str();
                std::wstring ucsFile = localeFolder + L"\\DOW2.ucs";

                if (GetFileAttributes(ucsFile.c_str()) != INVALID_FILE_ATTRIBUTES)
                {
                    localeFoldersWithUcs.insert(localeFolder);
                }
            }
        }
    }

    bool skipLocaleSgaChecks = (localeFoldersWithUcs.size() > 1);

    if (config.VerboseDebug)
    {
        if (skipLocaleSgaChecks)
        {
            MessageBox(NULL, L"Multiple localization folders containing DOW2.ucs detected. Assuming development build. Ignoring .sga checks in Locale subfolders.", L"Debug", MB_OK | MB_ICONINFORMATION);
        }
        else
        {
            MessageBox(NULL, L"Only one localization folder containing DOW2.ucs detected. Assuming not development build. Proceeding with .sga checks in Locale subfolders.", L"Debug", MB_OK | MB_ICONINFORMATION);
        }
    }

    // reset file reading position for the second pass
    moduleFile.clear();
    moduleFile.seekg(0, std::ios::beg);

    // second pass to perform .sga checks
    while (std::getline(moduleFile, line))
    {
        if (std::regex_search(line, match, archiveRegex))
        {
            std::wstring relativePath = match[1].str();
            std::wstring fullPath = rootDir + L"\\" + relativePath;

            if (std::regex_search(relativePath, localeMatch, localeRegex))
            {
                if (skipLocaleSgaChecks)
                {
                    continue; // skip .sga file checks in Locale subfolders
                }
                else
                {
                    std::wstring localeFolder = rootDir + L"\\GameAssets\\Locale\\" + localeMatch[1].str();
                    std::wstring ucsFile = localeFolder + L"\\DOW2.ucs";

                    if (GetFileAttributes(localeFolder.c_str()) == INVALID_FILE_ATTRIBUTES ||
                        GetFileAttributes(ucsFile.c_str()) == INVALID_FILE_ATTRIBUTES)
                    {
                        // Locale folder or DOW2.ucs file does not exist, we skip this .sga file check
                        continue;
                    }
                }
            }

            if (GetFileAttributes(fullPath.c_str()) == INVALID_FILE_ATTRIBUTES)
            {
                MessageBox(NULL, (L"Missing archive " + fullPath + L" required by the mod. Reacquire it from the mod package, or try again.").c_str(), L"Error", MB_OK | MB_ICONERROR);
                return false;
            }
        }
    }

    return true;
}

int main()
{
    HINSTANCE hInstance = GetModuleHandle(NULL);
    bool consoleShown = false;

    // set the console control handler
    SetConsoleCtrlHandler(ConsoleHandler, TRUE);

    // get the launcher executable name
    wchar_t launcherPath[MAX_PATH];
    GetModuleFileName(NULL, launcherPath, MAX_PATH);

    std::wstring launcherName = launcherPath;
    size_t lastSlashPos = launcherName.find_last_of(L"\\/");
    if (lastSlashPos != std::wstring::npos)
    {
        launcherName = launcherName.substr(lastSlashPos + 1);
    }

    // derive the bitmap and config file names from the launcher executable name
    std::wstring bitmapFileName = std::wstring(launcherName).substr(0, launcherName.find_last_of(L".")) + L".bmp";
    std::wstring moduleFileName = std::wstring(launcherName).substr(0, launcherName.find_last_of(L".")) + L".module";
    std::wstring configFileName = std::wstring(launcherName).substr(0, launcherName.find_last_of(L".")) + L".config";
    std::wstring modName = std::wstring(launcherName).substr(0, launcherName.find_last_of(L"."));

    // check if the bitmap file exists
    if (GetFileAttributes(bitmapFileName.c_str()) == INVALID_FILE_ATTRIBUTES)
    {
        // show the console window if the bitmap file does not exist
        HWND consoleWnd = GetConsoleWindow();
        ShowWindow(consoleWnd, SW_SHOW);
        consoleShown = true;
    }
    else
    {
        // hide the console window if the bitmap file exists
        HWND consoleWnd = GetConsoleWindow();
        ShowWindow(consoleWnd, SW_HIDE);
    }

    // redirect stdout to the console
    if (consoleShown) {
        FILE* stream;
        _wfreopen_s(&stream, L"CONOUT$", L"w", stdout);
        _wfreopen_s(&stream, L"CONOUT$", L"w", stderr);
    }

    CONSOLE_MESSAGE(L"Launcher executed.");
    CONSOLE_MESSAGE(L"Verifying files...");

    // check if DOW2.exe exists in the same directory as the launcher
    if (GetFileAttributes(APP_NAME) == INVALID_FILE_ATTRIBUTES)
    {
        MessageBox(NULL, L"Failed to find or open DOW2.exe. You have installed the mod into the wrong directory, or your game is missing or corrupt. Install the mod into the correct directory, or try again.", L"Error", MB_OK | MB_ICONERROR);
        return 1;
    }

    CONSOLE_MESSAGE(L"DOW2 check.");

    // check if the .module file exists in the same directory
    if (GetFileAttributes(moduleFileName.c_str()) == INVALID_FILE_ATTRIBUTES)
    {
        MessageBox(NULL, (L"Failed to find or open the mod's " + moduleFileName + L" module file. Reacquire it from the mod package, or try again.").c_str(), L"Error", MB_OK | MB_ICONERROR);
        return 1;
    }

    CONSOLE_MESSAGE(L"Module check.");

    // read launch parameters from the .launchconfig file
    LaunchConfig config = ReadLaunchConfig();

    if (config.VerboseDebug) {
        MessageBox(NULL, L"Launcher initialized. Attempting to read the configuration file.", L"Debug", MB_OK | MB_ICONINFORMATION);
        MessageBox(NULL, (L"Launcher name: " + launcherName).c_str(), L"Debug", MB_OK | MB_ICONINFORMATION);
        MessageBox(NULL, (L"Bitmap file name: " + bitmapFileName).c_str(), L"Debug", MB_OK | MB_ICONINFORMATION);
        MessageBox(NULL, (L"Module file name: " + moduleFileName).c_str(), L"Debug", MB_OK | MB_ICONINFORMATION);
        MessageBox(NULL, (L"Config file name: " + configFileName).c_str(), L"Debug", MB_OK | MB_ICONINFORMATION);
        MessageBox(NULL, (L"Mod name: " + modName).c_str(), L"Debug", MB_OK | MB_ICONINFORMATION);
    }

    CONSOLE_MESSAGE(L"Launch configuration check.");

    if (config.VerboseDebug) {
        MessageBox(NULL, L"Configuration file read successfully.", L"Debug", MB_OK | MB_ICONINFORMATION);
    }

    if (config.FirstTimeLaunchCheck)
    {
        MessageBox(NULL, config.FirstTimeLaunchMessage.c_str(), L"First Launch", MB_OK | MB_ICONINFORMATION);
        if (config.IsDXVK)
        {
            MessageBox(NULL, L"Beware that DXVK is required for this mod, meaning that the game will use Vulkan instead of DirectX. Some hardware configurations do not support Vulkan, so if your game is inexplicably failing to launch, or you receive any errors regarding your graphical configuration, you may remove the d3d9.dll and dxvk.conf files associated with DXVK, but this will hinder your gameplay experience.", L"Information", MB_OK | MB_ICONINFORMATION);
        }

        config.FirstTimeLaunchCheck = false;
        WriteLaunchConfig(config);
    }

    if (!CheckModuleFile(moduleFileName, config))
    {
        // error message and exit are handled within the CheckModuleFile function
        return 1;
    }

    CONSOLE_MESSAGE(L"Module contents check.");

    // check for ChaosRisingGDF.dll if IsRetribution is true
    if (config.IsRetribution && GetFileAttributes(L"ChaosRisingGDF.dll") != INVALID_FILE_ATTRIBUTES)
    {
        MessageBox(NULL, L"Found ChaosRisingGDF.dll; this may be Dawn of War II - Chaos Rising, but this mod is for Dawn of War II - Retribution. Install the mod to Dawn of War II - Retribution.", L"Error", MB_OK | MB_ICONERROR);
        return 1;
    }

    // check for CGalaxy.dll if IsSteam is true
    if (config.IsSteam && GetFileAttributes(L"CGalaxy.dll") != INVALID_FILE_ATTRIBUTES)
    {
        MessageBox(NULL, L"Found CGalaxy.dll; this may be a GOG distribution of the game, but this version of the mod is designed for the Steam distribution of the game. Install the mod to the Steam version of the game.", L"Error", MB_OK | MB_ICONERROR);
        return 1;
    }

    if (!config.IsRetribution && GetFileAttributes(L"ChaosRisingGDF.dll") == INVALID_FILE_ATTRIBUTES)
    {
        MessageBox(NULL, L"The ChaosRisingGDF.dll file is missing; this mod is designed for Dawn of War II - Chaos Rising. Install the mod to Dawn of War II - Chaos Rising.", L"Error", MB_OK | MB_ICONERROR);
        return 1;
    }

    // check for CGalaxy.dll if IsSteam is false and file does not exist
    if (!config.IsSteam && GetFileAttributes(L"CGalaxy.dll") == INVALID_FILE_ATTRIBUTES)
    {
        MessageBox(NULL, L"The CGalaxy.dll file is missing; this mod is designed for the GOG distribution of the game. Install the mod to the GOG distribution of the game.", L"Error", MB_OK | MB_ICONERROR);
        return 1;
    }

    // check for dxvk
    if (config.IsDXVK)
    {
        bool d3d9Missing = false;
        bool dxvkConfMissing = false;

        if (GetFileAttributes(L"d3d9.dll") == INVALID_FILE_ATTRIBUTES) {
            d3d9Missing = true;
        }

        if (GetFileAttributes(L"dxvk.conf") == INVALID_FILE_ATTRIBUTES) {
            dxvkConfMissing = true;
        }

        if (d3d9Missing)
        {
            int msgboxID = MessageBox(NULL, L"This mod requires DXVK, but the d3d9.dll file is missing. While you can still proceed to launch the mod, you will crash in large scenarios, and experience a loss in performance. Would you like to recover DXVK?", L"Warning", MB_YESNO | MB_ICONWARNING);
            if (msgboxID == IDYES)
            {
                CopyFileRaw(L"d3d9.bin", L"d3d9.dll");
                if (dxvkConfMissing)
                {
                    std::ofstream dxvkConfFile("dxvk.conf");
                    dxvkConfFile << "dxgi.maxFrameRate = 60\n";
                    dxvkConfFile << "d3d9.maxFrameRate = 60\n";
                    dxvkConfFile.close();
                }
            }
        }
        else
        {
            auto versionStrings = GetFileVersionStrings(L"d3d9.dll", config.VerboseDebug);

            if (config.VerboseDebug) {
                std::wstring debugMessage = L"d3d9.dll version info:\n";
                for (const auto& pair : versionStrings) {
                    debugMessage += pair.first + L": " + pair.second + L"\n";
                }
                MessageBox(NULL, debugMessage.c_str(), L"Debug", MB_OK | MB_ICONINFORMATION);
            }

            if (versionStrings.find(L"ProductName") == versionStrings.end() || versionStrings[L"ProductName"] != L"DXVK") {
                CopyFileRaw(L"d3d9.bin", L"d3d9.dll");
            }
            else if (dxvkConfMissing)
            {
                int msgboxID = MessageBox(NULL, L"The dxvk.conf file for DXVK is missing. Would you like to recover it?", L"Information", MB_YESNO | MB_ICONQUESTION);
                if (msgboxID == IDYES)
                {
                    std::ofstream dxvkConfFile("dxvk.conf");
                    dxvkConfFile << "dxgi.maxFrameRate = 60\n";
                    dxvkConfFile << "d3d9.maxFrameRate = 60\n";
                    dxvkConfFile.close();
                }
            }
        }
    }
    else
    {
        auto versionStrings = GetFileVersionStrings(L"d3d9.dll", config.VerboseDebug);

        if (config.VerboseDebug) {
            std::wstring debugMessage = L"d3d9.dll version info:\n";
            for (const auto& pair : versionStrings) {
                debugMessage += pair.first + L": " + pair.second + L"\n";
            }
            MessageBox(NULL, debugMessage.c_str(), L"Debug", MB_OK | MB_ICONINFORMATION);
        }

        if (versionStrings.find(L"ProductName") != versionStrings.end() && versionStrings[L"ProductName"] == L"DXVK") {
            int msgboxID = MessageBox(NULL, L"You have DXVK installed, but this mod does not require it. Would you like to remove DXVK?", L"Information", MB_YESNO | MB_ICONQUESTION);
            if (msgboxID == IDYES)
            {
                DeleteFile(L"d3d9.dll");
                DeleteFile(L"dxvk.conf");
            }
        }
    }

    // check for injector
    if (config.Injector)
    {
        if (GetFileAttributes(config.InjectorFileName.c_str()) == INVALID_FILE_ATTRIBUTES)
        {
            std::wstring binFileName = config.InjectorFileName.substr(0, config.InjectorFileName.find_last_of(L".")) + L".bin";
            if (!CopyFileRaw(binFileName, config.InjectorFileName))
            {
                MessageBox(NULL, (L"Failed to create the necessary injector version of the " + binFileName + L" file. Reacquire it from the mod package, or try again.").c_str(), L"Error", MB_OK | MB_ICONERROR);
                return 1;
            }
        }
        else
        {
            // start the binary processing thread to check and possibly replace the injector file
            HANDLE hThread = CreateThread(NULL, 0, BinaryProcessingThread, &config, 0, NULL);
            if (hThread == NULL)
            {
                MessageBox(NULL, L"Failed to create the binary processing thread.", L"Error", MB_OK | MB_ICONERROR);
                return 1;
            }

            WaitForSingleObject(hThread, INFINITE);
            CloseHandle(hThread);
        }
    }

    if (config.VerboseDebug) {
        MessageBox(NULL, L"File verification complete. Preparing to launch the game.", L"Debug", MB_OK | MB_ICONINFORMATION);
    }

    // check if DOW2.exe is already running
    if (IsProcessRunning(APP_NAME))
    {
        MessageBox(NULL, L"DOW2.exe is already running. Please close it before launching again.", L"Error", MB_OK | MB_ICONERROR);
        return 1;
    }

    // display the bitmap
    HWND hwndBitmap = ShowBitmap(hInstance, bitmapFileName, config.BitmapWidth, config.BitmapHeight);

    // start the global timeout timer
    DWORD64 startTime = GetTickCount64();

    // binary processing with timeout
    HANDLE hEvent = NULL;
    HANDLE hThread = NULL;

    if (config.Injector)
    {
        hEvent = CreateEvent(NULL, TRUE, FALSE, NULL); // manual-reset event
        if (hEvent == NULL)
        {
            if (hwndBitmap)
            {
                DestroyWindow(hwndBitmap);
            }
            return 1;
        }

        hThread = CreateThread(NULL, 0, BinaryProcessingThread, &config, 0, NULL);
        if (hThread == NULL)
        {
            CloseHandle(hEvent);
            if (hwndBitmap)
            {
                DestroyWindow(hwndBitmap);
            }
            return 1;
        }

        // check if the .config file exists in the same directory
        if (GetFileAttributes(configFileName.c_str()) == INVALID_FILE_ATTRIBUTES)
        {
            MessageBox(NULL, (L"Failed to find or open the mod's " + configFileName + L" injector config file. Reacquire it from the mod package, or try again.").c_str(), L"Error", MB_OK | MB_ICONERROR);
            return 1;
        }

        HANDLE handles[] = { hThread, hEvent };
        DWORD result = WaitForMultipleObjects(2, handles, FALSE, TIMEOUT_PROCESS);

        if (result == WAIT_TIMEOUT)
        {
            MessageBox(NULL, L"Injector processing or replacement timed out before it could complete.", L"Error", MB_OK | MB_ICONERROR);
            SetEvent(hEvent); // signal the event to exit the thread
            WaitForSingleObject(hThread, INFINITE); // wait for the thread to exit gracefully
            CloseHandle(hEvent);
            CloseHandle(hThread);
            if (hwndBitmap)
            {
                DestroyWindow(hwndBitmap);
            }
            return 1;
        }

        DWORD threadExitCode;
        if (!GetExitCodeThread(hThread, &threadExitCode) || threadExitCode != 0)
        {
            CloseHandle(hEvent);
            CloseHandle(hThread);
            if (hwndBitmap)
            {
                DestroyWindow(hwndBitmap);
            }
            return 1;
        }

        CloseHandle(hEvent);
        CloseHandle(hThread);
    }

    // check the global timeout
    if (GetTickCount64() - startTime > TIMEOUT_PROCESS)
    {
        MessageBox(NULL, L"Launcher process timed out before all operations could complete.", L"Error", MB_OK | MB_ICONERROR);
        if (hwndBitmap)
        {
            DestroyWindow(hwndBitmap);
        }
        return 1;
    }

    // launch the game
    std::wstring commandLine = std::wstring(APP_NAME) + L" -modname " + modName + L" " + config.LaunchParams;

    STARTUPINFO si = { sizeof(si) };
    PROCESS_INFORMATION pi;

    if (!CreateProcess(NULL, &commandLine[0], NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi))
    {
        MessageBox(NULL, L"Failed to find or open DOW2.exe. You have installed the mod into the wrong directory, or your game is missing or corrupt. Install the mod into the correct game directory, or try again.", L"Error", MB_OK | MB_ICONERROR);
        if (hwndBitmap)
        {
            DestroyWindow(hwndBitmap);
        }
        return 1;
    }

    CONSOLE_MESSAGE(L"DOW2.exe launching...");

    // start the monitoring thread after a time
    Sleep(15000);
    std::thread monitorThread(MonitorAndSetPriority);
    monitorThread.detach();

    // wait for a time before closing the launcher
    Sleep(30000);

    // destroy the bitmap window after waiting
    if (hwndBitmap)
    {
        DestroyWindow(hwndBitmap);
    }

    // close the process and thread handles
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    return 0;
}