@echo off

pushd ..\build
cl -Zi ..\code\win32_handmade.cpp user32.lib
popd