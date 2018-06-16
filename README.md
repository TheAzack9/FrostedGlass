This is a forked copy of the Rainmeter plugin repo and is just a simple configuration to make it easier to build plugins for c++. I have removed the C# dependencies and examples the came with the original project.
If you are new to creating plugins do i suggest starting out with the plugin repo that is officially created from the rainmeter team. The documentation might not be of much help, but i still recommend reading it: [documentation](https://github.com/rainmeter/rainmeter-plugin-sdk/wiki/C---plugin-API).

The things that is different with this project is a few things:
*   I have added some resource variables, so instead of having to change plugin version and copyright information in the rc file can you simply change it in version.h and it will be fetched from there. I find the rc file can be weird at times and prefer this myself.
*   The plugin name is the name of the project itself, so to rename the plugin do you only need to rename the project in visual studio. Note that i say project and not Solution!
*   I added a pre build step to cleanup the old build, this is just something i prefer myself since i can find myself renaming the plugin and it just saves me a small delete. ;)
*   I have a custom build script that assembles the final skin with a redistributable and an example skin.

Before release:
*   Tweak variables in version.h in order to reflect the changes (Remember, you are not allowed to build unless you change version!)
*   Update changelog
*   Build

## How to build the project
*   Run build.bat and make sure that msbuild is in your environment variables (it will search for it in C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\MSBuild\15.0\Bin\msbuild.exe)
*   You should find the built binaries in Release/#CurrentVersion# with a build example rmskin to accompany it

### Note:
Changing names of folders etc isn't easy as of now, so renaming folders might break everything.

See the original Rainmeter plugin repo for additional information:
[Repo](https://github.com/rainmeter/rainmeter-plugin-sdk)