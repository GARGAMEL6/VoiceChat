cmake_minimum_required(VERSION 3.16)
project(VoiceClient LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Qt6 REQUIRED COMPONENTS Core Network Widgets Multimedia)
qt_standard_project_setup()

# 🔹 WIN32 флаг убирает консольное окно
add_executable(VoiceClient WIN32
    main.cpp
    tcp_client.h tcp_client.cpp
    mainwindow.h mainwindow.cpp
    audio_manager.h audio_manager.cpp
)

target_link_libraries(VoiceClient PRIVATE Qt6::Core Qt6::Network Qt6::Widgets Qt6::Multimedia)

add_custom_command(TARGET VoiceClient POST_BUILD
    COMMAND "C:/Qt/6.11.2/mingw_64/bin/windeployqt.exe"
            --no-translations --no-qml --release
            "$<TARGET_FILE:VoiceClient>"
    VERBATIM
)

# Копируем config.ini в папку сборки
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/config.ini DESTINATION ${CMAKE_CURRENT_BINARY_DIR})