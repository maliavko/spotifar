
git clone --depth 1 https://github.com/FarGroup/FarManager build

cd build/far/
"C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" & build.bat vc 64 debug

:: configuring far
cd Debug.64.vc
mkdir Plugins

