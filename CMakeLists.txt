cmake_minimum_required(VERSION 3.16)

# Get the project name from the folder name
get_filename_component(PROJECT_NAME_FROM_FOLDER ${CMAKE_SOURCE_DIR} NAME)
# Project name and version
project(${PROJECT_NAME_FROM_FOLDER} VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Generate compile_commands.json for better IDE support
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Set default build type to Debug if not specified
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

# Set output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Configure different build types
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -Wall -Wextra")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG -s -march=native -mtune=native -flto")

# For MSVC, add similar optimizations
if(MSVC)
    set(CMAKE_CXX_FLAGS_RELEASE "/O2 /Ob2 /Oi /Ot /GL /DNDEBUG")
    set(CMAKE_EXE_LINKER_FLAGS_RELEASE "/LTCG")
endif()

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${CMAKE_SOURCE_DIR}/libs)

# Find packages
find_package(PkgConfig QUIET)

# EnTT - Header-only ECS library
if(NOT EXISTS "${CMAKE_SOURCE_DIR}/libs/entt")
    message(STATUS "EnTT not found in libs/entt. Please add EnTT to libs/entt or install via package manager")
else()
    include_directories(${CMAKE_SOURCE_DIR}/libs/entt/src)
    message(STATUS "EnTT found at: ${CMAKE_SOURCE_DIR}/libs/entt")
endif()

# Sokol - Single-header libraries for graphics, audio, etc.
if(NOT EXISTS "${CMAKE_SOURCE_DIR}/libs/sokol")
    message(STATUS "Sokol not found in libs/sokol. Please add Sokol headers to libs/sokol")
else()
    include_directories(${CMAKE_SOURCE_DIR}/libs/sokol)
    message(STATUS "Sokol found at: ${CMAKE_SOURCE_DIR}/libs/sokol")
endif()

# Platform-specific libraries for Sokol
if(WIN32)
    set(SOKOL_LIBS 
        opengl32 
        gdi32 
        ole32
        d3d11
        dxgi
        dxguid
    )
elseif(APPLE)
    find_library(COCOA_LIBRARY Cocoa)
    find_library(QUARTZ_CORE_LIBRARY QuartzCore)
    find_library(METAL_LIBRARY Metal)
    find_library(METALKIT_LIBRARY MetalKit)
    set(SOKOL_LIBS 
        ${COCOA_LIBRARY}
        ${QUARTZ_CORE_LIBRARY}
        ${METAL_LIBRARY}
        ${METALKIT_LIBRARY}
    )
elseif(UNIX)
    find_package(X11 REQUIRED)
    find_package(OpenGL REQUIRED)
    set(SOKOL_LIBS 
        ${X11_LIBRARIES}
        ${OPENGL_LIBRARIES}
        m
        dl
        pthread
    )
endif()

# Find all source files
file(GLOB_RECURSE SOURCES 
    "${CMAKE_SOURCE_DIR}/src/*.cpp"
    "${CMAKE_SOURCE_DIR}/src/**/*.cpp"
)

# Find all header files (for IDE support)
file(GLOB_RECURSE HEADERS
    "${CMAKE_SOURCE_DIR}/include/*.hpp"
    "${CMAKE_SOURCE_DIR}/include/**/*.hpp"
    "${CMAKE_SOURCE_DIR}/include/*.h"
    "${CMAKE_SOURCE_DIR}/include/**/*.h"
)

# Create executable
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

# Set target properties
set_target_properties(${PROJECT_NAME} PROPERTIES
    OUTPUT_NAME "${CMAKE_PROJECT_NAME}"
    DEBUG_POSTFIX "_debug"
)

# Compiler-specific options
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(${PROJECT_NAME} PRIVATE
        -Wall -Wextra -Wpedantic
        $<$<CONFIG:Debug>:-g -O0>
        $<$<CONFIG:Release>:-O3 -s -march=native -mtune=native -flto>
    )
    # Add size optimization flags for release
    target_link_options(${PROJECT_NAME} PRIVATE
        $<$<CONFIG:Release>:-s -Wl,--gc-sections -flto>
    )
    
    # Static linking for self-contained executable
    target_link_options(${PROJECT_NAME} PRIVATE $<$<CONFIG:Release>:-static-libgcc -static-libstdc++ -static>)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    target_compile_options(${PROJECT_NAME} PRIVATE
        /W4
        $<$<CONFIG:Debug>:/Od /Zi>
        $<$<CONFIG:Release>:/O2 /Ob2 /Oi /Ot /GL>
    )
    target_link_options(${PROJECT_NAME} PRIVATE
        $<$<CONFIG:Release>:/LTCG>
    )
endif()

# Sokol compile definitions
target_compile_definitions(${PROJECT_NAME} PRIVATE
    $<$<PLATFORM_ID:Windows>:SOKOL_D3D11>
    $<$<PLATFORM_ID:Darwin>:SOKOL_METAL>
    $<$<PLATFORM_ID:Linux>:SOKOL_GLCORE33>
)

# Link libraries
if(EXISTS "${CMAKE_SOURCE_DIR}/libs/sokol")
    target_link_libraries(${PROJECT_NAME} PRIVATE ${SOKOL_LIBS})
    message(STATUS "Linking Sokol platform libraries: ${SOKOL_LIBS}")
else()
    message(STATUS "Sokol not found - skipping platform library linking")
endif()

# Additional libraries can be added here as needed
# target_link_libraries(${PROJECT_NAME} PRIVATE additional_library)

# Custom target to copy assets
add_custom_target(copy_assets ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/assets
        ${CMAKE_BINARY_DIR}/bin/assets
    COMMENT "Copying assets to build directory"
)

# Make sure assets are copied before building the executable
add_dependencies(${PROJECT_NAME} copy_assets)

# Print configuration info
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Source files found: ${SOURCES}")
message(STATUS "Output directory: ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
