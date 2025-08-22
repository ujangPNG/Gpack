cmake_minimum_required(VERSION 3.16)
project(GitPack)

# Set C++ standard to C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add libgit2 from the externals directory
# This will also set up include directories for libgit2
add_subdirectory(externals/libgit2)

# Define the executable and its sources
add_executable(gpack
    GitPack/main.cpp
    GitPack/packer.cpp
    GitPack/unpacker.cpp
)

# Link the executable against libgit2
target_link_libraries(gpack PRIVATE git2)

# Since we are using std::filesystem, we need to handle a special case for older compilers
if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS "8.0")
    target_link_libraries(gpack PRIVATE -lstdc++fs)
endif()

if (MSVC)
    # Add definitions to suppress warnings in libgit2 headers
    target_compile_definitions(gpack PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()

# Define output directory for the executable
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
