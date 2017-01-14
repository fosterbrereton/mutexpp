@ECHO OFF

set VC_ROOT=%ProgramFiles%\Microsoft Visual Studio 14.0\VC\

if exist "%VC_ROOT%bin\VCVARS32.BAT" (
    call "%VC_ROOT%bin\VCVARS32.BAT"
)

MKDIR build_debug
PUSHD build_debug

conan install .. --build=missing -s build_type=Debug -s compiler="Visual Studio" -s compiler.runtime="MDd"

cmake -G "Visual Studio 14 Win64" ..

POPD

MKDIR build_release
PUSHD build_release

conan install .. --build=missing -s build_type=Release -s compiler="Visual Studio" -s compiler.runtime="MDd"

cmake -G "Visual Studio 14 Win64" ..

POPD
