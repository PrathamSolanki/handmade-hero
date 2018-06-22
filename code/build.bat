@echo off

pushd ..\build
call vcvarsall.bat x64
cl -FC -Zi ..\code\win32_handmade.cpp user32.lib gdi32.lib
popd