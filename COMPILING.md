> [!NOTE]
> Windows is the only supported platform, just to avoid confusions.

# Sources
Checkout the recent stable sources from the master branch including all the third-party submodules.
```
git clone --recurse-submodules https://github.com/maliavko/spotifar.git
```
> [!NOTE]
> Besides the necessary libraries for plugin compiling, the repo contains a Far Manager submodule for convenient debugging

# Plugin
### CMake
- Download and install [CMake](https://cmake.org/download/). The recommended version is 3.31.8

### MSVC Toolchain
- install [MS Build and Win SDKs](https://visualstudio.microsoft.com/downloads/). The list of the components with their minimum required versions needed for compiling:
  - MSBuild
  - MSVC v143 VS 2022
  - C++ core features
  - Windows 10 SDK
  - [optional] [Rust](https://www.rust-lang.org/tools/install) for building librespot

### Msys2/Clang64 toolchain
- install msys2
- open msys2 clang64 terminal:
  - install all the updates: "pacman -Syu"
  - install clang toolchain: "pacman -S mingw-w64-clang-x86_64-toolchain"
  - install ninja generator: "pacman -S mingw-w64-clang-x86_64-ninja"
- add your "C:\msys64\clang64\bin" to the system environment PATH
> [!NOTE]
> the WinToastLib does not provide a clang version, so the final binary will not have all the pop-up notifications functionality

### Compiling
1. to configure (x64 example, build librespot, install folder is "./install/vs17-x64)
```
cmake --preset vs17-x64 -DBUILD_LIBRESPOT=1 -DCMAKE_INSTALL_PREFIX=./install/vs17-x64
```
2. to build (x64-Release example)
```
cmake --build --preset vs17-x64-Release --target install
```
> [!NOTE]
> to list all available configure presets
> ```
> cmake --list-presets
> ```
> to list all available build presets
> ```
> cmake --build --list-presets
> ```
3. the resulting artifact can be found in the ".\build" directory. For the "x64-Release" preset for example it would be ".\build\x64\bin\Release\*"
# Far Manager
For the debugging purposes it could be convenient to have a Far Manager debug build. It is possible to do that right from the repo sources, as they include Far Manager's ones as a third-party submodule. The building procedure is performed via MSBuild, so the one way to get to it is to use "VsDevCmd". Assuming the command is launched from the project's root, the command for building Far without plugins in Debug x64 mode would be:
```
"C:/Program Files/Microsoft Visual Studio/2022/Community/Common7/Tools/VsDevCmd.bat" & msbuild .\thirdparty\FarManager\_build\vc\all.sln /t:Far\Far /p:Configuration=Debug /p:Platform=x64
```
The resulting artifact can be found in ".\thirdparty\FarManager\_build\vc\_output\product\*"
# Librespot
You can also build a Librespot from the plugin's repo sources, as they are included as a submodule as well. For custom Librespot compiling visit an appropriate web-page [instruction](https://github.com/librespot-org/librespot/wiki/Compiling). For the quick release build use the following command fromt he plugin's root folder:
```
cargo build --release --manifest-path=.\thirdparty\librespot\Cargo.toml
```
The artifact can be found in ".\thirparty\librespot\target\*"
# VS Code pipeline
The project originally configured for the development in VS Code, so it is recommended way to interact with the sources and building the libraries.
1. After sources checkout just open the root folder with the VS Code "Open Folder" menu
2. Open Extensions menu (Ctrl+Shift+X) and install CMake Tools plugin
3. Open CMake tools plugin and select a desired "Configure" and "Build" presets
4. Now the plugin can be already built via "F7" hotkey
To complete a build & debug routine you can build all the dependencies with the predefined tasks "Build all":
1. Open up the dropdown tasks menu via menu "Help" -> "Show all commands" (Ctrl+Shift+P)
2. Select "Tasks: Run Task" -> "Build all" -> select desired configuration and platform (default is debug x64). The task will build and configure Far Manager, Librespot and Spotifar itself. A plugin folder is installed into Far Manager via junction, so any further sources updates and rebuilds will be automatically seen in the debugging launches
3. Once finished you'll be able to launch a debugging via menu "Run" -> "Start Debugging" (F5), which will launch Far Manager, with installed Spotifar plugin
