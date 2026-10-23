@echo off
rem Build cl_color.exe with MSVC (run from a Developer Command Prompt / after loading the MSVC environment).
cl /nologo /O2 /EHsc /std:c++17 cl_color.cpp /Fe:cl_color.exe
if errorlevel 1 exit /b 1
del /q cl_color.obj 2>nul
echo Built cl_color.exe
