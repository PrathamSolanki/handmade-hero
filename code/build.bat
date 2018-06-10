@echo off

pushd ..\build
call vcvarsall.bat x64
cl -Zi ..\code\win32_handmade.cpp user32.lib
popd