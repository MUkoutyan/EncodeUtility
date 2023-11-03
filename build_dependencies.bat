

REM Visual Studio セットアップコマンド
SET VS_PATH="C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat"
call %VS_PATH% x64

SET QT_PATH=C:\Qt\6.5.2\msvc2019_64

REM zlibのソースとビルドディレクトリのパス
SET ZLIB_SRC_PATH=%~dp0zlib
SET ZLIB_BUILD_PATH=%~dp0zlib\build


if not exist .\lib (
    mkdir .\lib
)

cd %ZLIB_SRC_PATH%
del CMakeCache.txt
rmdir /s /q CMakeFiles
if not exist %ZLIB_BUILD_PATH% (
    mkdir %ZLIB_BUILD_PATH%
)

cd %ZLIB_BUILD_PATH%
cmake .. 
cmake --build . --config RELEASE

set LIB_DIR=%~dp0lib

echo %LIB_DIR%

copy .\zconf.h "%ZLIB_SRC_PATH%\zconf.h"
copy .\Release\zlibstatic.lib "%LIB_DIR%\zlibstatic.lib"

REM QuaZipのソースとビルドディレクトリのパス
SET QUAZIP_SRC_PATH=%~dp0quazip
SET QUAZIP_BUILD_PATH=%~dp0quazip\build

cd %QUAZIP_SRC_PATH%
del CMakeCache.txt
rmdir /s /q CMakeFiles
if not exist %QUAZIP_BUILD_PATH% (
    mkdir %QUAZIP_BUILD_PATH%
) 
cd %QUAZIP_BUILD_PATH%
cmake %QUAZIP_SRC_PATH% -DCMAKE_PREFIX_PATH=%QT_PATH% -DQUAZIP_QT_MAJOR_VERSION=6 -DZLIB_LIBRARY=%OUTPUT_DIR%/zlibstatic.lib -DZLIB_INCLUDE_DIR=%ZLIB_SRC_PATH%
cmake --build . --config DEBUG
copy .\quazip\Debug\quazip1-qt6d.lib "%LIB_DIR%\quazip1-qt6d.lib"
copy .\quazip\Debug\quazip1-qt6d.dll "%LIB_DIR%\quazip1-qt6d.dll"

cmake %QUAZIP_SRC_PATH% -DCMAKE_PREFIX_PATH=%QT_PATH% -DQUAZIP_QT_MAJOR_VERSION=6 -DZLIB_LIBRARY=%OUTPUT_DIR%/zlibstatic.lib -DZLIB_INCLUDE_DIR=%ZLIB_SRC_PATH%
cmake --build . --config RELEASE
copy .\quazip\Release\quazip1-qt6.lib "%LIB_DIR%\quazip1-qt6.lib"
copy .\quazip\Release\quazip1-qt6.dll "%LIB_DIR%\quazip1-qt6.dll"

exit /b 0