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

# Qt 6 — Widgets для UI, Network для REST, WebSockets для realtime /ws.
find_package(Qt6 REQUIRED COMPONENTS Widgets Network WebSockets)

set(SOURCES
    src/main.cpp
    src/app/MainWindow.cpp
    src/net/ApiClient.cpp
    src/net/Session.cpp
    src/net/WsClient.cpp
    src/ui/BackgroundWidget.cpp
    src/ui/LoginPage.cpp
    src/ui/RegisterPage.cpp
    src/ui/HomePage.cpp
    src/ui/ChatPage.cpp
)

set(HEADERS
    src/app/MainWindow.h
    src/net/ApiClient.h
    src/net/Session.h
    src/net/Models.h
    src/net/WsClient.h
    src/ui/Theme.h
    src/ui/BackgroundWidget.h
    src/ui/LoginPage.h
    src/ui/RegisterPage.h
    src/ui/HomePage.h
    src/ui/ChatPage.h
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
)

set_target_properties(Xipher PROPERTIES
    OUTPUT_NAME "Xipher"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
