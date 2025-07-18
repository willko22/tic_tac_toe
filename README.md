# Tower Defense Game

A modern C++23 tower defense game built with EnTT entity component system and Sokol graphics library.

## Prerequisites

- **CMake** (3.20 or higher)
- **MinGW-w64** with GCC supporting C++23
- **Git** (for downloading dependencies)

## Setup

1. **Clone the repository:**
   ```bash
   git clone https://github.com/willko22/tower_defense.git
   cd tower_defense
   ```

2. **Download external libraries:**
   ```bash
   setup_libs.bat
   ```
   This will download EnTT and Sokol libraries to the `libs/` folder.

3. **Build the project:**
   ```bash
   build_debug.bat     # For debug build
   build_release.bat   # For release build
   ```

4. **Run the game:**
   ```bash
   run_game.bat        # Runs the debug version
   ```

## Build System

The project uses CMake as the build system with convenient batch files:

- `setup_libs.bat` - Downloads external dependencies (EnTT, Sokol)
- `build_debug.bat` - Builds debug version using CMake + MinGW
- `build_release.bat` - Builds release version using CMake + MinGW  
- `build_and_run.bat` - Builds and runs the debug version
- `run_game.bat` - Runs the built executable

## Libraries Used

- **EnTT** - Modern header-only entity component system
- **Sokol** - Minimal cross-platform graphics library

See `docs/LIBRARIES.md` for detailed information about the libraries.