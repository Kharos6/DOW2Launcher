+++DOW2 STANDALONE LAUNCHER README+++

======

**SETUP INSTRUCTIONS**

- Put the packaged files into your desired Dawn of War II game directory.

- Change the "Launcher" part of the file names to match the name of your mod's .module file.

- Edit the [LaunchParams] field of the .launchconfig file to match your desired mod configuration. You don't need to insert the -modname launch parameter, as the launcher automatically launches the game with the -modname launch parameter that corresponds to the name of the launcher.

- Set the [IsSteam] field of the .launchconfig file to false if your mod is for the GOG distribution of the game, or true if your mod is for the Steam distribution of the game.

- Set the [IsRetribution] field of the .launchconfig file to false if your mod is for Dawn of War II - Chaos Rising, or true if your mod is for Dawn of War II - Retribution.

- If using an injector, set the [Injector] field of the .launchconfig file to true, and set the [InjectorFileName] field of the .launchconfig file to match the name of your injector file, including its file extension; this field can remain blank if you are not using an injector. Populate the [InjectedFiles] field of the .launchconfig file with the full names, including the DLL extension, of the files contained within the folder declared in the mod-folder field of the injector's .config file. The files listed must be separated by a comma and a space; this field can remain blank if you are not injecting anything. Populate the [InjectedConfigurations] field of the .launchconfig file with the extensions of configuration files unique to certain injections. The extensions files listed must start with a period, and be separated by a comma and a space; this field can remain blank if you are not injecting anything. Finally, duplicate your current injector .dll file, and change the duplicate file's extension to .bin, while changing its name to match the name of your .module file before the suffix name of the injector file, with the suffix starting with an underscore. This duplicate .bin file is used as a baseline for replacing a faulty or non-injector version of the injector file.

- If not using an injector, and the mod is for the Steam distribution of the game, it is advised to package the GOG distribution of the XThread.dll file with your mod in order to allow the game to be played on CPUs with more than twelve cores. Duplicate the XThread.dll file, and change the duplicate file's extension to .bin, while changing its name to match the name of your .module file before the _XThread suffix. This duplicate .bin file is used as a baseline for replacing the outdated version of the XThread.dll file.

- Optionally, set the [FirstTimeLaunchMessage] field of the .launchconfig file to your desired welcoming message for people who are launching the mod for the first time, and set the [FirstTimeLaunchCheck] field of the .launchconfig file to true if you wish for it to display on first launch. This boolean is automatically switched to false after the user has executed the launcher for the first time.

- Optionally, insert the file version of the DOW2.exe that the mod is created for into the [GameVersion] field of the .launchconfig file. Leaving the field blank will ignore the version check.

- Optionally, set the [IsDXVK] field of the .launchconfig file to true if your mod requires DXVK. If true, ensure that you package the d3d9.dll and dxvk.conf files with your mod, and that you duplicate the d3d9.dll file, then change the name of the duplicate file's extension to .bin, while changing its name to match the name of your .module file before the _d3d9 suffix. Additionally, the specific provided copies of DivxDecoder and DivxMediaLib files should also be packaged alongside their duplicate .bin equivalents with the appropriate naming convention. The DIVX files are there in order to avoid a crashing issue relating to in-game movies with the DXVK and injector combination. These duplicate .bin files are used as a baseline for replacing faulty or non-DXVK file equivalents.

- Optionally, set the [LAAPatch] field of the .launchconfig file to true if your mod requires DOW2.exe to be large address aware.

- Optionally, set the [UIWarnings] field of the .launchconfig file to true if your mod makes UI modifications that require the uiscale to be set to 100, and resolution to be of the 16:9 aspect ratio.

- Optionally, if you want a splash screen, create a .bmp file with your desired artwork and resolution, and change its name to match the name of your mod's .module file, then change the [BitmapWidth] and [BitmapHeight] fields of the .launchconfig file to match its resolution. Those fields can remain at any number value if you are not using a splash screen.

- Optionally, if your mod requires additional files, populate the [AdditionalFiles] field of the .launchconfig file with the full names, including extensions, of the additional files, separated by a comma and a space. These files will be verified through equivalently named .bin file whose name must contain a prefix name equivalent to your .module file, followed by an underscore. Leaving the field blank will ignore the additional file check.

- Optionally, set the [WIN7CompatibilityMode] field of the .launchconfig file to true if your mod recommends DOW2.exe to be set to the Windows 7 compatibility mode.

- If you wish to receive detailed debug messages, set the [VerboseDebug] field of the .launchconfig file to true.

- If you wish to skip most checks, errors, and warnings, set the [IsUnsafe] field of the .launchconfig file to true.

- If you wish to only skip warnings that do not prevent the game from launching, set the [Warnings] field of the .launchconfig file to false.

- If you wish to see the console window regardless of whether a bitmap exists or not, set the [Console] field of the .launchconfig file to true.


**FEATURES**

