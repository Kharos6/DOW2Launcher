#define APP_NAME L"DOW2.exe"

//local headers
#include "framework.h"

// structure to hold the launch configuration
struct LaunchConfig
{
    bool Injector = false;
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
    bool WIN7CompatibilityMode = false;
    bool Warnings = false;
    std::vector<std::wstring> AdditionalFiles;
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
            MessageBox(NULL, (L"Failed to find a specific injected file required by this mod. Try again, or reacquire it from the mod package: " + fileName).c_str(), L"Error", MB_OK | MB_ICONERROR);
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
            MessageBox(NULL, (L"Failed to find a specific injection configuration file required by this mod. Try again, or reacquire it from the mod package: " + filePath).c_str(), L"Error", MB_OK | MB_ICONERROR);
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
        L"Injector", L"BitmapWidth", L"BitmapHeight", L"InjectorFileName", L"LaunchParams", L"IsRetribution", L"IsSteam", L"VerboseDebug", L"IsDXVK", L"FirstTimeLaunchCheck", L"FirstTimeLaunchMessage", L"IsUnsafe", L"Console", L"InjectedFiles", L"InjectedConfigurations", L"GameVersion", L"LAAPatch", L"UIWarnings", L"WIN7CompatibilityMode", L"Warnings", L"AdditionalFiles"
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
                MessageBox(NULL, L"The [LaunchParams] field of the launch configuration file contains the -modname parameter, which is automatically applied with the appropriate argument for this mod based on the name of the launcher. Remove -modname from the launch configuration file.", L"Error", MB_OK | MB_ICONERROR);
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
                            MessageBox(NULL, L"Invalid formatting for the [InjectedConfigurations] field of the launch configuration file. Each entry must be a valid file extension for injection configuration file types used by this mod.", L"Error", MB_OK | MB_ICONERROR);
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
                        MessageBox(NULL, L"Invalid formatting for the [InjectedConfigurations] field of the launch configuration file. Each entry must be a valid file extension for injection configuration file types used by this mod.", L"Error", MB_OK | MB_ICONERROR);
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
        else if (key == L"WIN7CompatibilityMode")
        {
            if (!ValidateBooleanField(value))
            {
                MessageBox(NULL, L"Invalid formatting for the [WIN7CompatibilityMode] field of the launch configuration file. It must be true or false.", L"Error", MB_OK | MB_ICONERROR);
                exit(1);
            }
            config.WIN7CompatibilityMode = (value == L"true");
        }
        else if (key == L"Warnings")
        {
            if (!ValidateBooleanField(value))
            {
                MessageBox(NULL, L"Invalid formatting for the [Warnings] field of the launch configuration file. It must be true or false.", L"Error", MB_OK | MB_ICONERROR);
                exit(1);
            }
            config.Warnings = (value == L"true");
        }
        else if (key == L"AdditionalFiles")
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
                    if (token.find(L".") == std::wstring::npos)
                    {
                        MessageBox(NULL, L"Invalid formatting for the [AdditionalFiles] field of the launch configuration file. Each full file name must include a file extension.", L"Error", MB_OK | MB_ICONERROR);
                        exit(1);
                    }
                    if (value[endPos + 1] != L' ')
                    {
                        MessageBox(NULL, L"Invalid formatting for the [AdditionalFiles] field of the launch configuration file. Each entry must be separated by a comma and a space.", L"Error", MB_OK | MB_ICONERROR);
                        exit(1);
                    }
                    config.AdditionalFiles.push_back(token);
                    startPos = endPos + 2;
                }
                token = value.substr(startPos);
                token = TrimWString(token);
                if (token.find(L".") == std::wstring::npos)
                {
                    MessageBox(NULL, L"Invalid formatting for the [AdditionalFiles] field of the launch configuration file. Each full file name must include a file extension.", L"Error", MB_OK | MB_ICONERROR);
                    exit(1);
                }
                config.AdditionalFiles.push_back(token);
            }
            else
            {
                config.AdditionalFiles.push_back(value);
            }
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
            errorMsg += key + L", ";
        }

        // remove the last comma and space
        if (!requiredKeys.empty())
        {
            errorMsg = errorMsg.substr(0, errorMsg.length() - 2);
        }

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
    configFile << L"WIN7CompatibilityMode=" << (config.WIN7CompatibilityMode ? L"true" : L"false") << L"\n";
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
    configFile << L"AdditionalFiles=";
    for (size_t i = 0; i < config.AdditionalFiles.size(); ++i)
    {
        configFile << config.AdditionalFiles[i];
        if (i < config.AdditionalFiles.size() - 1)
        {
            configFile << L", ";
        }
    }
    configFile << L"\n";
    configFile << L"FirstTimeLaunchCheck=" << (config.FirstTimeLaunchCheck ? L"true" : L"false") << L"\n";
    configFile << L"FirstTimeLaunchMessage=" << config.FirstTimeLaunchMessage << L"\n";
    configFile << L"VerboseDebug=" << (config.VerboseDebug ? L"true" : L"false") << L"\n";
    configFile << L"Warnings=" << (config.Warnings ? L"true" : L"false") << L"\n";
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
bool InjectorBinaryProcessing(const LaunchConfig& config, const std::wstring& launcherName)
{
    std::string actualChecksum, expectedChecksum;

    // Construct the injector bin file name using the launcher name and _injectorfilename from the configuration
    std::wstring binFileName = launcherName + L"_" + config.InjectorFileName.substr(0, config.InjectorFileName.find_last_of(L".")) + L".bin";

    // calculate the expected checksum from the .bin file
    if (!CalculateMD5(binFileName.c_str(), expectedChecksum))
    {
        std::wstringstream errorMessage;
        errorMessage << L"Failed to calculate the MD5 checksum of the " << binFileName << L" file in order to validate the injector. The file may be missing. Reacquire it from the mod package, or try again.";
        MessageBox(NULL, errorMessage.str().c_str(), L"Error", MB_OK | MB_ICONERROR);
        return false;
    }

    // calculate the actual checksum of the injector file
    if (!CalculateMD5(config.InjectorFileName.c_str(), actualChecksum) && actualChecksum != expectedChecksum)
    {
        if (!CopyFileRaw(binFileName.c_str(), config.InjectorFileName.c_str()))
        {
            std::wstringstream errorMessage;
            errorMessage << L"Failed to replace the " << config.InjectorFileName << L" file with the valid injector version for this mod. The file " << binFileName << L" may be missing. Reacquire it from the mod package, or try again.";
            MessageBox(NULL, errorMessage.str().c_str(), L"Error", MB_OK | MB_ICONERROR);
            return false;
        }

        // verify the checksum again after replacement
        if (!CalculateMD5(config.InjectorFileName.c_str(), actualChecksum) && actualChecksum != expectedChecksum)
        {
            std::wstringstream errorMessage;
            errorMessage << config.InjectorFileName << L" file MD5 checksum still mismatched after attempted replacement with the valid injector version for this mod. Reacquire it from the mod package, or try again.";
            MessageBox(NULL, errorMessage.str().c_str(), L"Error", MB_OK | MB_ICONERROR);
            return false;
        }
    }
    return true;
}

