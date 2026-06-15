# Подпись и релиз Xipher Desktop

Зачем: чтобы у пользователей приложение запускалось **без предупреждений** SmartScreen
и без блокировки Smart App Control — **не прося отключать Defender**. Единственный
правильный путь — подписанный билд. Ниже бесплатные варианты.

## SmartScreen vs Smart App Control

- **SmartScreen** — у большинства. Для неизвестного издателя показывает баннер
  «Подробнее → Выполнить в любом случае» (запуск возможен). Подпись + наработанная
  репутация убирают баннер.
- **Smart App Control (SAC)** — у меньшинства (чистые Win11). Жёстко блокирует
  неподписанное без кнопки запуска. Снимается только подписью с облачной репутацией
  (Microsoft) или установкой из Microsoft Store.

## Бесплатные варианты подписи

### 1. SignPath Foundation (для open-source) — рекомендуется
Бесплатные code-signing сертификаты для открытых проектов, интеграция с GitHub Actions.

Шаги:
1. Подать проект в программу SignPath Foundation (нужен публичный OSS-репозиторий).
2. В репозитории GitHub:
   - **Settings → Secrets and variables → Variables**: `SIGNPATH_ORGANIZATION_ID`.
   - **Settings → Secrets**: `SIGNPATH_API_TOKEN`.
3. В `.github/workflows/release.yml` уточнить `project-slug`, `signing-policy-slug`,
   `artifact-configuration-slug` под свой проект SignPath.
4. Пуш тега `vX.Y.Z` → CI соберёт и подпишет, артефакт `Xipher-windows-signed`.

Пока переменная `SIGNPATH_ORGANIZATION_ID` не задана — workflow просто публикует
неподписанный артефакт (шаг подписи пропускается).

### 2. Microsoft Store (MSIX) — ~$19 единоразово
Приложения из Store SAC/SmartScreen доверяют автоматически (Store подписывает сам).
1. Аккаунт разработчика в Partner Center (физлицо ~$19, разово).
2. Вписать в `packaging/AppxManifest.xml` выданные Store `Identity/Name` и `Publisher`.
3. Собрать пакет: `packaging/build-msix.ps1 -BinDir build/bin` (нужен Windows SDK).
4. Загрузить `.msix` в Partner Center.

## Платно (на будущее, для прямой раздачи .exe)

- **Azure Trusted Signing** — ~$10/мес, корень доверия Microsoft, отличная репутация
  в SmartScreen/SAC, ключ в облаке (без токена).
- **EV code-signing** (Sectigo/SSL.com ~$300+/год) — мгновенная репутация, токен.

## Локальная подпись (если уже есть .pfx или токен)

```sh
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release \
  -DXIPHER_SIGN=ON -DXIPHER_SIGN_PFX=cert.pfx -DXIPHER_SIGN_PASS=*** \
  -DCMAKE_PREFIX_PATH=/c/msys64/mingw64
cmake --build build
```

или по отпечатку из хранилища сертификатов:

```sh
cmake -B build -DXIPHER_SIGN=ON -DXIPHER_SIGN_SHA1=<thumbprint> ...
```

`signtool` берётся из Windows SDK (должен быть в PATH или задай `-DXIPHER_SIGNTOOL=`).

## Разработка

Локально подпись не нужна (`XIPHER_SIGN=OFF` по умолчанию). Если на твоей машине
Smart App Control блокирует свой же билд — это только твоя машина и к релизу
отношения не имеет; на время разработки SAC можно выключить в
«Безопасность Windows → Управление приложениями и браузером → Smart App Control».
