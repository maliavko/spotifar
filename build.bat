@echo off

:::: Preparing a build folder
if not exist build mkdir build

:::: Preparing a toolchain
if not exist vcpkg (
	git clone https://github.com/microsoft/vcpkg.git
	cd vcpkg && ./bootstrap-vcpkg.bat
	cd ..
)

cd ./build
cmake ..
cmake --build . --config Debug
