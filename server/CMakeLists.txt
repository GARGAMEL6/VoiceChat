cmake_minimum_required(VERSION 3.16)
project(VoiceServer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Qt6 REQUIRED COMPONENTS Core Network Sql)
qt_standard_project_setup()

add_executable(VoiceServer
    main.cpp
    control_server.h control_server.cpp
    client_session.h client_session.cpp
    auth_manager.h auth_manager.cpp
)

target_link_libraries(VoiceServer PRIVATE Qt6::Core Qt6::Network Qt6::Sql)

add_custom_command(TARGET VoiceServer POST_BUILD
    COMMAND "C:/Qt/6.11.2/mingw_64/bin/windeployqt.exe"
            --no-translations --no-qml --release
            "$<TARGET_FILE:VoiceServer>"
    VERBATIM
)

# Копируем config.ini в папку сборки
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/config.ini DESTINATION ${CMAKE_CURRENT_BINARY_DIR})