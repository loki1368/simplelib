cmake_minimum_required(VERSION 3.2)
project(simplelib)

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")

# Tell CMake to run moc when necessary:
set(CMAKE_AUTOMOC ON)
# As moc files are generated in the binary dir, tell CMake
# to always look for includes there:
set(CMAKE_INCLUDE_CURRENT_DIR ON)

# Include a library search using find_package()
# via REQUIRED, specify that libraries are required
find_package(Qt5Core REQUIRED)
find_package(Qt5Gui REQUIRED)
find_package(Qt5Widgets REQUIRED)
find_package(Qt5Sql REQUIRED)

set(SOURCE_FILES    
    main.cpp
    mainwindow.cpp
    mainwindow.h    
    mainwindow.ui
    parsebigzip.cpp
    parsebigzip.h
    settingsdialog.cpp
    settingsdialog.h
    settingsdialog.ui
    smplibdatabase.cpp
    smplibdatabase.h)

qt5_wrap_ui(UIS_HDRS
    mainwindow.ui
    settingsdialog.ui
)

add_executable(simplelib ${SOURCE_FILES} ${UIS_HDRS})

# specify which libraries to connect
target_link_libraries(${PROJECT_NAME} Qt5::Core)
target_link_libraries(${PROJECT_NAME} Qt5::Gui)
target_link_libraries(${PROJECT_NAME} Qt5::Widgets)
target_link_libraries(${PROJECT_NAME} Qt5::Sql)
target_link_libraries(${PROJECT_NAME} quazip1-qt5)
