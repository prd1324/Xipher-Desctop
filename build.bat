@echo off
REM ─────────────────────────────────────────────────────────────────────────
REM  Сборка Xipher Desktop (Qt6 / MinGW из MSYS2). Запускать обычным cmd.
REM  Тулчейн ставится один раз:
REM    winget install MSYS2.MSYS2
REM    C:\msys64\usr\bin\bash -lc "pacman -S --needed --noconfirm \
REM      mingw-w64-x86_64-gcc mingw-w64-x86_64-cmake mingw-w64-x86_64-ninja \
REM      mingw-w64-x86_64-qt6-base mingw-w64-x86_64-qt6-tools"
REM ─────────────────────────────────────────────────────────────────────────
setlocal
set MINGW=C:\msys64\mingw64
set PATH=%MINGW%\bin;%PATH%

cmake -S "%~dp0" -B "%~dp0build" -G Ninja ^
  -DCMAKE_BUILD_TYPE=Release ^
  -DCMAKE_PREFIX_PATH=%MINGW%
if errorlevel 1 exit /b 1

cmake --build "%~dp0build"
if errorlevel 1 exit /b 1

echo.
echo Готово: %~dp0build\bin\Xipher.exe
echo Для запуска нужен PATH к Qt DLL: set PATH=%MINGW%\bin;%%PATH%%
endlocal
