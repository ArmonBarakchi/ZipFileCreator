cmake_minimum_required(VERSION 3.10)
project(ECE141-Archive)

set(CMAKE_CXX_STANDARD 17)

if(WIN32)
    #    # Set this to your Zlib root directory on Windows
    #    set(ZLIB_ROOT "zlib")
    #
    #    # Include the Zlib headers
    #    include_directories("${ZLIB_ROOT}/include")
    #
    #    find_library(ZLIB_LIBRARY NAMES zlib1 zlib libzlib PATHS "${ZLIB_ROOT}/lib" NO_DEFAULT_PATH)
    #    if(NOT ZLIB_LIBRARY)
    #        message(FATAL_ERROR "ZLIB library not found")
    #    endif()
else()
    # Find Zlib package. This will work out of the box on Linux and macOS, where Zlib is usually pre-installed
    find_package(ZLIB)
endif()

# Define your executable
add_executable(${PROJECT_NAME}
        Archive.cpp
        Archive.hpp
        main.cpp
        Testable.hpp
        Testing.hpp
        Timer.hpp
        Tracker.hpp
        Chunker.h)

# Link Zlib to your project
if (NOT WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE ${ZLIB_LIBRARY})
endif()