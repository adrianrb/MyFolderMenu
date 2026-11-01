@echo off
setlocal EnableExtensions
cd /d "%~dp0"

set "CMAKE_EXE="
where cmake >nul 2>nul && set "CMAKE_EXE=cmake"

set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" (
    echo vswhere.exe nao encontrado. Instale o Visual Studio com C++.
    exit /b 1
)

set "VSPATH="
for /f "usebackq delims=" %%i in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "VSPATH=%%i"
if not defined VSPATH (
    echo Visual Studio com ferramentas C++ x64 nao encontrado.
    exit /b 1
)

if not defined VCINSTALLDIR (
    call "%VSPATH%\VC\Auxiliary\Build\vcvars64.bat" >nul
    if errorlevel 1 (
        echo Falha ao inicializar o ambiente do Visual Studio.
        exit /b 1
    )
)

if not defined CMAKE_EXE (
    if exist "%VSPATH%\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" (
        set "CMAKE_EXE=%VSPATH%\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
    )
)
if not defined CMAKE_EXE (
    echo cmake.exe nao encontrado no PATH nem no Visual Studio.
    exit /b 1
)
if exist "%VSPATH%\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja" (
    set "PATH=%VSPATH%\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja;%PATH%"
)

echo Configurando build-msvc...
"%CMAKE_EXE%" -S . -B build-msvc -G Ninja -DCMAKE_BUILD_TYPE=Release -DMFM_BUILD_TESTS=ON
if errorlevel 1 exit /b 1

echo Compilando...
"%CMAKE_EXE%" --build build-msvc
if errorlevel 1 exit /b 1

echo.
echo OK: %~dp0build-msvc\MyFolderMenu.exe
exit /b 0
