#define APP_NAME L"DOW2.exe"

#include "framework.h"

// structure to hold the launch configuration
struct LaunchConfig
{
    bool Injector = false;
    std::string ExpectedInjectorFileMD5Checksum;
    std::string ExpectedXThreadFileMD5Checksum;
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
    bool IsUnsafe = false;
    bool Console = false;
    std::vector<std::wstring> InjectedFiles;
    std::vector<std::wstring> InjectedConfigurations;
    std::wstring GameVersion;
    bool LAAPatch = false;
    bool UIWarnings = false;
};

// function to validate the presence of necessary injector files
bool InjectedFilesPresent(const std::wstring& folderPath, const std::vector<std::wstring>& injectedFiles)
{
    if (GetFileAttributes(folderPath.c_str()) == INVALID_FILE_ATTRIBUTES)
    {
        MessageBox(NULL, (L"Failed to find the Injector mod folder. Try again, or reacquire it from the mod package: " + folderPath).c_str(), L"Error", MB_OK | MB_ICONERROR);
        return false;
    }

    for (const auto& fileName : injectedFiles)
    {
        std::wstring filePath = folderPath + L"\\" + fileName;

        if (GetFileAttributes(filePath.c_str()) == INVALID_FILE_ATTRIBUTES)
        {
            MessageBox(NULL, (L"Failed to find a specific injected file required by the mod. Try again, or reacquire it from the mod package: " + fileName).c_str(), L"Error", MB_OK | MB_ICONERROR);
            return false;
        }
    }
    return true;
}

