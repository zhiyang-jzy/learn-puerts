cmake_minimum_required(VERSION 3.24)
project(testpuerts)

set(CMAKE_CXX_STANDARD 14)

#if (WIN32 AND NOT CYGWIN AND NOT (CMAKE_SYSTEM_NAME STREQUAL "WindowsStore") AND NOT ANDROID)
#    set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} /MT" CACHE STRING "")
#    set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} /MTd" CACHE STRING "")
#    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT" CACHE STRING "")
#    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MTd" CACHE STRING "")
#    message(STATUS "tt")
#endif ()
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

include_directories(include v8/Inc
        )

link_directories(lib)



add_executable(testpuerts main.cpp)
target_link_libraries(testpuerts wee8.lib  winmm.lib dbghelp.lib shlwapi.lib puerts_il2cpp.lib)
target_compile_definitions(testpuerts PRIVATE PLATFORM_WINDOWS)