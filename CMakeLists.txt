cmake_minimum_required(VERSION 3.12)
project(
        packed_vector
        VERSION 1.0.0
        DESCRIPTION "A variation of the STL std::vector, exclusively for unsigned integer or enum values."
        HOMEPAGE_URL "https://github.com/rodrigoo-r/packed_vector"
)

set(CMAKE_CXX_STANDARD 20)
set(CXXSTANDARD cxx_std_20)

# Check if we have a development environment
if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    add_executable(
            zext_packed_vector
            Example/Main.cpp
    )

    target_include_directories(
            zext_packed_vector PRIVATE
            Include
    )
else()
    add_library(zext_packed_vector INTERFACE)
    target_compile_features(zext_packed_vector INTERFACE ${CXXSTANDARD})

    add_library(zext::packed_vector ALIAS zext_packed_vector)

    target_include_directories(
            zext_packed_vector
            INTERFACE
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/Include>
            $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
    target_compile_features(zext_packed_vector INTERFACE ${CXXSTANDARD})
endif()