// function to validate the presence of configuration files for specific injections
bool InjectedConfigurationsPresent(const std::wstring& launcherName, const std::vector<std::wstring>& injectedConfigurations)
{
    for (const auto& ext : injectedConfigurations)
    {
        std::wstring filePath = launcherName + ext;

        if (GetFileAttributes(filePath.c_str()) == INVALID_FILE_ATTRIBUTES)
        {
            MessageBox(NULL, (L"Failed to find a specific injection configuration file required by the mod. Try again, or reacquire it from the mod package: " + filePath).c_str(), L"Error", MB_OK | MB_ICONERROR);
            return false;
        }
    }
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
    std::set<std::wstring> requiredKeys = 
    {
        L"Injector", L"ExpectedInjectorFileMD5Checksum", L"ExpectedXThreadFileMD5Checksum", L"BitmapWidth", L"BitmapHeight", L"InjectorFileName", L"LaunchParams", L"IsRetribution", L"IsSteam", L"VerboseDebug", L"IsDXVK", L"FirstTimeLaunchCheck", L"FirstTimeLaunchMessage", L"IsUnsafe", L"Console", L"InjectedFiles", L"InjectedConfigurations", L"GameVersion", L"LAAPatch", L"UIWarnings"
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
            if (config.Injector)
            {
                config.ExpectedInjectorFileMD5Checksum = WStringToString(value);
            }
        }
        else if (key == L"ExpectedXThreadFileMD5Checksum")
        {
            if (!config.Injector)
            {
                config.ExpectedInjectorFileMD5Checksum = WStringToString(value);
            }
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
            if (config.Injector)
            {
                config.InjectorFileName = value;
                if (!ValidateFileName(config.InjectorFileName))
                {
                    MessageBox(NULL, L"Invalid formatting for the [InjectorFileName] field of the launch configuration file. The full file name must include the file extension.", L"Error", MB_OK | MB_ICONERROR);
                    exit(1);
                }
            }
        }
        else if (key == L"LaunchParams")
        {
            config.LaunchParams = value;
            if (!ValidateLaunchParams(config.LaunchParams))
            {
                MessageBox(NULL, L"Invalid formatting for the [LaunchParams] field of the launch configuration file. Each parameter must start with a [-] sign, and be separated by a space.", L"Error", MB_OK | MB_ICONERROR);
                exit(1);
            }
            if (config.LaunchParams.find(L"-modname") != std::wstring::npos)
            {
                MessageBox(NULL, L"The [LaunchParams] field of the launch configuration file contains the -modname parameter, which is automatically applied with the appropriate argument for the mod based on the name of the launcher. Remove -modname from the launch configuration file.", L"Error", MB_OK | MB_ICONERROR);
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
        else if (key == L"IsUnsafe")
        {
            if (!ValidateBooleanField(value))
            {
                MessageBox(NULL, L"Invalid formatting for the [IsUnsafe] field of the launch configuration file. It must be true or false.", L"Error", MB_OK | MB_ICONERROR);
                exit(1);
            }
            config.IsUnsafe = (value == L"true");
        }
        else if (key == L"Console")
        {
            if (!ValidateBooleanField(value))
            {
                MessageBox(NULL, L"Invalid formatting for the [Console] field of the launch configuration file. It must be true or false.", L"Error", MB_OK | MB_ICONERROR);
                exit(1);
            }
            config.Console = (value == L"true");
        }
        else if (key == L"InjectedFiles")
        {
            if (config.Injector)
            {
                if (!value.empty())
                {
                    std::wistringstream ss(value);
                    std::wstring token;
                    size_t startPos = 0, endPos;
                    while ((endPos = value.find(L", ", startPos)) != std::wstring::npos)
                    {
                        token = value.substr(startPos, endPos - startPos);
                        token = TrimWString(token);
                        if (token.find(L".dll") == std::wstring::npos)
                        {
                            MessageBox(NULL, L"Invalid formatting for the [InjectedFiles] field of the launch configuration file. Each full file name must include the DLL file extension.", L"Error", MB_OK | MB_ICONERROR);
                            exit(1);
                        }
                        if (value[endPos + 1] != L' ')
                        {
                            MessageBox(NULL, L"Invalid formatting for the [InjectedFiles] field of the launch configuration file. Each entry must be separated by a comma and a space.", L"Error", MB_OK | MB_ICONERROR);
                            exit(1);
                        }
                        config.InjectedFiles.push_back(token);
                        startPos = endPos + 2;
                    }
                    token = value.substr(startPos);
                    token = TrimWString(token);
                    if (token.find(L".dll") == std::wstring::npos)
                    {
                        MessageBox(NULL, L"Invalid formatting for the [InjectedFiles] field of the launch configuration file. Each full file name must include the DLL file extension.", L"Error", MB_OK | MB_ICONERROR);
                        exit(1);
                    }
                    config.InjectedFiles.push_back(token);
                }
            }
            else
            {
                config.InjectedFiles.push_back(value);
            }
        }
        else if (key == L"InjectedConfigurations")
        {
            if (config.Injector)
            {
                if (!value.empty())
                {
                    std::wistringstream ss(value);
                    std::wstring token;
                    size_t startPos = 0, endPos;
                    while ((endPos = value.find(L", ", startPos)) != std::wstring::npos)
                    {
                        token = value.substr(startPos, endPos - startPos);
                        token = TrimWString(token);
                        if (token.find(L'.') == std::wstring::npos)
                        {
                            MessageBox(NULL, L"Invalid formatting for the [InjectedConfigurations] field of the launch configuration file. Each entry must be a valid file extension for injection configuration file types used by the mod.", L"Error", MB_OK | MB_ICONERROR);
                            exit(1);
                        }
                        if (value[endPos + 1] != L' ')
                        {
                            MessageBox(NULL, L"Invalid formatting for the [InjectedConfigurations] field of the launch configuration file. Each entry must be separated by a comma and a space.", L"Error", MB_OK | MB_ICONERROR);
                            exit(1);
                        }
                        config.InjectedConfigurations.push_back(token);
                        startPos = endPos + 2;
                    }
                    token = value.substr(startPos);
                    token = TrimWString(token);
                    if (token.find(L'.') == std::wstring::npos)
                    {
                        MessageBox(NULL, L"Invalid formatting for the [InjectedConfigurations] field of the launch configuration file. Each entry must be a valid file extension for injection configuration file types used by the mod.", L"Error", MB_OK | MB_ICONERROR);
                        exit(1);
                    }
                    config.InjectedConfigurations.push_back(token);
                }
            }
            else
            {
                config.InjectedConfigurations.push_back(value);
            }
        }
        else if (key == L"FirstTimeLaunchMessage")
        {
            config.FirstTimeLaunchMessage = value;
        }
        else if (key == L"GameVersion")
        {
            config.GameVersion = value;
        }
        else if (key == L"LAAPatch")
        {
            if (!ValidateBooleanField(value))
            {
                MessageBox(NULL, L"Invalid formatting for the [LAAPatch] field of the launch configuration file. It must be true or false.", L"Error", MB_OK | MB_ICONERROR);
                exit(1);
            }
            config.LAAPatch = (value == L"true");
        }
        else if (key == L"UIWarnings")
        {
            if (!ValidateBooleanField(value))
            {
                MessageBox(NULL, L"Invalid formatting for the [UIWarnings] field of the launch configuration file. It must be true or false.", L"Error", MB_OK | MB_ICONERROR);
                exit(1);
            }
            config.UIWarnings = (value == L"true");
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
            errorMsg += key;
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
    configFile << L"GameVersion=" << config.GameVersion << L"\n";
    configFile << L"IsDXVK=" << (config.IsDXVK ? L"true" : L"false") << L"\n";
    configFile << L"LAAPatch=" << (config.LAAPatch ? L"true" : L"false") << L"\n";
    configFile << L"UIWarnings=" << (config.UIWarnings ? L"true" : L"false") << L"\n";
    configFile << L"LaunchParams=" << config.LaunchParams << L"\n";
    configFile << L"BitmapWidth=" << config.BitmapWidth << L"\n";
    configFile << L"BitmapHeight=" << config.BitmapHeight << L"\n";
    configFile << L"Injector=" << (config.Injector ? L"true" : L"false") << L"\n";
    configFile << L"InjectorFileName=" << config.InjectorFileName << L"\n";
    configFile << L"InjectedFiles=";
    for (size_t i = 0; i < config.InjectedFiles.size(); ++i)
    {
        configFile << config.InjectedFiles[i];
        if (i < config.InjectedFiles.size() - 1)
        {
            configFile << L", ";
        }
    }
    configFile << L"\n";
    configFile << L"InjectedConfigurations=";
    for (size_t i = 0; i < config.InjectedConfigurations.size(); ++i)
    {
        configFile << config.InjectedConfigurations[i];
        if (i < config.InjectedConfigurations.size() - 1)
        {
            configFile << L", ";
        }
    }
    configFile << L"\n";
    configFile << L"ExpectedInjectorFileMD5Checksum=" << std::wstring(config.ExpectedInjectorFileMD5Checksum.begin(), config.ExpectedInjectorFileMD5Checksum.end()) << L"\n";
    configFile << L"ExpectedXThreadFileMD5Checksum=" << std::wstring(config.ExpectedXThreadFileMD5Checksum.begin(), config.ExpectedXThreadFileMD5Checksum.end()) << L"\n";
    configFile << L"FirstTimeLaunchCheck=" << (config.FirstTimeLaunchCheck ? L"true" : L"false") << L"\n";
    configFile << L"FirstTimeLaunchMessage=" << config.FirstTimeLaunchMessage << L"\n";
    configFile << L"VerboseDebug=" << (config.VerboseDebug ? L"true" : L"false") << L"\n";
    configFile << L"IsUnsafe=" << (config.IsUnsafe ? L"true" : L"false") << L"\n";
    configFile << L"Console=" << (config.Console ? L"true" : L"false") << L"\n";

    configFile.close();
}

// function to read the injector mod folder
std::wstring ReadModFolderFromConfig(const std::wstring& configFilePath)
{
    std::wifstream configFile(configFilePath);
    if (!configFile.is_open())
    {
        MessageBox(NULL, (L"Failed to find or open the mod's " + configFilePath + L" injector config file. Reacquire it from the mod package, or try again.").c_str(), L"Error", MB_OK | MB_ICONERROR);
        exit(1);
    }

    std::wstring line;
    std::wstring modFolder;
    while (std::getline(configFile, line))
    {
        size_t pos = line.find(L":");
        if (pos == std::wstring::npos) continue;

        std::wstring key = line.substr(0, pos);
        std::wstring value = line.substr(pos + 1);

        if (key == L"mod-folder")
        {
            modFolder = TrimWString(value);
            break;
        }
    }
    configFile.close();

    return modFolder;
}

// function to handle the binary processing with a timeout
DWORD WINAPI InjectorBinaryProcessingThread(LPVOID lpParam)
{
    LaunchConfig* config = (LaunchConfig*)lpParam;
    std::string actualChecksum;

    if (!CalculateMD5(config->InjectorFileName.c_str(), actualChecksum) || actualChecksum != config->ExpectedInjectorFileMD5Checksum)
    {
        if (config->VerboseDebug) 
        {
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

// function to process individual UCS files
bool ProcessUCSFile(const std::wstring& filePath, const LaunchConfig& config)
{
    std::ifstream ucsFile(filePath, std::ios::binary);

    if (!ucsFile.is_open())
    {
        MessageBox(NULL, (L"Failed to open UCS file. Reacquire it from the mod package: " + filePath).c_str(), L"Error", MB_OK | MB_ICONERROR);
        return false;
    }

    // read entire file content into a string
    std::string fileContent((std::istreambuf_iterator<char>(ucsFile)), std::istreambuf_iterator<char>());
    ucsFile.close();

    std::wstring wFileContent;
    bool conversionNeeded = false;

    try
    {
        if (fileContent.size() > 2 && (unsigned char)fileContent[0] == 0xFF && (unsigned char)fileContent[1] == 0xFE)
        {
            // UTF-16 LE BOM detected
            wFileContent.assign(reinterpret_cast<const wchar_t*>(fileContent.data() + 2), (fileContent.size() - 2) / sizeof(wchar_t));
        }
        else if (fileContent.size() > 2 && (unsigned char)fileContent[0] == 0xEF && (unsigned char)fileContent[1] == 0xBB && (unsigned char)fileContent[2] == 0xBF)
        {
            // UTF-8 BOM detected
            std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> conv;
            wFileContent = conv.from_bytes(fileContent.substr(3));
            conversionNeeded = true;
        }
        else
        {
            // no BOM or unknown BOM, assume UTF-8 without BOM
            std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> conv;
            wFileContent = conv.from_bytes(fileContent);
            conversionNeeded = true;
        }
    }
    catch (const std::exception& e)
    {
        std::wstring errorMessage = L"Failed to convert UCS file to UTF-16 LE. Try again, or reacquire it from the mod package: " + filePath + L"\nException: " + std::wstring(e.what(), e.what() + strlen(e.what()));
        MessageBox(NULL, errorMessage.c_str(), L"Error", MB_OK | MB_ICONERROR);
        return false;
    }

    if (conversionNeeded)
    {
        if (config.VerboseDebug)
        {
            MessageBox(NULL, (L"This UCS file is not UTF-16 LE: " + filePath + L", attempting conversion.").c_str(), L"Debug", MB_OK | MB_ICONINFORMATION);
        }

        // convert to UTF-16 LE and write back to the file
        std::ofstream outFile(filePath, std::ios::binary | std::ios::trunc);
        if (!outFile.is_open())
        {
            MessageBox(NULL, (L"Failed to find or open UCS file for writing. Try again, or reacquire it from the mod package: " + filePath).c_str(), L"Error", MB_OK | MB_ICONERROR);
            return false;
        }

        // write BOM for UTF-16 LE
        char16_t bom = 0xFEFF;
        outFile.write(reinterpret_cast<const char*>(&bom), sizeof(bom));

        // write the converted content
        outFile.write(reinterpret_cast<const char*>(wFileContent.data()), wFileContent.size() * sizeof(wchar_t));
        outFile.close();
    }

    // re-read the file to ensure it's now UTF-16 LE
    ucsFile.open(filePath, std::ios::binary);
    if (!ucsFile.is_open())
    {
        MessageBox(NULL, (L"Failed to find or open faulty UCS file to confirm attempted conversion to UTF-16 LE. Try again, or reacquire it from the mod package: " + filePath).c_str(), L"Error", MB_OK | MB_ICONERROR);
        return false;
    }

    std::string newFileContent((std::istreambuf_iterator<char>(ucsFile)), std::istreambuf_iterator<char>());
    ucsFile.close();

    if (!(newFileContent.size() > 2 && (unsigned char)newFileContent[0] == 0xFF && (unsigned char)newFileContent[1] == 0xFE))
    {
        std::wstring errorMessage = L"UCS file could not be verified as UTF-16 LE after attempted conversion: " + filePath;
        MessageBox(NULL, errorMessage.c_str(), L"Error", MB_OK | MB_ICONERROR);
        exit(EXIT_FAILURE);
    }

    std::wstringstream wss(wFileContent);
    std::unordered_map<unsigned long, std::vector<int>> numberLineMap;
    std::wstring line;
    unsigned long currentNumber = 0;
    int lineNumber = 0;

    while (std::getline(wss, line))
    {
        lineNumber++;
        // trim any leading or trailing whitespace
        line.erase(0, line.find_first_not_of(L" \t"));
        line.erase(line.find_last_not_of(L" \t") + 1);

        // skip empty lines
        if (line.empty())
        {
            continue;
        }

        size_t firstNonDigitPos = line.find_first_not_of(L"0123456789");

        // if no digits found at the start of the line
        if (firstNonDigitPos == 0)
        {
            std::wstring errorMessage = L"Not a number entry in UCS file: " + filePath + L"\nLine: " + std::to_wstring(lineNumber) + L": " + line;
            MessageBox(NULL, errorMessage.c_str(), L"Error", MB_OK | MB_ICONERROR);
            return false;
        }

        // if the number part is found, extract it
        std::wstring numberPart = line.substr(0, firstNonDigitPos);
        std::wstring textPart = line.substr(firstNonDigitPos);

        // ensure the number part is purely numeric
        if (!std::all_of(numberPart.begin(), numberPart.end(), iswdigit))
        {
            std::wstring errorMessage = L"Not a number entry in UCS file: " + filePath + L"\nLine: " + std::to_wstring(lineNumber) + L": " + line;
            MessageBox(NULL, errorMessage.c_str(), L"Error", MB_OK | MB_ICONERROR);
            return false;
        }

        try
        {
            currentNumber = std::stoul(numberPart);
        }
        catch (const std::exception& e)
        {
            std::wstring errorMessage = L"Failed to convert entry number for reading in UCS file: " + filePath + L"\nLine: " + std::to_wstring(lineNumber) + L": " + line + L"\nException: " + std::wstring(e.what(), e.what() + strlen(e.what()));
            MessageBox(NULL, errorMessage.c_str(), L"Error", MB_OK | MB_ICONERROR);
            return false;
        }

        numberLineMap[currentNumber].push_back(lineNumber);
    }

    return true;
}

// function to validate the formatting of ucs files
bool ValidateUCSFiles(const std::wstring& rootDir, const LaunchConfig& config)
{
    WIN32_FIND_DATA findFileData;
    std::wstring localeDir = rootDir + L"\\GameAssets\\Locale";
    HANDLE hFind = FindFirstFile((localeDir + L"\\*").c_str(), &findFileData);

    if (hFind == INVALID_HANDLE_VALUE)
    {
        MessageBox(NULL, L"Failed to find or open any locale directories. Verify your game cache and reacquire the necessary files from the mod package.", L"Error", MB_OK | MB_ICONERROR);
        return false;
    }
    
    do
    {
        if (findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {
            std::wstring subDir = findFileData.cFileName;
            if (subDir != L"." && subDir != L"..")
            {
                std::wstring fullPath = localeDir + L"\\" + subDir;
                WIN32_FIND_DATA fileFindData;
                HANDLE hFileFind = FindFirstFile((fullPath + L"\\*.ucs").c_str(), &fileFindData);

                if (hFileFind != INVALID_HANDLE_VALUE)
                {
                    do
                    {
                        std::wstring filePath = fullPath + L"\\" + fileFindData.cFileName;

                        // skip DOW2.ucs files
                        if (fileFindData.cFileName == std::wstring(L"DOW2.ucs"))
                        {
                            continue;
                        }

                        if (!ProcessUCSFile(filePath, config))
                        {
                            FindClose(hFileFind);
                            FindClose(hFind);
                            return false;
                        }
                    } 
                    while (FindNextFile(hFileFind, &fileFindData) != 0);

                    FindClose(hFileFind);
                }
            }
        }
    } 
    while (FindNextFile(hFind, &findFileData) != 0);

    FindClose(hFind);
    return true;
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

    // check if no files exist at all under GameAssets/Locale
    std::wstring localeRoot = rootDir + L"\\GameAssets\\Locale";
    WIN32_FIND_DATA findFileData;
    HANDLE hFind = FindFirstFile((localeRoot + L"\\*").c_str(), &findFileData);

    bool hasFiles = false;
    if (hFind != INVALID_HANDLE_VALUE)
    {
        do
        {
            if (!(findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
            {
                hasFiles = true;
                break;
            }
            else
            {
                std::wstring subfolderPath = localeRoot + L"\\" + findFileData.cFileName;
                HANDLE hSubFind = FindFirstFile((subfolderPath + L"\\*").c_str(), &findFileData);

                if (hSubFind != INVALID_HANDLE_VALUE)
                {
                    do
                    {
                        if (!(findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
                        {
                            hasFiles = true;
                            FindClose(hSubFind);
                            break;
                        }
                    } 
                    while (FindNextFile(hSubFind, &findFileData) != 0);
                    FindClose(hSubFind);

                    if (hasFiles)
                        break;
                }
            }
        }
        while (FindNextFile(hFind, &findFileData) != 0);
        FindClose(hFind);
    }

    if (!hasFiles)
    {
        MessageBox(NULL, L"No localization files were found under the GameAssets/Locale directory. Verify your game cache and reacquire the necessary files from the mod package.", L"Error", MB_OK | MB_ICONERROR);
        return false;
    }

    bool skipLocaleSgaChecks = (localeFoldersWithUcs.size() > 1);

    if (config.VerboseDebug)
    {
        if (skipLocaleSgaChecks)
        {
            MessageBox(NULL, L"Multiple localization folders or DOW2.ucs files detected. Assuming development build. Ignoring .sga checks in Locale subfolders.", L"Debug", MB_OK | MB_ICONINFORMATION);
        }
        else
        {
            MessageBox(NULL, L"Only one localization folder and DOW2.ucs file detected. Assuming not development build. Proceeding with .sga checks in Locale subfolders.", L"Debug", MB_OK | MB_ICONINFORMATION);
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

void CheckGameConfiguration(const LaunchConfig& config) 
{
    wchar_t* userProfilePath = nullptr;
    SHGetKnownFolderPath(FOLDERID_Documents, 0, NULL, &userProfilePath);

    std::wstring gameFolder = config.IsRetribution ? L"Dawn of War II - Retribution" : L"Dawn of War 2";
    std::wstring gameConfigFilePath = std::wstring(userProfilePath) + L"\\My Games\\" + gameFolder + L"\\Settings\\configuration.lua";

    std::wifstream configFile(gameConfigFilePath);
    if (!configFile) 
    {
        std::cerr << "Error opening the game's configuration file: " << WStringToString(gameConfigFilePath) << std::endl;
        return; // if file loading fails, simply return and continue the program
    }

    int screenWidth = 0;
    int screenHeight = 0;
    int uiScale = 100;

    std::wregex screenWidthRegex(L"setting = \"screenwidth\".*?value = (\\d+)");
    std::wregex screenHeightRegex(L"setting = \"screenheight\".*?value = (\\d+)");
    std::wregex uiScaleRegex(L"setting = \"uiscale\".*?value = (\\d+)");

    std::wsmatch match;
    std::wstring line;

    while (std::getline(configFile, line)) 
    {
        if (std::regex_search(line, match, screenWidthRegex)) 
        {
            screenWidth = std::stoi(match[1]);
        }
        else if (std::regex_search(line, match, screenHeightRegex)) 
        {
            screenHeight = std::stoi(match[1]);
        }
        else if (std::regex_search(line, match, uiScaleRegex)) 
        {
            uiScale = std::stoi(match[1]);
        }
    }

    configFile.close();

    if (screenWidth > 0 && screenHeight > 0) 
    {
        if (!CheckAspectRatio(screenWidth, screenHeight)) 
        {
            std::wstringstream warningMessage;
            warningMessage << L"This mod requires the game resolution to be set to a 16:9 aspect ratio in order for the UI to function correctly. 16:9 resolutions include any resolution marked in the game as Widescreen, such as 1280x720, 1920x1080, 2560x1440, or 3840x2160. If you are unable to change the resolution in the game, you instead change the screenWidth and screenHeight values in the following configuration file:" << gameConfigFilePath;
            MessageBox(NULL, warningMessage.str().c_str(), L"Warning", MB_OK | MB_ICONWARNING);
        }
    }

    if (uiScale != 100) 
    {
        std::wstringstream warningMessage;
        warningMessage << L"This mod requires the UI scale setting to be set to 100 in order for the UI to function correctly. Adjust the UI scale in the following configuration file: " << gameConfigFilePath;
        MessageBox(NULL, warningMessage.str().c_str(), L"Warning", MB_OK | MB_ICONWARNING);
    }
}

// main function
int main()
{
    std::string process_name = get_current_process_name();

    if (is_process_running(process_name)) 
    {
        std::cerr << "Another instance of the launcher is already running. Wait for it to close, or manually terminate it before attempting to launch again." << std::endl;
        std::cerr << "Press [Enter] to exit..." << std::endl;
        std::cin.get();
        return 1;
    }

    bool isWindows = false;
    bool isLinux = false;

    if (RunningOnWindows())
    {
        isWindows = true;
        isLinux = false;
    }

    if (RunningOnLinux())
    {
        std::cout << "Detected Linux. The launcher will proceed in Linux safe mode, though advanced launcher features and functionality will be limited." << std::endl;
        std::cerr << "Press [Enter] to continue..." << std::endl;
        std::cin.get();
        isLinux = true;
        isWindows = false;
    }

    if (isWindows)
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

        // define the root directory
        std::wstring rootDir = std::wstring(launcherPath).substr(0, std::wstring(launcherPath).find_last_of(L"\\/"));

        // read launch parameters from the .launchconfig file
        LaunchConfig config = ReadLaunchConfig();

        // display the bitmap
        HWND hwndBitmap = ShowBitmap(hInstance, bitmapFileName, config.BitmapWidth, config.BitmapHeight);

        // start the global timeout timer
        DWORD64 startTime = GetTickCount64();

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

        // check for console and bitmap
        if (GetFileAttributes(bitmapFileName.c_str()) == INVALID_FILE_ATTRIBUTES || config.Console)
        {
            // show the console window if the bitmap file does not exist or if console is true
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
        if (consoleShown)
        {
            FILE* stream;
            _wfreopen_s(&stream, L"CONOUT$", L"w", stdout);
            _wfreopen_s(&stream, L"CONOUT$", L"w", stderr);
        }

        if (config.VerboseDebug)
        {
            MessageBox(NULL, L"Verbose logging is enabled.", L"Debug", MB_OK | MB_ICONINFORMATION);
        }

        if (config.VerboseDebug && config.IsUnsafe) 
        {
            MessageBox(NULL, L"Unsafe mode is enabled.", L"Debug", MB_OK | MB_ICONINFORMATION);
        }

        CONSOLE_MESSAGE(L"Launcher initialized.");

        // START CHECKS
        if (!config.IsUnsafe)
        {
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

            if (config.VerboseDebug)
            {
                MessageBox(NULL, L"Verified first time launch.", L"Debug", MB_OK | MB_ICONINFORMATION);
            }

            // check if DOW2.exe is already running
            if (IsProcessRunning(APP_NAME))
            {
                MessageBox(NULL, L"DOW2.exe is already running. Please close it before launching again.", L"Error", MB_OK | MB_ICONERROR);
                return 1;
            }

            // check if DOW2.exe exists in the same directory as the launcher
            if (GetFileAttributes(APP_NAME) == INVALID_FILE_ATTRIBUTES)
            {
                MessageBox(NULL, L"Failed to find DOW2.exe. You have installed the mod into the wrong directory, or your game is missing or corrupt. Install the mod into the correct directory, or try again.", L"Error", MB_OK | MB_ICONERROR);
                return 1;
            }

            if (config.VerboseDebug)
            {
                MessageBox(NULL, L"Found DOW2.exe.", L"Debug", MB_OK | MB_ICONINFORMATION);
            }

            CONSOLE_MESSAGE(L"DOW2 check.");

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
                MessageBox(NULL, L"The ChaosRisingGDF.dll file is missing, but this mod is designed for Dawn of War II - Chaos Rising. Install the mod to Dawn of War II - Chaos Rising.", L"Error", MB_OK | MB_ICONERROR);
                return 1;
            }

            // check for CGalaxy.dll if IsSteam is false and file does not exist
            if (!config.IsSteam && GetFileAttributes(L"CGalaxy.dll") == INVALID_FILE_ATTRIBUTES)
            {
                MessageBox(NULL, L"The CGalaxy.dll file is missing, but this mod is designed for the GOG distribution of the game. Install the mod to the GOG distribution of the game.", L"Error", MB_OK | MB_ICONERROR);
                return 1;
            }

            // check GameVersion field entry against DOW2.exe file version
            if (!config.GameVersion.empty())
            {
                std::map<std::wstring, std::wstring> versionStrings = GetFileVersionStrings(APP_NAME, config.VerboseDebug);
                if (versionStrings[L"FileVersion"] != config.GameVersion) {
                    MessageBox(NULL, (L"File version of DOW2.exe does not match the supported version of the game. Your gameplay experience may be altered, or the mod may not work. Expected: " + config.GameVersion + L", Found: " + versionStrings[L"FileVersion"]).c_str(), L"Warning", MB_OK | MB_ICONWARNING);
                    if (config.VerboseDebug)
                    {
                        std::wstring debugMessage = L"DOW2.exe version info:\n";
                        for (const auto& pair : versionStrings) {
                            debugMessage += pair.first + L": " + pair.second + L"\n";
                        }
                        MessageBox(NULL, debugMessage.c_str(), L"Debug", MB_OK | MB_ICONINFORMATION);
                    }
                }
            }

            if (config.VerboseDebug)
            {
                MessageBox(NULL, L"Verified version.", L"Debug", MB_OK | MB_ICONINFORMATION);
            }

            CONSOLE_MESSAGE(L"Version check.");

            int numCores = GetProcessorCoreCount();

            if (numCores > 12 && config.IsSteam && !config.Injector)
            {
                std::wstring dllPath = L"XThread.dll";
                std::wstring binPath = L"XThread.bin";
                std::string currentMD5;

                if (GetFileAttributes(dllPath.c_str()) != INVALID_FILE_ATTRIBUTES)
                {
                    if (CalculateMD5(dllPath.c_str(), currentMD5) && currentMD5 != config.ExpectedInjectorFileMD5Checksum)
                    {
                        std::wstringstream warningMessage;
                        warningMessage << L"Your CPU has more than 12 cores, and DOW2.exe will not launch unless the current XThread.dll file is replaced with an updated version. Would you like to replace the file?";
                        int msgboxID = MessageBox(NULL, warningMessage.str().c_str(), L"Warning", MB_YESNO | MB_ICONWARNING);
                        if (msgboxID == IDYES)
                        {
                            if (!CopyFileRaw(binPath, dllPath))
                            {
                                MessageBox(NULL, L"Failed to create or replace the XThread.dll file with the updated version, the XThread.bin file may be missing. Reacquire it from the mod package, or try again.", L"Error", MB_OK | MB_ICONERROR);
                                return 1;
                            }
                        }
                    }
                }
            }

            if (config.VerboseDebug && config.IsSteam && !config.Injector)
            {
                MessageBox(NULL, L"Verified CPU.", L"Debug", MB_OK | MB_ICONINFORMATION);
            }

            if (config.VerboseDebug && config.IsSteam && !config.Injector)
            {
                CONSOLE_MESSAGE(L"CPU check.");
            }

            // check for a vulkan-capable GPU if DXVK is true
            if (config.IsDXVK && !HasVulkanSupport())
            {
                MessageBox(NULL, L"No Vulkan capable GPU detected by the launcher, the game will likely not run with DXVK, which is required for this mod.", L"Warning", MB_OK | MB_ICONWARNING);
            }

            if (config.VerboseDebug && config.IsDXVK)
            {
                MessageBox(NULL, L"Verified Vulkan.", L"Debug", MB_OK | MB_ICONINFORMATION);
            }

            if (config.VerboseDebug && config.IsDXVK)
            {
                CONSOLE_MESSAGE(L"Vulkan check.");
            }

            bool d3d9IsDXVK = false;

            // check for dxvk
            if (config.IsDXVK)
            {
                bool d3d9Missing = false;
                bool dxvkConfMissing = false;

                if (GetFileAttributes(L"d3d9.dll") == INVALID_FILE_ATTRIBUTES)
                {
                    d3d9Missing = true;
                }

                if (GetFileAttributes(L"dxvk.conf") == INVALID_FILE_ATTRIBUTES)
                {
                    dxvkConfMissing = true;
                }

                if (d3d9Missing)
                {
                    int msgboxID = MessageBox(NULL, L"This mod requires DXVK, but the d3d9.dll file is missing. While you can still proceed to launch the mod, you will crash in large scenarios, and experience a loss in performance. Would you like to acquire DXVK?", L"Warning", MB_YESNO | MB_ICONWARNING);
                    if (msgboxID == IDYES)
                    {
                        if (!CopyFileRaw(L"d3d9.bin", L"d3d9.dll"))
                        {
                            MessageBox(NULL, L"Failed to create or replace the d3d9.dll file with the DXVK version, the d3d9.bin file may be missing. Reacquire it from the mod package, or try again.", L"Error", MB_OK | MB_ICONERROR);
                            return 1;
                        }
                        d3d9IsDXVK = true;

                        if (dxvkConfMissing)
                        {
                            std::ofstream dxvkConfFile("dxvk.conf");
                            if (!dxvkConfFile)
                            {
                                MessageBox(NULL, L"Failed to create the dxvk.conf file. Reacquire it from the mod package, or try again.", L"Error", MB_OK | MB_ICONERROR);
                                return 1;
                            }
                            dxvkConfFile << "dxgi.maxFrameRate = 60\n";
                            dxvkConfFile << "d3d9.maxFrameRate = 60\n";
                            dxvkConfFile.close();
                        }
                    }
                }
                else
                {
                    auto versionStrings = GetFileVersionStrings(L"d3d9.dll", config.VerboseDebug);

                    if (config.VerboseDebug)
                    {
                        std::wstring debugMessage = L"d3d9.dll version info:\n";
                        for (const auto& pair : versionStrings)
                        {
                            debugMessage += pair.first + L": " + pair.second + L"\n";
                        }
                        MessageBox(NULL, debugMessage.c_str(), L"Debug", MB_OK | MB_ICONINFORMATION);
                    }

                    if (versionStrings.find(L"ProductName") == versionStrings.end() || versionStrings[L"ProductName"] != L"DXVK")
                    {
                        int msgboxID = MessageBox(NULL, L"This mod requires DXVK, but the present d3d9.dll file is not identified as DXVK. Would you like to replace it with the DXVK version?", L"Warning", MB_YESNO | MB_ICONWARNING);
                        if (msgboxID == IDYES)
                        {
                            if (!CopyFileRaw(L"d3d9.bin", L"d3d9.dll"))
                            {
                                MessageBox(NULL, L"Failed to create or replace the d3d9.dll file with the DXVK version, the d3d9.bin file may be missing. Reacquire it from the mod package, or try again.", L"Error", MB_OK | MB_ICONERROR);
                                return 1;
                            }
                        }
                    }

                    if (versionStrings.find(L"ProductName") == versionStrings.end() || versionStrings[L"ProductName"] == L"DXVK")
                    {
                        d3d9IsDXVK = true;
                    }

                    if (dxvkConfMissing && d3d9IsDXVK)
                    {
                        std::ofstream dxvkConfFile("dxvk.conf");
                        if (!dxvkConfFile)
                        {
                            MessageBox(NULL, L"Failed to create the dxvk.conf file. Reacquire it from the mod package, or try again.", L"Error", MB_OK | MB_ICONERROR);
                            return 1;
                        }
                        dxvkConfFile << "dxgi.maxFrameRate = 60\n";
                        dxvkConfFile << "d3d9.maxFrameRate = 60\n";
                        dxvkConfFile.close();
                    }
                }
            }
            else
            {
                auto versionStrings = GetFileVersionStrings(L"d3d9.dll", config.VerboseDebug);

                if (config.VerboseDebug)
                {
                    std::wstring debugMessage = L"d3d9.dll version info:\n";
                    for (const auto& pair : versionStrings)
                    {
                        debugMessage += pair.first + L": " + pair.second + L"\n";
                    }
                    MessageBox(NULL, debugMessage.c_str(), L"Debug", MB_OK | MB_ICONINFORMATION);
                }

                if (versionStrings.find(L"ProductName") != versionStrings.end() && versionStrings[L"ProductName"] == L"DXVK") {
                    int msgboxID = MessageBox(NULL, L"You have DXVK installed, but this mod does not require it. Would you like to remove DXVK?", L"Warning", MB_YESNO | MB_ICONWARNING);
                    if (msgboxID == IDYES)
                    {
                        DeleteFile(L"d3d9.dll");
                        DeleteFile(L"dxvk.conf");
                    }
                }
            }

            // additional checks for DXVK and Injector
            if (config.IsDXVK && config.Injector && d3d9IsDXVK)
            {
                auto versionStrings = GetFileVersionStrings(L"d3d9.dll", config.VerboseDebug);
                if (versionStrings.find(L"ProductName") != versionStrings.end() && versionStrings[L"ProductName"] == L"DXVK")
                {
                    struct FileCheck
                    {
                        std::wstring fileName;
                        std::wstring binFileName;
                        std::string expectedMD5;
                    };

                    FileCheck filesToCheck[] =
                    {
                        {L"DivxDecoder.dll", L"DivxDecoder.bin", "57e72cae12091dafa29a8e4db8b4f1d1"},
                        {L"DivxMediaLib.dll", L"DivxMediaLib.bin", "937c646bcd8407aee9de381b947433c8"}
                    };

                    for (const auto& file : filesToCheck)
                    {
                        std::wstring filePath = file.fileName;
                        std::string currentMD5;
                        if (GetFileAttributes(filePath.c_str()) != INVALID_FILE_ATTRIBUTES)
                        {
                            if (CalculateMD5(filePath.c_str(), currentMD5) && currentMD5 != file.expectedMD5)
                            {
                                if (!CopyFileRaw(file.binFileName, file.fileName))
                                {
                                    std::wstringstream errorMessage;
                                    errorMessage << L"The DXVK and injector combination requires special DIVX files in order to allow for movies to play correctly, but we failed to create or replace the " << file.fileName << L" file with the correct version, the " << file.binFileName << L" file may be missing. Reacquire it from the mod package, or try again.";
                                    MessageBox(NULL, errorMessage.str().c_str(), L"Error", MB_OK | MB_ICONERROR);
                                }
                            }
                        }
                    }
                }
            }

            if (config.VerboseDebug && config.IsDXVK)
            {
                MessageBox(NULL, L"Verified DXVK.", L"Debug", MB_OK | MB_ICONINFORMATION);
            }

            if (config.IsDXVK)
            {
                CONSOLE_MESSAGE(L"DXVK check.");
            }

            // check for LAA
            if (config.LAAPatch && Is32BitApplication(APP_NAME))
            {
                if (!IsLargeAddressAware(APP_NAME))
                {
                    int msgboxID = MessageBox(NULL, L"DOW2.exe is not large address aware, meaning it won't allocate more than 2gb of address space, which is not enough for this mod. Would you like to apply the large address aware patch?", L"Warning", MB_YESNO | MB_ICONWARNING);
                    if (msgboxID == IDYES)
                    {
                        if (!ApplyLargeAddressAwarePatch(APP_NAME))
                        {
                            MessageBox(NULL, L"Failed to apply the large address aware patch to DOW2.exe. Try again, or apply it manually.", L"Error", MB_OK | MB_ICONERROR);
                        }
                    }
                }
            }

            if (config.VerboseDebug && config.LAAPatch)
            {
                MessageBox(NULL, L"Verified large address aware.", L"Debug", MB_OK | MB_ICONINFORMATION);
            }

            if (config.LAAPatch)
            {
                CONSOLE_MESSAGE(L"Large address aware check.");
            }

            // check game settings for UI incompatibilities, errors are handled in the function
            if (config.UIWarnings)
            {
                CheckGameConfiguration(config);
            }

            if (config.VerboseDebug && config.UIWarnings)
            {
                MessageBox(NULL, L"Verified game configuration.", L"Debug", MB_OK | MB_ICONINFORMATION);
            }

            if (config.UIWarnings)
            {
                CONSOLE_MESSAGE(L"Game configuration check.");
            }

            // check for injector
            if (config.Injector)
            {
                // check if the .config file exists in the same directory
                if (GetFileAttributes(configFileName.c_str()) == INVALID_FILE_ATTRIBUTES)
                {
                    MessageBox(NULL, (L"Failed to find the mod's " + configFileName + L" injector config file. Reacquire it from the mod package, or try again.").c_str(), L"Error", MB_OK | MB_ICONERROR);
                    return 1;
                }

                if (config.VerboseDebug && config.Injector) {
                    MessageBox(NULL, L"Verified injector config.", L"Debug", MB_OK | MB_ICONINFORMATION);
                }

                // read mod-folder from the .config file
                std::wstring configFilePath = rootDir + L"\\" + configFileName;
                std::wstring modFolder = ReadModFolderFromConfig(configFilePath);

                std::wstring modFolderPath = rootDir + L"\\" + modFolder;
                if (!InjectedFilesPresent(modFolderPath, config.InjectedFiles))
                {
                    return 1; // error message is handled in InjectedFilesPresent
                }

                if (config.VerboseDebug && config.Injector) {
                    MessageBox(NULL, L"Verified injected files.", L"Debug", MB_OK | MB_ICONINFORMATION);
                }

                if (!InjectedConfigurationsPresent(modName, config.InjectedConfigurations))
                {
                    return 1; // error message is handled in InjectedConfigurationsPresent
                }

                if (config.VerboseDebug && config.Injector) {
                    MessageBox(NULL, L"Verified injected configurations.", L"Debug", MB_OK | MB_ICONINFORMATION);
                }

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
                    // create the event for synchronization
                    HANDLE hEvent = CreateEvent(NULL, TRUE, FALSE, NULL); // manual-reset event
                    if (hEvent == NULL)
                    {
                        return 1;
                    }

                    // start the binary processing thread to check and possibly replace the injector file
                    HANDLE hThread = CreateThread(NULL, 0, InjectorBinaryProcessingThread, &config, 0, NULL);
                    if (hThread == NULL)
                    {
                        CloseHandle(hEvent);
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
                        return 1;
                    }

                    DWORD threadExitCode;
                    if (!GetExitCodeThread(hThread, &threadExitCode) || threadExitCode != 0)
                    {
                        CloseHandle(hEvent);
                        CloseHandle(hThread);
                        return 1;
                    }

                    CloseHandle(hEvent);
                    CloseHandle(hThread);
                }
            }

            if (config.VerboseDebug && config.Injector)
            {
                MessageBox(NULL, L"Verified injector.", L"Debug", MB_OK | MB_ICONINFORMATION);
            }

            if (config.Injector)
            {
                CONSOLE_MESSAGE(L"Injector check.");
            }

            // check if the .module file exists in the same directory
            if (GetFileAttributes(moduleFileName.c_str()) == INVALID_FILE_ATTRIBUTES)
            {
                MessageBox(NULL, (L"Failed to find the mod's " + moduleFileName + L" module file. Reacquire it from the mod package, or try again.").c_str(), L"Error", MB_OK | MB_ICONERROR);
                return 1;
            }

            if (config.VerboseDebug)
            {
                MessageBox(NULL, L"Found module file.", L"Debug", MB_OK | MB_ICONINFORMATION);
            }

            if (!CheckModuleFile(moduleFileName, config))
            {
                return 1;
            }

            if (config.VerboseDebug)
            {
                MessageBox(NULL, L"Verified module contents.", L"Debug", MB_OK | MB_ICONINFORMATION);
            }

            CONSOLE_MESSAGE(L"Module check.");

            // call the UCS file validation function
            if (!ValidateUCSFiles(rootDir, config))
            {
                return 1;
            }

            if (config.VerboseDebug)
            {
                MessageBox(NULL, L"Verified UCS files.", L"Debug", MB_OK | MB_ICONINFORMATION);
            }

            CONSOLE_MESSAGE(L"UCS check.");

            if (config.VerboseDebug)
            {
                MessageBox(NULL, L"All checks complete. Preparing to launch the game.", L"Debug", MB_OK | MB_ICONINFORMATION);
            }

            CONSOLE_MESSAGE(L"ALL CHECKS COMPLETE.");
        }

        // END CHECKS
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

        CONSOLE_MESSAGE(L"DOW2.exe executed.");

        bool dow2_running = false;

        // loop until the program is found running
        while (!dow2_running) 
        {
            dow2_running = IsProcessRunning(APP_NAME);
            if (!dow2_running) 
            {
                Sleep(1000);
            }
        }

        if (dow2_running)
        {
            if (!config.IsUnsafe)
            {
                DWORD dow2ProcessId = 0;
                HWND mainWindowHandle = NULL;

                while (true)
                {
                    dow2ProcessId = FindProcessId(APP_NAME);
                    mainWindowHandle = GetMainWindowHandle(dow2ProcessId);
                    if (dow2ProcessId != 0 && mainWindowHandle != NULL)
                    {
                        break;
                    }
                    Sleep(1000);
                }

                std::thread monitorThread(MonitorAndSetPriority);
                monitorThread.detach();

                std::string rawCommandLine = GetCommandLineOfProcess(dow2ProcessId);
                std::string actualCommandLine = StripExecutablePath(rawCommandLine);
                std::string expectedCommandLine = WStringToString(commandLine);

                // trim whitespace
                actualCommandLine = TrimString(actualCommandLine);
                expectedCommandLine = TrimString(expectedCommandLine);

                // normalize command lines (convert to lowercase and remove extra spaces)
                actualCommandLine = NormalizeCommandLine(actualCommandLine);
                expectedCommandLine = NormalizeCommandLine(expectedCommandLine);

                // ensure both are trimmed again to remove any leading/trailing spaces
                actualCommandLine = TrimString(actualCommandLine);
                expectedCommandLine = TrimString(expectedCommandLine);

                if (config.VerboseDebug)
                {
                    std::cout << "Raw actual command line: " << rawCommandLine << std::endl;
                    std::cout << "Processed actual command line: " << actualCommandLine << std::endl;
                    std::cout << "Expected command line: " << expectedCommandLine << std::endl;
                }

                // convert strings to wstrings for comparison and MessageBox
                std::wstring actualCommandLineW = StringToWString(actualCommandLine);
                std::wstring expectedCommandLineW = StringToWString(expectedCommandLine);

                if (actualCommandLineW != expectedCommandLineW)
                {
                    SuspendProcess(dow2ProcessId);
                    std::wstring warningMessage = L"DOW2.exe was launched with externally set launch parameters. If this is unintentional, make sure that you do not have extra launch parameters set through Steam, GOG, the runoptions.cfg file, or other means.\n\n";
                    warningMessage += L"Expected Launch Parameters: " + expectedCommandLineW + L"\n";
                    warningMessage += L"Actual Launch Parameters: " + actualCommandLineW;
                    MessageBox(NULL, warningMessage.c_str(), L"Warning", MB_OK | MB_ICONWARNING);
                    ResumeProcess(dow2ProcessId);
                    ForceFocusOnWindow(mainWindowHandle);
                }
            }
        }

        // wait for a time before closing the launcher
        Sleep(15000);

        // destroy the bitmap window after waiting
        if (hwndBitmap)
        {
            DestroyWindow(hwndBitmap);
        }

        // close the process and thread handles
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }

    if (isLinux)
    {
        // get the executable path using Boost.DLL
        std::string executablePath = boost::dll::program_location().string();
        std::vector<std::string> pathComponents;
        boost::split(pathComponents, executablePath, boost::is_any_of("\\/"));
        std::string executableName = pathComponents.empty() ? "" : pathComponents.back();
        std::wstring launcherName = boost::locale::conv::to_utf<wchar_t>(executableName, "UTF-8");

        std::wstring modName = std::wstring(launcherName).substr(0, launcherName.find_last_of(L"."));
        std::wstring commandLine = std::wstring(APP_NAME) + L" -modname " + modName;
        try
        {
            boost::process::ipstream pipe_stream;
            std::string commandLineStr = WStringToString(commandLine);
            boost::process::child c(commandLineStr, boost::process::std_out > pipe_stream);

            std::string line;
            while (pipe_stream && std::getline(pipe_stream, line) && !line.empty())
            {
                std::cout << line << std::endl;
            }

            c.wait();
            if (c.exit_code() != 0)
            {
                std::cerr << "Failed to find or open DOW2.exe. You have installed the mod into the wrong directory, or your game is missing or corrupt. Install the mod into the correct game directory, or try again." << std::endl;
                std::cerr << "Press [Enter] to exit..." << std::endl;
                std::cin.get();
                return 1;
            }

            // close the launcher after DOW2.exe has finished executing
            std::cout << "DOW2.exe executed." << std::endl;
            return 0;
        }
        catch (const std::exception& e) 
        {
            std::cerr << "Error: " << e.what() << std::endl;
            std::cerr << "Press [Enter] to exit..." << std::endl;
            std::cin.get();
            return 1;
        }
    }

    return 0;
}