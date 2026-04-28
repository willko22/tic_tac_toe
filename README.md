# Tic Tac Toe Game

A C++23 terminal-based Tic Tac Toe game with configurable board size, win count, and an efficient AI opponent.

## Features

- **Configurable Board Size**: Play on boards of any size (3x3, 5x5, etc.)
- **Flexible Win Count**: Set how many marks in a row are needed to win
- **AI Opponent**: Fast minimax-based AI that works efficiently up to board size 6
- **Terminal Interface**: Simple command-line gameplay

## Prerequisites

- **CMake** (3.20 or higher)
- **MinGW-w64** with GCC supporting C++23

## Build and Run

1. **Build the project:**
   ```bash
   build_debug.bat     # For debug build
   build_release.bat   # For release build
   ```

2. **Run the game:**
   ```bash
   run_game.bat        # Runs the built version
   ```

   Or build and run in one step:
   ```bash
   build_and_run.bat
   ```

## Build System

The project uses CMake as the build system with convenient batch files:

- `build_debug.bat` - Builds debug version using CMake + MinGW
- `build_release.bat` - Builds release version using CMake + MinGW  
- `build_and_run.bat` - Builds and runs the debug version
- `run_game.bat` - Runs the built executable
