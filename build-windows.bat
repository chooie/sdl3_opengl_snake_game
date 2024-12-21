@echo off
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" x64

echo "Starting..."
echo %CD%

:: %~dp0 is relative to current file path
set BUILD_DIR=%~dp0build
IF NOT EXIST %BUILD_DIR% (
    mkdir %BUILD_DIR%
    echo Copying SDL3.dll to the build directory
    copy /Y %~dp0vendor\SDL3\windows\lib\x64\SDL3.dll build\
    echo Copying SDL3_mixer.dll to the build directory
    copy /Y %~dp0vendor\SDL3_mixer\windows\lib\x64\SDL3_mixer.dll build\
    echo Copying SDL3_tff.dll to the build directory
    copy /Y %~dp0vendor\SDL3_ttf\windows\lib\x64\SDL3_ttf.dll build\
)

pushd %BUILD_DIR%

rem Get the current time
set hour=%time:~0,2%
rem Remove the leading space for single-digit hours
if "%hour:~0,1%" == " " set hour=0%hour:~1,1%
rem Create the timestamp
set timestamp=%date:~-4,4%%date:~-10,2%%date:~-7,2%_%hour%%time:~3,2%%time:~6,2%
rem Set the filename
set filename=main%timestamp%.pdb

del %BUILD_DIR%\*.pdb > NUL 2> NUL
set SDL3__INCLUDE_PATH=%~dp0vendor\SDL3\windows\include
set SDL3__LIB_PATH=%~dp0vendor\SDL3\windows\lib\x64
set SDL3_MIXER__INCLUDE_PATH=%~dp0vendor\SDL3_mixer\windows\include
set SDL3_MIXER__LIB_PATH=%~dp0vendor\SDL3_mixer\windows\lib\x64
set SDL3_TTF__INCLUDE_PATH=%~dp0vendor\SDL3_ttf\windows\include
set SDL3_TTF__LIB_PATH=%~dp0vendor\SDL3_ttf\windows\lib\x64
set FREETYPE__INCLUDE_PATH=%~dp0vendor\freetype\windows\include
set FREETYPE__LIB_PATH=%~dp0vendor\freetype\windows\lib

cl ^
  /Zi ^
  /W4 ^
  /WX ^
  /wd4100 ^
  /wd4101 ^
  /wd4189 ^
  /wd4456 ^
  /wd4459 ^
  /MD ^
  /EHsc ^
  /D_CRT_SECURE_NO_WARNINGS ^
  %~dp0src\main.cpp ^
  %~dp0vendor\GLAD\src\glad.cpp ^
  /I %SDL3__INCLUDE_PATH% ^
  /I %SDL3_MIXER__INCLUDE_PATH% ^
  /I %SDL3_ttf__INCLUDE_PATH% ^
  /I %FREETYPE__INCLUDE_PATH% ^
  /I %~dp0vendor\GLAD\include ^
  /I %~dp0vendor\stb_image\include ^
  /I %~dp0vendor\glm\include ^
  /link ^
  /LIBPATH:%SDL3__LIB_PATH% ^
  /LIBPATH:%SDL3_MIXER__LIB_PATH% ^
  /LIBPATH:%SDL3_TTF__LIB_PATH% ^
  /LIBPATH:%FREETYPE__LIB_PATH% ^
  -PDB:%filename% ^
  SDL3.lib SDL3_mixer.lib SDL3_ttf.lib freetype.lib ^
  /SUBSYSTEM:CONSOLE
REM TODO: use /SUBSYSTEM:WINDOWS for release to avoid opening a console

REM Check if compilation was successful
if %errorlevel% neq 0 (
    echo Compilation failed.
    exit /b %errorlevel%
) else (
    echo Compilation succeeded.
)
popd

exit /b 0