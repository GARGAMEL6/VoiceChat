cmake_minimum_required(VERSION 3.16)
project(VoiceClient LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Подключаем Core, Network (для сокетов) и Widgets (для GUI)
find_package(Qt6 REQUIRED COMPONENTS Core Network Widgets)
qt_standard_project_setup()

add_executable(VoiceClient
    main.cpp
)

target_link_libraries(VoiceClient PRIVATE Qt6::Core Qt6::Network Qt6::Widgets)

# Автоматический вызов windeployqt после сборки
add_custom_command(TARGET VoiceClient POST_BUILD
    COMMAND "C:/Qt/6.11.2/mingw_64/bin/windeployqt.exe"
            --no-translations --no-qml --release
            "$<TARGET_FILE:VoiceClient>"
    VERBATIM
)