// function to process individual UCS files
bool ProcessUCSFile(const std::wstring& filePath)
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
        // convert to UTF-16 LE and write back to the file
        std::ofstream outFile(filePath, std::ios::binary | std::ios::trunc);
        if (!outFile.is_open())
        {
            MessageBox(NULL, (L"Failed to find or open faulty UCS file. Try again, or reacquire it from the mod package: " + filePath).c_str(), L"Error", MB_OK | MB_ICONERROR);
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
        return false;
    }

    std::wstringstream wss(wFileContent);
    std::unordered_map<unsigned long, std::vector<int>> numberLineMap;
    std::wstring line;
    unsigned long currentNumber = 0;
    int lineNumber = 0;

    while (std::getline(wss, line))
    {
        lineNumber++;

        size_t firstNonWhitespace = line.find_first_not_of(L" \t");

        // allow empty lines and lines with only carriage returns or newlines
        if (line.empty() || line == L"\r" || line == L"\n" || line == L"\r\n")
        {
            continue;
        }

        // check if the line is entirely whitespace
        if (firstNonWhitespace == std::wstring::npos)
        {
            std::wstring errorMessage = L"Whitespace entry in UCS file: " + filePath + L"\nLine: " + std::to_wstring(lineNumber);
            MessageBox(NULL, errorMessage.c_str(), L"Error", MB_OK | MB_ICONERROR);
            return false;
        }

        // remove leading whitespace
        line = line.substr(firstNonWhitespace);

        // if no digits found at the start of the line
        if (!iswdigit(line[0]))
        {
            std::wstring errorMessage = L"Not a numeric entry in UCS file: " + filePath + L"\nLine: " + std::to_wstring(lineNumber) + L": " + line;
            MessageBox(NULL, errorMessage.c_str(), L"Error", MB_OK | MB_ICONERROR);
            return false;
        }

        size_t firstNonDigitPos = line.find_first_not_of(L"0123456789");

        // if the number part is found, extract it
        std::wstring numberPart = line.substr(0, firstNonDigitPos);
        std::wstring textPart = line.substr(firstNonDigitPos);

        // ensure the number part is purely numeric
        if (!std::all_of(numberPart.begin(), numberPart.end(), iswdigit))
        {
            std::wstring errorMessage = L"Not a numeric entry in UCS file: " + filePath + L"\nLine: " + std::to_wstring(lineNumber) + L": " + line;
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
bool ValidateUCSFiles(const std::wstring& rootDir)
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

                        if (!ProcessUCSFile(filePath))
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

                    if (GetFileAttributes(localeFolder.c_str()) == INVALID_FILE_ATTRIBUTES || GetFileAttributes(ucsFile.c_str()) == INVALID_FILE_ATTRIBUTES)
                    {
                        // Locale folder or DOW2.ucs file does not exist, we skip this .sga file check
                        continue;
                    }
                }
            }

            if (GetFileAttributes(fullPath.c_str()) == INVALID_FILE_ATTRIBUTES)
            {
                MessageBox(NULL, (L"Missing archive " + fullPath + L" required by this mod. Reacquire it from the mod package, or try again.").c_str(), L"Error", MB_OK | MB_ICONERROR);
                return false;
            }
        }
    }

    return true;
}

