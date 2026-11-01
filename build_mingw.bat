@echo off
setlocal EnableExtensions
cd /d "%~dp0"

where cmake >nul 2>nul
if errorlevel 1 (
    echo cmake.exe nao encontrado no PATH.
    exit /b 1
)
where g++ >nul 2>nul
if errorlevel 1 (
    echo g++ nao encontrado no PATH. Instale o MinGW-w64 ou use build_msvc.bat.
    exit /b 1
)

echo Configurando build-mingw...
cmake -S . -B build-mingw -G Ninja -DCMAKE_BUILD_TYPE=Release ^
      -DCMAKE_CXX_COMPILER=g++ -DCMAKE_RC_COMPILER=windres ^
      -DMFM_BUILD_TESTS=ON
if errorlevel 1 exit /b 1

echo Compilando...
cmake --build build-mingw
if errorlevel 1 exit /b 1

echo.
echo OK: %~dp0build-mingw\MyFolderMenu.exe
exit /b 0
