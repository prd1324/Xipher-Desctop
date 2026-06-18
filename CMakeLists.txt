cmake_minimum_required(VERSION 3.21)

# ─────────────────────────────────────────────────────────────────────────────
#  Xipher Desktop — фул-нативный клиент на Qt 6 (C++), без webview.
#  Версия 0.1: вход / регистрация 1:1 с веб-дизайном + мини-экран после входа.
# ─────────────────────────────────────────────────────────────────────────────
project(XipherDesktop VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC OFF)

# Qt 6 — Widgets (UI), Network (REST), WebSockets (realtime), Multimedia (голос).
find_package(Qt6 REQUIRED COMPONENTS Widgets Network WebSockets Multimedia)

set(SOURCES
    src/main.cpp
    src/app/MainWindow.cpp
    src/net/ApiClient.cpp
    src/net/Session.cpp
    src/net/WsClient.cpp
    src/net/VoiceRecorder.cpp
    src/net/CallEngine.cpp
    src/net/CallController.cpp
    src/ui/CallOverlay.cpp
    src/ui/BackgroundWidget.cpp
    src/ui/LoginPage.cpp
    src/ui/RegisterPage.cpp
    src/ui/HomePage.cpp
    src/ui/ChatPage.cpp
    src/ui/NewChatDialog.cpp
    src/ui/VoiceMessageWidget.cpp
    src/ui/RecordingBar.cpp
    src/ui/EmojiPicker.cpp
    src/ui/AvatarUtil.cpp
    src/ui/Checklist.cpp
    src/ui/ModalOverlay.cpp
    src/ui/EmptyChatGreeting.cpp
    src/ui/AnimatedEmojiLabel.cpp
    src/ui/ProfilePanel.cpp
    src/ui/ToggleSwitch.cpp
    src/ui/SettingsDialog.cpp
    src/ui/GroupChannelDialogs.cpp
)

set(HEADERS
    src/app/MainWindow.h
    src/net/ApiClient.h
    src/net/Session.h
    src/net/Models.h
    src/net/WsClient.h
    src/net/VoiceRecorder.h
    src/net/CallEngine.h
    src/net/CallController.h
    src/ui/CallOverlay.h
    src/ui/Theme.h
    src/ui/BackgroundWidget.h
    src/ui/LoginPage.h
    src/ui/RegisterPage.h
    src/ui/HomePage.h
    src/ui/ChatPage.h
    src/ui/NewChatDialog.h
    src/ui/VoiceMessageWidget.h
    src/ui/RecordingBar.h
    src/ui/EmojiPicker.h
    src/ui/AvatarUtil.h
    src/ui/Icons.h
    src/ui/Checklist.h
    src/ui/ModalOverlay.h
    src/ui/EmptyChatGreeting.h
    src/ui/AnimatedEmojiLabel.h
    src/ui/ProfilePanel.h
)

qt_add_executable(Xipher
    WIN32                       # GUI-приложение без консоли
    ${SOURCES}
    ${HEADERS}
    resources/resources.qrc
)

target_include_directories(Xipher PRIVATE src)

target_link_libraries(Xipher PRIVATE
    Qt6::Widgets
    Qt6::Network
    Qt6::WebSockets
    Qt6::Multimedia
)

if(WIN32)
    target_link_libraries(Xipher PRIVATE dwmapi)   # тёмный заголовок окна (DwmSetWindowAttribute)
endif()

# ── WebRTC (libdatachannel, собран в third_party) для звонков ──────────────────
set(LIBDC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party/libdatachannel")
if(EXISTS "${LIBDC_DIR}/build/libdatachannel.dll.a")
    target_include_directories(Xipher PRIVATE "${LIBDC_DIR}/include")
    target_link_libraries(Xipher PRIVATE "${LIBDC_DIR}/build/libdatachannel.dll.a")
    target_compile_definitions(Xipher PRIVATE XIPHER_HAVE_WEBRTC=1 RTC_ENABLE_MEDIA=1)
    # Opus для кодирования голоса
    find_library(OPUS_LIB opus REQUIRED)
    find_path(OPUS_INC opus/opus.h)
    target_include_directories(Xipher PRIVATE "${OPUS_INC}" "${OPUS_INC}/opus")
    target_link_libraries(Xipher PRIVATE "${OPUS_LIB}")
    message(STATUS "Xipher: WebRTC (libdatachannel) + Opus подключены")
else()
    message(WARNING "Xipher: libdatachannel не собран — звонки будут отключены")
endif()

set_target_properties(Xipher PROPERTIES
    OUTPUT_NAME "Xipher"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# ─────────────────────────────────────────────────────────────────────────────
#  Подпись релиза (опционально). По умолчанию ВЫКЛ — для локальной разработки
#  не нужна. На релизе включается -DXIPHER_SIGN=ON и подставляется сертификат.
#  Бесплатные пути к сертификату: SignPath Foundation (open-source) или
#  Microsoft Store (MSIX). См. docs/SIGNING.md.
# ─────────────────────────────────────────────────────────────────────────────
option(XIPHER_SIGN "Подписывать Xipher.exe через signtool после сборки" OFF)
set(XIPHER_SIGN_PFX  "" CACHE FILEPATH "Путь к .pfx сертификату")
set(XIPHER_SIGN_SHA1 "" CACHE STRING   "Отпечаток (thumbprint) сертификата из хранилища")
set(XIPHER_SIGN_PASS "" CACHE STRING   "Пароль к .pfx (если задан)")
set(XIPHER_TIMESTAMP_URL "http://timestamp.sectigo.com" CACHE STRING "RFC3161 timestamp URL")
set(XIPHER_SIGNTOOL "signtool" CACHE FILEPATH "Путь к signtool.exe (из Windows SDK)")

if(WIN32 AND XIPHER_SIGN)
    set(_sign_args sign /fd SHA256 /tr "${XIPHER_TIMESTAMP_URL}" /td SHA256)
    if(XIPHER_SIGN_PFX)
        list(APPEND _sign_args /f "${XIPHER_SIGN_PFX}")
        if(XIPHER_SIGN_PASS)
            list(APPEND _sign_args /p "${XIPHER_SIGN_PASS}")
        endif()
    elseif(XIPHER_SIGN_SHA1)
        list(APPEND _sign_args /sha1 "${XIPHER_SIGN_SHA1}")
    else()
        message(WARNING "XIPHER_SIGN=ON, но не задан ни XIPHER_SIGN_PFX, ни XIPHER_SIGN_SHA1")
    endif()
    add_custom_command(TARGET Xipher POST_BUILD
        COMMAND "${XIPHER_SIGNTOOL}" ${_sign_args} "$<TARGET_FILE:Xipher>"
        COMMENT "Подпись Xipher.exe (signtool)"
        VERBATIM)
endif()