// function to check the game's configuration file
void CheckGameConfiguration(const LaunchConfig& config) 
{
    wchar_t* userProfilePath = nullptr;
    SHGetKnownFolderPath(FOLDERID_Documents, 0, NULL, &userProfilePath);

    std::wstring gameFolder = config.IsRetribution ? L"Dawn of War II - Retribution" : L"Dawn of War 2";
    std::wstring gameConfigFilePath = std::wstring(userProfilePath) + L"\\My Games\\" + gameFolder + L"\\Settings\\configuration.lua";

    std::wifstream configFile(gameConfigFilePath);
    if (!configFile) 
    {
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

// function to check additional files
bool CheckAdditionalFiles(const LaunchConfig& config, const std::wstring& launcherName)
{
    for (const auto& fileName : config.AdditionalFiles)
    {
        if (fileName.empty())
        {
            continue;
        }

        bool fileMissing = false;

        std::wstring filePath = fileName;
        std::string actualChecksum, expectedChecksum;
        size_t lastDotPos = fileName.find_last_of(L'.');
        std::wstring baseFileName = (lastDotPos == std::wstring::npos) ? fileName : fileName.substr(0, lastDotPos);
        std::wstring expectedFileName = launcherName + L"_" + baseFileName + L".bin";

        // skip the checksum verification if the .bin file is only the launcher name with an underscore
        if (baseFileName == launcherName + L"_")
        {
            continue;
        }

        if (GetFileAttributes(filePath.c_str()) == INVALID_FILE_ATTRIBUTES)
        {
            fileMissing = true;
        }

        if (fileMissing)
        {
            if (!CopyFileRaw(expectedFileName, fileName))
            {
                std::wstringstream errorMessage;
                errorMessage << L"Failed to create or replace the " + fileName + L" file required by this mod. Reacquire it from the mod package, or try again.";
                MessageBox(NULL, errorMessage.str().c_str(), L"Error", MB_OK | MB_ICONERROR);
                return false;
            }
        }

        if (!CalculateMD5(filePath.c_str(), actualChecksum))
        {
            MessageBox(NULL, (L"Failed to calculate the MD5 checksum of the " + fileName + L" file. It may be missing. Reacquire it from the mod package").c_str(), L"Error", MB_OK | MB_ICONERROR);
            return false;
        }

        if (!CalculateMD5(expectedFileName.c_str(), expectedChecksum))
        {
            MessageBox(NULL, (L"Failed to calculate MD5 checksum of the " + expectedFileName + L" file. It may be missing. Reacquire it from the mod package").c_str(), L"Error", MB_OK | MB_ICONERROR);
            return false;
        }

        if (actualChecksum != expectedChecksum)
        {
            if (!CopyFileRaw(expectedFileName, filePath))
            {
                MessageBox(NULL, (L"Failed to replace the " + fileName + L" file with the required version for this mod. Reacquire it from the mod package, or try again.").c_str(), L"Error", MB_OK | MB_ICONERROR);
                return false;
            }

            if (!CalculateMD5(filePath.c_str(), actualChecksum) && actualChecksum != expectedChecksum)
            {
                MessageBox(NULL, (fileName + L" file MD5 checksum still mismatched after attempted replacement with the required version for this mod. Reacquire it from the mod package, or try again.").c_str(), L"Error", MB_OK | MB_ICONERROR);
                return false;
            }
        }
    }
    return true;
}

// function to verify XThread
bool VerifyXThread(const LaunchConfig& config, const std::wstring& launcherName)
{
    std::wstring dllPath = L"XThread.dll";
    std::wstring binPath = launcherName + L"_XThread.bin";
    std::string binMD5, currentMD5;

    if (CalculateMD5(binPath.c_str(), binMD5))
    {
        if (GetFileAttributes(dllPath.c_str()) != INVALID_FILE_ATTRIBUTES)
        {
            if (CalculateMD5(dllPath.c_str(), currentMD5) && currentMD5 != binMD5)
            {
                if (!CopyFileRaw(binPath, dllPath))
                {
                    std::wstringstream errorMessage;
                    errorMessage << L"Failed to create or replace the XThread.dll file with the updated version that is required for the game to run on CPUs with more than twelve cores. The " << binPath << L" file may be missing. Reacquire it from the mod package, or try again.";
                    MessageBox(NULL, errorMessage.str().c_str(), L"Error", MB_OK | MB_ICONERROR);
                    return false;
                }

                if (CalculateMD5(dllPath.c_str(), currentMD5) && currentMD5 != binMD5)
                {
                    MessageBox(NULL, L"XThread.dll file MD5 checksum still mismatched after attempted replacement with the updated version that is required for the game to run on CPUs with more than twelve cores. Reacquire it from the mod package, or try again.", L"Error", MB_OK | MB_ICONERROR);
                    return false;
                }
            }
        }
    }
    else
    {
        std::wstringstream errorMessage;
        errorMessage << L"Failed to calculate the MD5 checksum of the " << binPath << L" file in order to validate XThread.dll. The file may be missing. Reacquire it from the mod package, or try again.";
        MessageBox(NULL, errorMessage.str().c_str(), L"Error", MB_OK | MB_ICONERROR);
        return false;
    }
    return true;
}

// function to verify DXVK
bool VerifyDXVK(const LaunchConfig& config, const std::wstring& launcherName)
{
    bool d3d9IsDXVK = false;
    bool d3d9Missing = false;
    bool dxvkConfMissing = false;

    if (config.IsDXVK)
    {

        if (GetFileAttributes(L"d3d9.dll") == INVALID_FILE_ATTRIBUTES)
        {
            d3d9Missing = true;
        }

        if (GetFileAttributes(L"dxvk.conf") == INVALID_FILE_ATTRIBUTES)
        {
            dxvkConfMissing = true;
        }

        std::wstring d3d9BinPath = launcherName + L"_d3d9.bin";

        if (d3d9Missing)
        {
            if (config.Warnings)
            {
                int msgboxID = MessageBox(NULL, L"This mod requires DXVK, but the d3d9.dll file is missing. While you can still proceed to launch this mod, you will crash in large scenarios, and experience a loss in performance. Would you like to acquire DXVK?", L"Warning", MB_YESNO | MB_ICONWARNING);
                if (msgboxID == IDYES)
                {
                    if (!CopyFileRaw(d3d9BinPath, L"d3d9.dll"))
                    {
                        std::wstringstream errorMessage;
                        errorMessage << L"Failed to create or replace the d3d9.dll file with the DXVK version. The " << d3d9BinPath << L" file may be missing. Reacquire it from the mod package, or try again.";
                        MessageBox(NULL, errorMessage.str().c_str(), L"Error", MB_OK | MB_ICONERROR);
                        return false;
                    }
                    d3d9IsDXVK = true;

                    if (dxvkConfMissing)
                    {
                        std::ofstream dxvkConfFile("dxvk.conf");
                        if (!dxvkConfFile)
                        {
                            MessageBox(NULL, L"Failed to create the dxvk.conf file. Reacquire it from the mod package, or try again.", L"Error", MB_OK | MB_ICONERROR);
                            return false;
                        }
                        dxvkConfFile << "dxgi.maxFrameRate = 60\n";
                        dxvkConfFile << "d3d9.maxFrameRate = 60\n";
                        dxvkConfFile.close();
                    }
                }
            }
        }
        else
        {
            auto versionStrings = GetFileVersionStrings(L"d3d9.dll");

            if (versionStrings.find(L"ProductName") == versionStrings.end() || versionStrings[L"ProductName"] != L"DXVK")
            {
                if (config.Warnings)
                {
                    int msgboxID = MessageBox(NULL, L"This mod requires DXVK, but the present d3d9.dll file is not identified as DXVK. While you can still proceed to launch this mod, you will crash in large scenarios, and experience a loss in performance. Would you like to replace it with the DXVK version?", L"Warning", MB_YESNO | MB_ICONWARNING);
                    if (msgboxID == IDYES)
                    {
                        if (!CopyFileRaw(d3d9BinPath, L"d3d9.dll"))
                        {
                            std::wstringstream errorMessage;
                            errorMessage << L"Failed to create or replace the d3d9.dll file with the DXVK version. The " << d3d9BinPath << L" file may be missing. Reacquire it from the mod package, or try again.";
                            MessageBox(NULL, errorMessage.str().c_str(), L"Error", MB_OK | MB_ICONERROR);
                            return false;
                        }
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
                    MessageBox(NULL, L"Failed to create the dxvk.conf file for DXVK. Reacquire it from the mod package, or try again.", L"Error", MB_OK | MB_ICONERROR);
                    return false;
                }
                dxvkConfFile << "dxgi.maxFrameRate = 60\n";
                dxvkConfFile << "d3d9.maxFrameRate = 60\n";
                dxvkConfFile.close();
            }
        }
    }

    if (config.IsDXVK && config.Injector && d3d9IsDXVK)
    {
        auto versionStrings = GetFileVersionStrings(L"d3d9.dll");
        if (versionStrings.find(L"ProductName") != versionStrings.end() && versionStrings[L"ProductName"] == L"DXVK")
        {
            struct FileCheck
            {
                std::wstring fileName;
                std::wstring binFileName;
            };

            FileCheck filesToCheck[] =
            {
                {L"DivxDecoder.dll", L"DivxDecoder.bin"},
                {L"DivxMediaLib.dll", L"DivxMediaLib.bin"}
            };

            for (const auto& file : filesToCheck)
            {
                std::wstring filePath = file.fileName;
                std::wstring binFilePath = launcherName + L"_" + file.binFileName;
                std::string currentMD5, expectedMD5;
                if (GetFileAttributes(filePath.c_str()) != INVALID_FILE_ATTRIBUTES)
                {
                    // calculate the expected MD5 checksum from the .bin file
                    if (CalculateMD5(binFilePath.c_str(), expectedMD5))
                    {
                        // calculate the current MD5 checksum from the .dll file
                        if (CalculateMD5(filePath.c_str(), currentMD5) && currentMD5 != expectedMD5)
                        {
                            if (!CopyFileRaw(binFilePath.c_str(), file.fileName.c_str()))
                            {
                                std::wstringstream errorMessage;
                                errorMessage << L"Failed to create or replace the " << file.fileName << L" file with the correct version that is required in order to allow movies to play correctly with the DXVK and injector combination. The " << binFilePath << L" file may be missing. Reacquire it from the mod package, or try again.";
                                MessageBox(NULL, errorMessage.str().c_str(), L"Error", MB_OK | MB_ICONERROR);
                                return false;
                            }
                        }
                    }
                    else
                    {
                        std::wstringstream errorMessage;
                        errorMessage << L"Failed to calculate the MD5 checksum of the " << binFilePath << L" file in order to validate the necessary DIVX files for the injector and DXVK combination. The file may be missing. Reacquire it from the mod package, or try again.";
                        MessageBox(NULL, errorMessage.str().c_str(), L"Error", MB_OK | MB_ICONERROR);
                        return false;
                    }
                }
            }
        }
    }

    if (!config.IsDXVK)
    {
        auto versionStrings = GetFileVersionStrings(L"d3d9.dll");

        if (versionStrings.find(L"ProductName") != versionStrings.end() && versionStrings[L"ProductName"] == L"DXVK")
        {
            if (config.Warnings)
            {
                int msgboxID = MessageBox(NULL, L"You have DXVK installed, but this mod does not require it. Would you like to remove DXVK?", L"Warning", MB_YESNO | MB_ICONWARNING);
                if (msgboxID == IDYES)
                {
                    DeleteFile(L"d3d9.dll");
                    DeleteFile(L"dxvk.conf");

                    if (GetFileAttributes(L"d3d9.dll") != INVALID_FILE_ATTRIBUTES)
                    {
                        MessageBox(NULL, L"Failed to delete the DXVK d3d9.dll file. Remove it manually, or try again.", L"Error", MB_OK | MB_ICONERROR);
                        return false;
                    }

                    if (GetFileAttributes(L"dxvk.conf") != INVALID_FILE_ATTRIBUTES)
                    {
                        MessageBox(NULL, L"Failed to delete the DXVK dxvk.conf file. Remove it manually, or try again.", L"Error", MB_OK | MB_ICONERROR);
                        return false;
                    }
                }
            }
        }
    }
    return true;
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

        std::wstring baseLauncherName = std::wstring(launcherName).substr(0, launcherName.find_last_of(L"."));

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

        if (!config.Warnings && config.VerboseDebug)
        {
            MessageBox(NULL, L"Warnings are disabled.", L"Debug", MB_OK | MB_ICONINFORMATION);
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
                MessageBox(NULL, L"DOW2.exe is already running. Close it before launching again.", L"Error", MB_OK | MB_ICONERROR);
                return 1;
            }

            if (config.VerboseDebug)
            {
                MessageBox(NULL, L"Verified DOW2.exe running state.", L"Debug", MB_OK | MB_ICONINFORMATION);
            }

            // check if DOW2.exe exists in the same directory as the launcher
            if (GetFileAttributes(APP_NAME) == INVALID_FILE_ATTRIBUTES)
            {
                MessageBox(NULL, L"Failed to find DOW2.exe. You have installed the mod into the wrong directory, or your game is missing or corrupt. Install the mod into the correct directory, or try again.", L"Error", MB_OK | MB_ICONERROR);
                return 1;
            }

            if (config.VerboseDebug)
            {
                MessageBox(NULL, L"Verified DOW2.exe presence.", L"Debug", MB_OK | MB_ICONINFORMATION);
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

            // check for ChaosRisingGDF.dll if IsRetribution is false
            if (!config.IsRetribution && GetFileAttributes(L"ChaosRisingGDF.dll") == INVALID_FILE_ATTRIBUTES)
            {
                MessageBox(NULL, L"The ChaosRisingGDF.dll file is missing, but this mod is designed for Dawn of War II - Chaos Rising. Install the mod to Dawn of War II - Chaos Rising.", L"Error", MB_OK | MB_ICONERROR);
                return 1;
            }

            // check for CGalaxy.dll if IsSteam is false
            if (!config.IsSteam && GetFileAttributes(L"CGalaxy.dll") == INVALID_FILE_ATTRIBUTES)
            {
                MessageBox(NULL, L"The CGalaxy.dll file is missing, but this mod is designed for the GOG distribution of the game. Install the mod to the GOG distribution of the game.", L"Error", MB_OK | MB_ICONERROR);
                return 1;
            }

            // check GameVersion field entry against DOW2.exe file version
            if (!config.GameVersion.empty())
            {
                std::map<std::wstring, std::wstring> versionStrings = GetFileVersionStrings(APP_NAME);
                if (versionStrings[L"FileVersion"] != config.GameVersion)
                {
                    if (config.Warnings)
                    {
                        MessageBox(NULL, (L"File version of DOW2.exe does not match the supported version of the game for this mod. Your gameplay experience may be altered, or the mod may not work. Expected: " + config.GameVersion + L", Found: " + versionStrings[L"FileVersion"]).c_str(), L"Warning", MB_OK | MB_ICONWARNING);
                    }
                }
            }

            if (config.VerboseDebug)
            {
                MessageBox(NULL, L"Verified version.", L"Debug", MB_OK | MB_ICONINFORMATION);
            }

            CONSOLE_MESSAGE(L"Version check.");

            int numCores = GetProcessorCoreCount();

            if (numCores >= 12 && config.IsSteam && !config.Injector)
            {
               if (!VerifyXThread(config, baseLauncherName))
               {
                   return 1;
               }
            }

            if (config.VerboseDebug)
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
                if (config.Warnings)
                {
                    MessageBox(NULL, L"No Vulkan capable GPU detected by the launcher, the game will likely not run with DXVK, which is required for this mod.", L"Warning", MB_OK | MB_ICONWARNING);
                }
            }

            if (config.VerboseDebug)
            {
                MessageBox(NULL, L"Verified Vulkan.", L"Debug", MB_OK | MB_ICONINFORMATION);
            }

            if (config.IsDXVK)
            {
                CONSOLE_MESSAGE(L"Vulkan check.");
            }

            // check for dxvk
            if (!VerifyDXVK(config, baseLauncherName))
            {
               return 1;
            }

            if (config.VerboseDebug)
            {
                MessageBox(NULL, L"Verified DXVK.", L"Debug", MB_OK | MB_ICONINFORMATION);
            }

            if (config.IsDXVK)
            {
                CONSOLE_MESSAGE(L"DXVK check.");
            }

            // check for large address aware
            if (config.LAAPatch && Is32BitApplication(APP_NAME))
            {
                if (!IsLargeAddressAware(APP_NAME))
                {
                    if (config.Warnings)
                    {
                        int msgboxID = MessageBox(NULL, L"This mod recommends DOW2.exe to be large address aware and allocate more than 2gb of address space. Would you like to apply the large address aware patch to DOW2.exe?", L"Warning", MB_YESNO | MB_ICONWARNING);
                        if (msgboxID == IDYES)
                        {
                            if (!ApplyLargeAddressAwarePatch(APP_NAME))
                            {
                                MessageBox(NULL, L"Failed to apply the large address aware patch to DOW2.exe. While this mod will still proceed to launch, you may crash in large scenarios. Try again, or apply it manually.", L"Warning", MB_OK | MB_ICONWARNING);
                            }
                        }
                    }
                }
            }

            if (!config.LAAPatch && Is32BitApplication(APP_NAME))
            {
                if (IsLargeAddressAware(APP_NAME))
                {
                    if (config.Warnings)
                    {
                        int msgboxID = MessageBox(NULL, L"This mod recommends against DOW2.exe being large address aware and allocating more than 2gb of address space. Would you like to unapply the large address aware patch to DOW2.exe?", L"Warning", MB_YESNO | MB_ICONWARNING);
                        if (msgboxID == IDYES)
                        {
                            if (!UnapplyLargeAddressAwarePatch(APP_NAME))
                            {
                                MessageBox(NULL, L"Failed to unapply the large address aware patch from DOW2.exe. The launch will proceed, but you should unapply the large address aware patch manually next time, or let the launcher try again.", L"Warning", MB_OK | MB_ICONWARNING);
                            }
                        }
                    }
                }
            }

            if (config.VerboseDebug)
            {
                MessageBox(NULL, L"Verified large address aware.", L"Debug", MB_OK | MB_ICONINFORMATION);
            }

            if (config.LAAPatch)
            {
                CONSOLE_MESSAGE(L"Large address aware check.");
            }

            // check for the compatibility mode
            if (config.WIN7CompatibilityMode)
            {
                if (!IsWindows7OrEarlier())
                {
                    if (!CheckCompatibilityMode(APP_NAME))
                    {
                        if (config.Warnings)
                        {
                            int msgboxID = MessageBox(NULL, L"This mod recommends DOW2.exe to be set to the Windows 7 compatibility mode, WIN7RTM. Would you like to set DOW2.exe to the Windows 7 compatibility mode?", L"Warning", MB_YESNO | MB_ICONWARNING);
                            if (msgboxID == IDYES)
                            {
                                if (!SetCompatibilityMode(APP_NAME))
                                {
                                    MessageBox(NULL, L"Failed to set DOW2.exe compatibility mode to WIN7RTM. The launch will proceed, but you should set the compatibility mode manually next time, or let the launcher try again.", L"Warning", MB_OK | MB_ICONWARNING);
                                }
                            }
                        }
                    }
                }
            }

            if (!config.WIN7CompatibilityMode)
            {
                if (!IsWindows7OrEarlier())
                {
                    if (CheckCompatibilityMode(APP_NAME))
                    {
                        if (config.Warnings)
                        {
                            int msgboxID = MessageBox(NULL, L"This mod recommends against DOW2.exe being set to the Windows 7 compatibility mode, WIN7RTM. Would you like to unset the Windows 7 compatibility mode from DOW2.exe?", L"Warning", MB_YESNO | MB_ICONWARNING);
                            if (msgboxID == IDYES)
                            {
                                if (!RemoveWin7RtmCompatibilityMode(APP_NAME))
                                {
                                    MessageBox(NULL, L"Failed to unset the WIN7RTM compatibility mode. The launch will proceed, but you should unset the compatibility mode manually next time, or let the launcher try again.", L"Warning", MB_OK | MB_ICONWARNING);
                                }
                            }
                        }
                    }
                }
            }

            if (config.VerboseDebug)
            {
                MessageBox(NULL, L"Verified compatibility.", L"Debug", MB_OK | MB_ICONINFORMATION);
            }

            CONSOLE_MESSAGE(L"Compatibility check.");

            // check game settings for UI incompatibilities, errors are handled in the function
            if (config.UIWarnings)
            {
                CheckGameConfiguration(config);
            }

            if (config.VerboseDebug)
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

                // read mod-folder from the .config file
                std::wstring configFilePath = rootDir + L"\\" + configFileName;
                std::wstring modFolder = ReadModFolderFromConfig(configFilePath);

                std::wstring modFolderPath = rootDir + L"\\" + modFolder;
                if (!InjectedFilesPresent(modFolderPath, config.InjectedFiles))
                {
                    return 1; // error message is handled in InjectedFilesPresent
                }

                if (!InjectedConfigurationsPresent(modName, config.InjectedConfigurations))
                {
                    return 1; // error message is handled in InjectedConfigurationsPresent
                }

                if (GetFileAttributes(config.InjectorFileName.c_str()) == INVALID_FILE_ATTRIBUTES)
                {
                    std::wstring binFileName = baseLauncherName + L"_" + config.InjectorFileName.substr(0, config.InjectorFileName.find_last_of(L".")) + L".bin";
                    if (!CopyFileRaw(binFileName.c_str(), config.InjectorFileName.c_str()))
                    {
                        MessageBox(NULL, (L"Failed to create or replace the injector file required by this mod. The " + binFileName + L" file may be missing. Reacquire it from the mod package, or try again.").c_str(), L"Error", MB_OK | MB_ICONERROR);
                        return 1;
                    }
                }
                
                if (!InjectorBinaryProcessing(config, baseLauncherName))
                {
                    return 1;
                }
            }

            if (config.VerboseDebug)
            {
                MessageBox(NULL, L"Verified injector.", L"Debug", MB_OK | MB_ICONINFORMATION);
            }

            if (config.Injector)
            {
                CONSOLE_MESSAGE(L"Injector check.");
            }

            if (!CheckAdditionalFiles(config, baseLauncherName))
            {
                return 1;
            }

            if (config.VerboseDebug)
            {
                MessageBox(NULL, L"Verified additional files.", L"Debug", MB_OK | MB_ICONINFORMATION);
            }

            // check if the .module file exists in the same directory
            if (GetFileAttributes(moduleFileName.c_str()) == INVALID_FILE_ATTRIBUTES)
            {
                MessageBox(NULL, (L"Failed to find this mod's " + moduleFileName + L" module file. Reacquire it from the mod package, or try again.").c_str(), L"Error", MB_OK | MB_ICONERROR);
                return 1;
            }

            if (!CheckModuleFile(moduleFileName, config))
            {
                return 1;
            }

            if (config.VerboseDebug)
            {
                MessageBox(NULL, L"Verified module.", L"Debug", MB_OK | MB_ICONINFORMATION);
            }

            CONSOLE_MESSAGE(L"Module check.");

            // call the UCS file validation function
            if (!ValidateUCSFiles(rootDir))
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
            if (GetTickCount64() - startTime > TIMEOUT_PROCESS) // timeout check
            {
                MessageBox(NULL, L"Launcher process timed out before DOW2.exe was executed.", L"Error", MB_OK | MB_ICONERROR);
                if (hwndBitmap)
                {
                    DestroyWindow(hwndBitmap);
                }
                return 1;
            }
            
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
                    if (GetTickCount64() - startTime > TIMEOUT_PROCESS) // timeout check
                    {
                        MessageBox(NULL, L"Launcher process timed out while waiting for the main window handle of DOW2.exe.", L"Error", MB_OK | MB_ICONERROR);
                        if (hwndBitmap)
                        {
                            DestroyWindow(hwndBitmap);
                        }
                        return 1;
                    }

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

                // convert strings to wstrings for comparison and MessageBox
                std::wstring actualCommandLineW = StringToWString(actualCommandLine);
                std::wstring expectedCommandLineW = StringToWString(expectedCommandLine);

                if (actualCommandLineW != expectedCommandLineW)
                {
                    if (config.Warnings)
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
        }

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