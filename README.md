# Xipher Desktop — v0.1

Фул-нативный десктоп-клиент Xipher на **Qt 6 / C++** (без webview).
Архитектурный ориентир — Telegram Desktop (tdesktop тоже на Qt).

В версии **0.1** реализованы:

- **Вход** — экран 1:1 с веб-дизайном (`web/login.html` + `web/css/login.css`):
  тёмный фон `#05060f` с радиальными свечениями, «glass»-карточка, бренд, поля,
  градиентная кнопка, ошибки под полями, ссылки.
- **Регистрация** — 1:1 с `web/register.html` + `web/js/register.js`: валидация ника,
  живая проверка доступности через `/api/check-username` (дебаунс 500 мс),
  подтверждение пароля.
- **Мини-экран после входа** (пока без дизайна) — подтверждает успешный вход
  (имя, `user_id`, кнопка выхода). Заглушка под будущий мессенджер.
- Реальная работа против прод-API **https://messenger.xipher.pro** по HTTPS.
- Восстановление сессии по сохранённому токену (`QSettings` → реестр
  `HKCU\Software\Xipher\Desktop`), проверка через `/api/validate-token`.

Наша индивидуальная штука: тег **DESKTOP** рядом с брендом.

## Структура

```
src/
  main.cpp                 — точка входа, глобальный QSS
  app/MainWindow.*         — QStackedWidget: login / register / home
  net/ApiClient.*          — QNetworkAccessManager → /api/login,/register,...
  net/Session.*            — хранение токена (QSettings)
  ui/Theme.h               — палитра + глобальный стиль (значения из login.css)
  ui/BackgroundWidget.*    — отрисовка фона-градиентов
  ui/AuthUi.h              — хелперы карточки (общие для login/register)
  ui/LoginPage.*           — экран входа
  ui/RegisterPage.*        — экран регистрации
  ui/HomePage.*            — мини-экран после входа
resources/resources.qrc    — задел под иконки/шрифты
```

## Сборка (Windows, MinGW из MSYS2)

Тулчейн ставится один раз:

```sh
winget install MSYS2.MSYS2
C:\msys64\usr\bin\bash -lc "pacman -S --needed --noconfirm \
  mingw-w64-x86_64-gcc mingw-w64-x86_64-cmake mingw-w64-x86_64-ninja \
  mingw-w64-x86_64-qt6-base mingw-w64-x86_64-qt6-tools"
```

Сборка:

```bat
build.bat
```

или вручную:

```sh
export PATH=/c/msys64/mingw64/bin:$PATH
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=/c/msys64/mingw64
cmake --build build
```

Результат: `build/bin/Xipher.exe`.

Чтобы exe запускался без MSYS2 в PATH, рядом раскладываются Qt-DLL и рантайм MinGW
(делается `windeployqt6 Xipher.exe` + копирование зависимостей; уже выполнено для
текущей сборки в `build/bin`).

## Дальнейшие шаги (после 0.1)

- Список чатов и переписка (как в Telegram), WebSocket `/ws`.
- E2EE (Signal-протокол) — на сервере уже есть `request_handler_signal`.
- Звонки (TURN/SFU), боты, каналы.
- Сборка инсталлятора, обновления.
