+++DOW2 STANDALONE LAUNCHER README+++

======

**SETUP INSTRUCTIONS**

- Put the .exe and .launchconfig files into your Dawn of War II game directory.

- Change the names of both files to match the name of your mod's .module file.

- Edit the [LaunchParams] field of the .launchconfig file to match your desired mod configuration.

- Set the [IsSteam] field of the .launchconfig file to false if your mod is for the GOG distribution of the game, or true if your mod is for the Steam distribution of the game.

- Set the [IsRetribution] field of the .launchconfig file to false if your mod is for Dawn of War II - Chaos Rising, or true if your mod is for Dawn of War II - Retribution.

- Set the [IsDXVK] field of the .launchconfig file to false if your mod does not require DXVK, or true if your mod requires DXVK. If true, ensure that you package the d3d9.dll and dxvk.conf files with your mod, and that you duplicate the d3d9.dll file, then change the name of the duplicate file's extension to .bin. This duplicate .bin file is used as a baseline for replacing a non-DXVK d3d9.dll file.

- Optionally, create a .bmp file with your desired artwork and resolution, and change its name to match the name of your mod's .module file, then change the [BitmapWidth] and [BitmapHeight] fields of the .launchconfig file to match its resolution.

- If using an injector, set the [Injector] field of the .launchconfig file to true, and set the [InjectorFileName] field of the .launchconfig file to match the name of your injector file, including its file extension; this field can remain blank if you are not using an injector. Populate the [InjectedFiles] field of the .launchconfig file with the full names, including the DLL extension, of the files contained within the folder declared in the mod-folder field of the injector's .config file; this field can remain blank if you are not injecting anything. After that, create an MD5 checksum for it, and insert the value into the [ExpectedInjectorFileMD5Checksum] field of the .launchconfig file; this field can remain blank if you are not using an injector. Finally, duplicate your current injector .dll file, and change the duplicate file's extension to .bin. This duplicate .bin file is used as a baseline for replacing a faulty or non-injector version of the injector file.

- Set the [FirstTimeLaunchMessage] field of the .launchconfig file to your desired welcoming message for people who are launching the mod for the first time. The [FirstTimeLaunchCheck] field is automatically switched to false after the user has executed the launcher for the first time.

- If you wish to receive detailed debug messages, set the [VerboseDebug] field of the .launchconfig file to true.

- If you wish to skip errors or warnings about [IsSteam], [IsRetribution], [IsDXVK], [Injector], [InjectedFiles], and GPU checks, set the [IsUnsafe] field of the .launchconfig file to true.

- If you wish to see the console window regardless of whether a bitmap exists or not, set the [Console] field of the .launchconfig file to true.


**FEATURES**

- Failsafe for DOW2.exe; if DOW2.exe is not found, or if DOW2.exe is already running, a warning is displayed and the entire process is aborted.

- Failsafe for the launcher configuration file; if the .launchconfig file is not found, a warning is displayed and the entire process is aborted.

- Failsafe for multiple instances of the launcher; if another instance of the launcher is already running at the time that the user decides to execute the launcher, a warning is displayed and the entire process is aborted.

- Failsafe for the misspelled or missing variables in the .launchconfig file; if the .launchconfig file has misspelled or missing variables, a warning is displayed and the entire process is aborted.

- Failsafe for the injector file; if [Injector] is set to true, and the injector file doesn't exist or match the provided checksum, it replaces the injector file with the binary data of the .bin file, and then launches the game. If the replacement fails, or the injector file is not found, a warning is displayed and the entire process is aborted.

- Failsafe for the injector configuration file; if [Injector] is set to true, and the injector's .config file is missing, a warning is displayed and the entire process is aborted.

- Failsafe for the injector mod folder; if [Injector] is set to true, and the injector's designated mod folder is missing the .dll files listed in the [InjectedFiles] field of the .launchconfig file, a warning is displayed and the entire process is aborted.

- Failsafe for the .module file; if the .module file of the same name as the launcher is missing, a warning is displayed and the entire process is aborted.

- Failsafe for the DXVK; if [IsDXVK] is set to true, and the dxvk.conf or d3d9.dll files are missing, or the d3d9.dll file is not from DXVK, a warning is displayed, and the user can either recover DXVK or continue with the launch. If [IsDXVK] is set to false, and the d3d9.dll file specific to DXVK is present, a warning is displayed, and the user can either delete DXVK or continue with the launch.

- Failsafe for .sga archives loaded by the .module file; if any of the .sga archives required by the mod are missing, a warning is displayed and the entire process is aborted. Locale .sga files are checked based on which language folders exist, and whether they contain a DOW2.ucs file. If only one language folder exists, and it contains a DOW2.ucs file, then it verifies that the corresponding .sga archives exist for that language. If multiple language folders exist, and more than one language folder contains a DOW2.ucs file, we ignore checking the existence of .sga archives under the entire Locale folder, as this is typical of dev builds that may contain multiple languages, and we should not cause errors for those.

- Failsafe for closing the injector through the close button on the console window; if the user tries to close the injector before it finishes its operations, a warning is displayed, advising against doing so.

- Failsafe for invalid launch configuration file formatting; if any of the fields are formatted incorrectly, a warning is displayed and the entire process is aborted.

- Failsafe for invalid .ucs file formatting; if any .ucs files have incorrect formatting or line order, a warning is displayed and the entire process is aborted. Additionally, if a .ucs file is not UTF-16 LE, an attempt will be made to convert the file to UTF-16 LE. If this attempt fails, a warning is displayed and the entire process is aborted.

- Failsafe for localization folders; if there are no files under the Locale folder, a warning is displayed and the entire process is aborted.

- Failsafe for missing GPU; if the launcher detects no GPU, integrated or dedicated, a warning is displayed, but the process continues; likewise if [IsDXVK] is set to true, and the launcher detects no Vulkan capable GPU, a warning is displayed, but the process continues.

- Automatically launches the game with the -module launch parameter that corresponds to the name of the launcher.

- A console window that appears if a .bmp file doesn't exist.

- Timeout after 30 seconds following the launch of the game in order to avoid hogging up resources or causing other issues.

- Timeout starting after 15 seconds that checks if DOW2.exe is closed while the launcher is running.

- Absolute timeout after 60 seconds of launcher execution in order to avoid hogging up resources or causing other issues.

- High CPU priority set for DOW2.exe.

- Runs with elevated privileges.

- Detailed error and debug messages.
