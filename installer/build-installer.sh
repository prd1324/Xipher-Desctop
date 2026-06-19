#!/usr/bin/env bash
# Собирает портативную папку dist/ (windeployqt + зависимости MinGW) и Setup.exe (Inno).
# Запуск из MSYS2 MinGW64. Перед этим собрать build/bin/Xipher.exe (ninja).
set -e
export PATH=/mingw64/bin:$PATH
cd "$(dirname "$0")/.."
rm -rf dist && mkdir -p dist
cp build/bin/Xipher.exe dist/
cp assets/xipher.ico dist/ 2>/dev/null || true
find third_party/libdatachannel -name libdatachannel.dll | head -1 | xargs -I{} cp {} dist/
windeployqt6 --release --no-translations --no-system-d3d-compiler --no-opengl-sw dist/Xipher.exe >/dev/null
for pass in 1 2 3; do
  for dll in dist/*.dll dist/Xipher.exe; do ldd "$dll" 2>/dev/null | grep -i mingw64 | awk '{print $3}'; done \
    | sort -u | while read d; do [ -f "$d" ] && cp -u "$d" dist/; done
done
ISCC=$(find "/c/Users/$USER/AppData/Local/Programs/Inno Setup 6" "/c/Program Files (x86)/Inno Setup 6" -name ISCC.exe 2>/dev/null | head -1)
[ -n "$ISCC" ] && "$ISCC" installer/xipher.iss && echo "Setup → installer/output/" || echo "Inno не найден — есть портативная dist/"
