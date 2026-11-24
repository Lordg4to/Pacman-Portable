@echo off
REM Build script for PAC-MAN C++
REM Compiles with MSVC in Release mode with optimizations

setlocal

REM Find MSVC installation
set "VC_PATH="
for /f "delims=" %%a in ('dir /b /ad "C:\Program Files\Microsoft Visual Studio" 2^>nul') do (
    if exist "C:\Program Files\Microsoft Visual Studio\%%a\Community\VC\Auxiliary\Build\vcvarsall.bat" (
        set "VC_PATH=C:\Program Files\Microsoft Visual Studio\%%a\Community\VC\Auxiliary\Build\vcvarsall.bat"
    ) else if exist "C:\Program Files\Microsoft Visual Studio\%%a\Professional\VC\Auxiliary\Build\vcvarsall.bat" (
        set "VC_PATH=C:\Program Files\Microsoft Visual Studio\%%a\Professional\VC\Auxiliary\Build\vcvarsall.bat"
    ) else if exist "C:\Program Files\Microsoft Visual Studio\%%a\Enterprise\VC\Auxiliary\Build\vcvarsall.bat" (
        set "VC_PATH=C:\Program Files\Microsoft Visual Studio\%%a\Enterprise\VC\Auxiliary\Build\vcvarsall.bat"
    )
)

if not defined VC_PATH (
    for /f "delims=" %%a in ('dir /b /ad "C:\Program Files (x86)\Microsoft Visual Studio" 2^>nul') do (
        if exist "C:\Program Files (x86)\Microsoft Visual Studio\%%a\Community\VC\Auxiliary\Build\vcvarsall.bat" (
            set "VC_PATH=C:\Program Files (x86)\Microsoft Visual Studio\%%a\Community\VC\Auxiliary\Build\vcvarsall.bat"
        ) else if exist "C:\Program Files (x86)\Microsoft Visual Studio\%%a\Professional\VC\Auxiliary\Build\vcvarsall.bat" (
            set "VC_PATH=C:\Program Files (x86)\Microsoft Visual Studio\%%a\Professional\VC\Auxiliary\Build\vcvarsall.bat"
        ) else if exist "C:\Program Files (x86)\Microsoft Visual Studio\%%a\Enterprise\VC\Auxiliary\Build\vcvarsall.bat" (
            set "VC_PATH=C:\Program Files (x86)\Microsoft Visual Studio\%%a\Enterprise\VC\Auxiliary\Build\vcvarsall.bat"
        )
    )
)

if not defined VC_PATH (
    echo ERROR: No se encontro Visual Studio con MSVC.
    echo Instale Visual Studio 2022 Community (gratuito) con la carga de trabajo "Desarrollo para el escritorio con C++".
    exit /b 1
)

echo Usando: %VC_PATH%

REM Set up MSVC environment for x64
call "%VC_PATH%" x64

REM Create dist directory
if not exist dist mkdir dist

REM Compile with optimizations
echo Compilando PAC-MAN en modo Release...
cl /O2 /GL /EHsc /std:c++17 /DNOMINMAX /W4 /wd4244 /wd4267 /wd4100 /wd4189 ^
   src\pacman.cpp /link /OPT:REF /OPT:ICF /SUBSYSTEM:CONSOLE /OUT:dist\Pacman.exe winmm.lib

if %errorlevel% neq 0 (
    echo.
    echo ERROR: La compilacion fallo.
    exit /b 1
)

echo.
echo Compilacion exitosa!
echo Ejecutable generado: dist\Pacman.exe

REM Show file size
for %%f in (dist\Pacman.exe) do (
    echo Tamano: %%~zf bytes (%%~zf / 1024 KB)
)

echo.
echo Para ejecutar: dist\Pacman.exe
echo Para recompilar: build.bat