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
### MSVC Toolchain
- Download and install [CMake](https://cmake.org/download/). The recommended version is 3.31.8
- install [MS Build and Win SDKs](https://visualstudio.microsoft.com/downloads/). The list of the components with their minimum required versions needed for compiling:
  - MSBuild
  - MSVC v143 VS 2022
  - C++ core features
  - Windows 10 SDK

### Msys2/Clang64 toolchain
- install msys2
- open msys2 clang64 terminal:
  - install all the updates: "pacman -Syu"
  - install clang toolchain: "pacman -S mingw-w64-clang-x86_64-toolchain"
  - install cmake generator: "pacman -S mingw-w64-clang-x86_64-cmake"

### Msys2/Mingw64 toolchain
- install msys2
- open msys2 mingw64 terminal:
  - install all the updates: "pacman -Syu"
  - install gcc toolchain: "pacman -S mingw-w64-x86_64-toolchain"
  - install cmake generator: "pacman -S mingw-w64-x86_64-cmake"

> [!NOTE]
> the WinToastLib does provide only MSVC version, the projects will still compile, but notification pop-ups will not be available

### [Optional] Librespot
In case you want the project to be built together with Librespot executable, you need:
- Download and install [Rust](https://www.rust-lang.org/tools/install)
- configure the project to start Cargo right after the library compilation
  - open CMakePresets.json file and set the "BUILD_LIBRESPOT" variable to "1"

If the Cargo is found in the system environment during the compilation process, Cargo will be launched automatically.

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
For the debugging purposes it could be convenient to have a Far Manager debug build. It is possible to do that right from the repo sources, as they include Far Manager's ones as a third-party submodule. VS Code tasks are preconfigured with the ones to compile Far from sources via MSBuild or Clang: Ctrl+Shift+P -> "Tasks: Run Task":
- "Far MSBuild" - the binraries will be found at "./thirdparty/FarManager/_build/vc/_output/product/Release.x64"
- "Far Msys2/Clang64" - "thirdparty/FarManager/far/Release.64.clang"

To make VS Code storing your freshly compiled library binaries into appropriate folder, modify CMakePresets.json file, adding attribute "installDir" apointed to "${sourceDir}/thirdparty/FarManager/far/Release.64.clang/Plugins/Spotifar" for Clang version e.g.

# VS Code pipeline
The project originally configured for the development in VS Code, so it is recommended way to interact with the sources and building the libraries.
1. After sources checkout just open the root folder with the VS Code "Open Folder" menu
2. Open Extensions menu (Ctrl+Shift+X) and install CMake Tools plugin
3. Open CMake tools plugin and select a desired "Configure" and "Build" presets
4. Now the plugin can be already built via "F7" hotkey

To complete a build & debug routine you can build all the dependencies with the predefined tasks "Build all":
1. Compile Far Manager as described above and specify "installDir" folder
2. Select Cmake -> Build -> "install" and compile library via F7
3. Select launch task on the VS Code "Run and Debug" (Ctrl+Shift+D) page: lldp-dap for Clang and VS dbg for msvc, hit F5