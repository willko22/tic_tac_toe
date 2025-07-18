# Library Management Guide

## Where to Place Libraries

### 1. `libs/` Directory (Manual Installation)
Place manually downloaded libraries here:
```
libs/
├── SDL2/
│   ├── include/
│   ├── lib/
│   └── bin/
├── SFML/
│   ├── include/
│   ├── lib/
│   └── bin/
└── glm/
    └── include/
```

For manual libraries, add to CMakeLists.txt:
```cmake
# Manual library setup
set(SDL2_ROOT "${CMAKE_SOURCE_DIR}/libs/SDL2")
find_package(SDL2 REQUIRED)
```

### 2. System-Wide Installation (Recommended)

#### Using vcpkg (Windows/Cross-platform)
```bash
# Install vcpkg
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg
.\bootstrap-vcpkg.bat

# Install libraries
.\vcpkg install sdl2 sdl2-ttf sfml glm nlohmann-json

# Use in CMake
cmake -DCMAKE_TOOLCHAIN_FILE=path/to/vcpkg/scripts/buildsystems/vcpkg.cmake ..
```

#### Using package managers:
- **Windows**: vcpkg, Conan
- **Linux**: apt, dnf, pacman
- **macOS**: Homebrew, MacPorts

### 3. Git Submodules (For Source Libraries)
```bash
# Add library as submodule
git submodule add https://github.com/nlohmann/json.git libs/json

# In CMakeLists.txt
add_subdirectory(libs/json)
target_link_libraries(${PROJECT_NAME} nlohmann_json::nlohmann_json)
```

## Recommended Libraries for Tower Defense

### Graphics & Input
- **SDL2** - Cross-platform graphics, input, and audio
- **SFML** - Alternative to SDL2, more C++ friendly
- **GLFW** - For OpenGL window management

### Math
- **GLM** - OpenGL Mathematics library
- **Eigen** - Advanced linear algebra

### Audio
- **SDL2_mixer** - Audio mixing
- **FMOD** - Professional audio engine
- **OpenAL** - 3D audio

### Utilities
- **nlohmann/json** - JSON parsing
- **spdlog** - Fast logging library
- **fmt** - String formatting

### Game Development
- **Box2D** - 2D physics engine
- **Bullet** - 3D physics engine
- **Dear ImGui** - Immediate mode GUI
