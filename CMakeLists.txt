# Example CMake command line to create project build files:
#
# *** Windows ***
# cmake -G "Visual Studio 17 2022" -A Win32 -B ../C_StateMachineWithThreadsBuild -S .
#
# *** Linux ***
# cmake -G "Unix Makefiles" -B ../C_StateMachineWithThreadsBuild -S .

# Specify the minimum CMake version required
cmake_minimum_required(VERSION 3.10)

# Project name and language (C or C++)
project(C_StateMachineWithThreads VERSION 1.0 LANGUAGES C CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Collect all .cpp and *.h source files in the current directory
file(GLOB SOURCES "${CMAKE_SOURCE_DIR}/*.cpp" "${CMAKE_SOURCE_DIR}/*.h")

# Add subdirectories to include path
include_directories( 
    ${CMAKE_SOURCE_DIR}/Allocator
    ${CMAKE_SOURCE_DIR}/Callback
    ${CMAKE_SOURCE_DIR}/Port
    ${CMAKE_SOURCE_DIR}/SelfTest
    ${CMAKE_SOURCE_DIR}/StateMachine
)

# Add an executable target
add_executable(C_StateMachineWithThreadsApp ${SOURCES})

# Add subdirectories to build
add_subdirectory(Allocator)
add_subdirectory(Callback)
add_subdirectory(Port)
add_subdirectory(SelfTest)
add_subdirectory(StateMachine)

target_link_libraries(C_StateMachineWithThreadsApp PRIVATE 
    AllocatorLib
    CallbackLib
    PortLib
    SelfTestLib
    StateMachineLib
)

