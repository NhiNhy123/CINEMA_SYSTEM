cmake_minimum_required(VERSION 3.20)
project(CINEMA_SYSTEM VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Search for SFML (SFML 3 or SFML 2.6)
find_package(SFML 3 COMPONENTS Graphics Window System CONFIG QUIET)
if (NOT SFML_FOUND)
    find_package(SFML 2.6 COMPONENTS graphics window system REQUIRED)
endif()

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/include/UI
)

# Core Backend Source Files
set(BACKEND_SOURCES
    src/ActivityLog.cpp
    src/Customer.cpp
    src/Database.cpp
    src/FAndB.cpp
    src/Invoice.cpp
    src/Movie.cpp
    src/Promotion.cpp
    src/Room.cpp
    src/Seat.cpp
    src/ShilftManagement.cpp
    src/Showtime.cpp
    src/SysLog.cpp
    src/Ticket.cpp
    src/User.cpp
)

# UI Source Files
set(UI_SOURCES
    src/UI/UITheme.cpp
    src/UI/CinemaUIApp.cpp
)

# Target 1: Graphical Cinema Management System (SFML UI)
add_executable(CinemaUI
    src/Cinema_UI_main.cpp
    ${BACKEND_SOURCES}
    ${UI_SOURCES}
)

if (WIN32)
    target_compile_options(CinemaUI PRIVATE -include windows.h)
endif()

if (TARGET SFML::Graphics)
    target_link_libraries(CinemaUI PRIVATE SFML::Graphics SFML::Window SFML::System "-Wl,-Bstatic,-lsqlite3,-Bdynamic")
else()
    target_link_libraries(CinemaUI PRIVATE sfml-graphics sfml-window sfml-system "-Wl,-Bstatic,-lsqlite3,-Bdynamic")
endif()

# Target 2: Console / Terminal Cinema System
add_executable(CinemaConsole
    src/Cinema_main.cpp
    ${BACKEND_SOURCES}
)

if (WIN32)
    target_compile_options(CinemaConsole PRIVATE -include windows.h)
endif()

target_link_libraries(CinemaConsole PRIVATE "-Wl,-Bstatic,-lsqlite3,-Bdynamic")

