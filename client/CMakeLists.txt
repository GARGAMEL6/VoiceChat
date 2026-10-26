cmake_minimum_required(VERSION 3.16)
project(VoiceClient LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# 🔹 ДОБАВЛЕН Multimedia
find_package(Qt6 REQUIRED COMPONENTS Core Network Widgets Multimedia)
qt_standard_project_setup()

add_executable(VoiceClient
    main.cpp
    tcp_client.h tcp_client.cpp
    mainwindow.h mainwindow.cpp
    audio_manager.h audio_manager.cpp  # <-- ДОБАВЛЕНО
)

# 🔹 ДОБАВЛЕН Qt6::Multimedia
target_link_libraries(VoiceClient PRIVATE Qt6::Core Qt6::Network Qt6::Widgets Qt6::Multimedia)

add_custom_command(TARGET VoiceClient POST_BUILD
    COMMAND "C:/Qt/6.11.2/mingw_64/bin/windeployqt.exe"
            --no-translations --no-qml --release
            "$<TARGET_FILE:VoiceClient>"
    VERBATIM
)