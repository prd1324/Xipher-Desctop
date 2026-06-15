# ─────────────────────────────────────────────────────────────────────────────
#  Сборка MSIX-пакета Xipher Desktop из готового build/bin.
#  Требуется Windows SDK (makeappx.exe, signtool.exe в PATH).
#
#  Использование (из обычного PowerShell):
#     ./packaging/build-msix.ps1 -BinDir ..\build\bin
#     ./packaging/build-msix.ps1 -BinDir ..\build\bin -Pfx cert.pfx -Password ***
#
#  Для Microsoft Store подпись делать НЕ нужно — пакет подписывает сам Store
#  при публикации через Partner Center (поправь Identity/Publisher в манифесте
#  на выданные Store значения).
# ─────────────────────────────────────────────────────────────────────────────
param(
    [string]$BinDir   = "$PSScriptRoot/../build/bin",
    [string]$Manifest = "$PSScriptRoot/AppxManifest.xml",
    [string]$AssetsDir = "$PSScriptRoot/Assets",
    [string]$Out      = "$PSScriptRoot/../build/Xipher.msix",
    [string]$Pfx      = "",
    [string]$Password = ""
)

$ErrorActionPreference = "Stop"

$stage = Join-Path $env:TEMP "xipher_msix_stage"
if (Test-Path $stage) { Remove-Item $stage -Recurse -Force }
New-Item -ItemType Directory -Path $stage | Out-Null

# 1) Копируем бинарники + развёрнутый Qt-runtime
Copy-Item "$BinDir/*" $stage -Recurse -Force

# 2) Манифест и ассеты (иконки плиток — положи PNG в packaging/Assets)
Copy-Item $Manifest (Join-Path $stage "AppxManifest.xml") -Force
if (Test-Path $AssetsDir) {
    Copy-Item $AssetsDir (Join-Path $stage "Assets") -Recurse -Force
} else {
    Write-Warning "Нет папки Assets — добавь иконки плиток (StoreLogo/Square150x150/Square44x44 PNG)."
}

# 3) Упаковка
& makeappx pack /d $stage /p $Out /overwrite
Write-Host "MSIX собран: $Out"

# 4) Подпись (опционально, для прямой раздачи вне Store)
if ($Pfx -ne "") {
    $ts = "http://timestamp.sectigo.com"
    if ($Password -ne "") {
        & signtool sign /fd SHA256 /a /f $Pfx /p $Password /tr $ts /td SHA256 $Out
    } else {
        & signtool sign /fd SHA256 /a /f $Pfx /tr $ts /td SHA256 $Out
    }
    Write-Host "MSIX подписан."
}
