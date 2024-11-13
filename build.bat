@echo off

git submodule update --init

:::: Preparing a build folder
if not exist build mkdir build

cd ./build && cmake ..
cmake --build . --config Debug
