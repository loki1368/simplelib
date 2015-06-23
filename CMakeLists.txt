cmake_minimum_required(VERSION 2.8.4)
project(simplelib)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(SOURCE_FILES
    quazip/crypt.h
    quazip/ioapi.h
    quazip/JlCompress.cpp
    quazip/JlCompress.h
    quazip/qioapi.cpp
    quazip/quaadler32.cpp
    quazip/quaadler32.h
    quazip/quachecksum32.h
    quazip/quacrc32.cpp
    quazip/quacrc32.h
    quazip/quagzipfile.cpp
    quazip/quagzipfile.h
    quazip/quaziodevice.cpp
    quazip/quaziodevice.h
    quazip/quazip.cpp
    quazip/quazip.h
    quazip/quazip_global.h
    quazip/quazipdir.cpp
    quazip/quazipdir.h
    quazip/quazipfile.cpp
    quazip/quazipfile.h
    quazip/quazipfileinfo.cpp
    quazip/quazipfileinfo.h
    quazip/quazipnewinfo.cpp
    quazip/quazipnewinfo.h
    quazip/unzip.c
    quazip/unzip.h
    quazip/zip.c
    quazip/zip.h
    main.cpp
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui)

add_executable(simplelib ${SOURCE_FILES})