- If DOW2.exe is not found, or is already running at the time of initializing the launcher, a warning is displayed and the entire process is aborted.

- If the .launchconfig file is not found, a warning is displayed and the entire process is aborted.

- If another instance of the launcher is already running at the time that the user decides to execute the launcher, a warning is displayed and the entire process is aborted.

- If the .launchconfig file has misspelled or missing variables or variable entries, a warning is displayed and the entire process is aborted.

- If the [Injector] field of the .launchconfig file is set to true, and the injector file doesn't exist or match the provided checksum, it replaces the injector file with the binary data of the injector's .bin file, and then launches the game. If the replacement fails, or the injector file is not found, a warning is displayed and the entire process is aborted.

- If the [Injector] field of the .launchconfig file is set to true, and the injector's .config file is missing, a warning is displayed and the entire process is aborted.

- If the [Injector] field of the .launchconfig file is set to true, and the injector's designated mod folder is missing, or it's missing the .dll files listed in the [InjectedFiles] field of the .launchconfig file, a warning is displayed and the entire process is aborted.

- If the [Injector] field of the .launchconfig file is set to false, and the [IsSteam] field of the .launchconfig file is set to true, and the local XThread.dll file doesn't exist or match the provided checksum for the version that fixes the game's behavior with twelve core CPUs, it replaces the XThread.dll file with the binary data of the XThread.bin file, and then launches the game. If the replacement fails, or the XThread.dll file is not found, a warning is displayed and the entire process is aborted.

- If the [InjectedConfigurations] field of the .launchconfig file is populated, and files with those extensions don't exist, a warning is displayed and the entire process is aborted.

- If the .module file of the same name as the launcher is missing, a warning is displayed and the entire process is aborted.

- If any of the .sga archives required by the mod are missing, a warning is displayed and the entire process is aborted. Locale .sga files are checked based on which language folders exist, and whether they contain a DOW2.ucs file. If only one language folder exists, and it contains a DOW2.ucs file, then it verifies that the corresponding .sga archives exist for that language. If multiple language folders exist, and more than one language folder contains a DOW2.ucs file, we ignore checking the existence of .sga archives under the entire Locale folder, as this is typical of dev builds that may contain multiple languages, and we should not cause errors for those.

- If the [IsDXVK] field of the .launchconfig file is set to true, and the dxvk.conf or d3d9.dll files are missing, or the d3d9.dll file is not from DXVK, a warning is displayed, and the user can either acquire DXVK or continue with the launch. Additionaly, the DIVX files are checked for compatibility if the DXVK d3d9.dll file exists.

- If the [IsDXVK] field of the .launchconfig file is set to false, and the d3d9.dll file specific to DXVK is present, a warning is displayed, and the user can either delete DXVK or continue with the launch.

- If the [LAAPatch] field of the .launchconfig file is set to true, a warning is displayed, and the user can either set DOW2.exe to be large address aware, or continue with the launch.

- If the [UIWarnings] field of the .launchconfig file is set to true, and the user has their game settings using a ui scale lesser than 100, and an aspect ratio that is not 16:9, a warning is displayed, but the process continues.

- If the [GameVersion] field of the .launchconfig file is populated, and the DOW2.exe file version doesn't match it, a warning is displayed, but the process continues.

- If the user tries to close the injector before it finishes its operations, a warning is displayed, advising against doing so, but giving the option to proceed or exit.

- If any .ucs files have incorrect formatting, or non-number entries, a warning is displayed and the entire process is aborted. Additionally, if a .ucs file is not UTF-16 LE, an attempt will be made to convert the file to UTF-16 LE. If this attempt fails, a warning is displayed and the entire process is aborted.

- If there are no files under the Locale folder, a warning is displayed and the entire process is aborted.

- If the [IsDXVK] field of the .launchconfig file is set to true, and the launcher detects no Vulkan capable GPU, a warning is displayed, but the process continues.

- If the [AdditionalFiles] field of the .launchconfig file is populated, and those files are invalid or missing, the launcher will attempt to recover them using their .bin equivalents. If the .bin equivalents are not found, a warning is displayed and the entire process is aborted.

- If the [WIN7CompatibilityMode] field of the .launchconfig file is true, a warning is displayed, and the user can either set DOW2.exe to the WIN7RTM compatibility mode, or continue with the launch.

- If the launcher detects that DOW2.exe is using external launch parameters, a warning is displayed, but the process continues.

- Compatibility with Linux, though currently severely limited in functionality.

- Various errors and warnings for failed operations.

- A console window that appears if a .bmp file doesn't exist.

- Timeout after 30 seconds following the launch of the game in order to avoid hogging up resources or causing other issues.

- Timeout starting after the game launches that checks if DOW2.exe is closed while the launcher is running.

- Absolute timeout after 60 seconds of launcher execution in order to avoid hogging up resources or causing other issues.

- High CPU priority set for DOW2.exe.

- Runs with elevated privileges.

- Detailed error and debug messages.