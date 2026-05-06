# STM32 Project Setup Guide for VS Code

## Issues Addressed
1. ✅ STM32CubeIDE extension configuration
2. ✅ ARM GCC compiler integration
3. ✅ Build and flash tasks setup
4. ✅ VS Code workspace configuration

## Prerequisites

### 1. Install Required Tools

#### ARM GCC Compiler
Download and install from: https://developer.arm.com/tools-and-software/open-source-toolchains-for-embedded/gnu-toolchain/downloads

- **Windows**: Download `arm-none-eabi-gcc-*-win32.exe` or portable version
- After installation, verify it's in your PATH:
  ```powershell
  arm-none-eabi-gcc --version
  ```

#### CMake
Download from: https://cmake.org/download/

- Ensure it's added to PATH
- Verify: `cmake --version`

#### Ninja (Optional but recommended)
Download from: https://github.com/ninja-build/ninja/releases

- Extract and add to PATH
- Verify: `ninja --version`

#### STM32 Programmer CLI (for flashing)
Download from: https://www.st.com/en/development-tools/stm32cubeprogrammer.html

- Install and note the installation path
- Add to PATH or use full path in tasks

### 2. VS Code Extensions (Required)
- **CMake** (`ms-vscode.cmake-tools`)
- **C/C++** (`ms-vscode.cpptools`)
- **Cortex-Debug** (`mcu-debug.cortex-debug`) - Optional, for debugging
- **STM32CubeIDE** (`stmicroelectronics.stm32cubemx`)

Install via: `Extensions` panel or:
```
code --install-extension ms-vscode.cmake-tools
code --install-extension ms-vscode.cpptools
code --install-extension stmicroelectronics.stm32cubemx
```

## Configuration Files Created

### `.vscode/settings.json`
- Configures CMake to use Ninja generator
- Sets up C/C++ IntelliSense paths
- Defines compiler settings for ARM GCC

### `.vscode/tasks.json`
Available tasks:
- **build** (default): Configure and build project
- **configure**: Run CMake configuration
- **rebuild**: Clean build
- **flash**: Flash firmware to device (requires STM32_Programmer_CLI)
- **erase**: Erase device memory

Run tasks with: `Ctrl+Shift+B` (build) or `Ctrl+Shift+P` → `Tasks: Run Task`

### `.vscode/launch.json`
- Configured for Cortex-Debug debugging
- Requires OpenOCD and Cortex-Debug extension

### `.vscode/extensions.json`
- Recommends essential extensions

## Building Your Project

### Option 1: Using CMake Tools UI (Recommended)
1. Open Command Palette: `Ctrl+Shift+P`
2. Select "CMake: Select a Configure Preset" → Choose "Debug"
3. Select "CMake: Build" or press `Ctrl+Shift+B`

### Option 2: Using Tasks
1. Press `Ctrl+Shift+B` for default build task
2. Or use `Ctrl+Shift+P` → `Tasks: Run Task` → Select task

### Option 3: Using Terminal
```powershell
cmake --preset=Debug
cmake --build build/Debug --config Debug
```

## Flashing to Device

### Method 1: Using STM32CubeProgrammer CLI (Easiest)
1. Connect ST-Link debugger to PC and device
2. Run task: `Ctrl+Shift+P` → `Tasks: Run Task` → "flash"

### Method 2: Using OpenOCD + Cortex-Debug
1. Install OpenOCD: https://openocd.org/
2. Press `F5` to start debugging (uses launch.json config)

### Method 3: Manual STM32CubeProgrammer GUI
1. Open STM32CubeProgrammer
2. Load: `build/Debug/Bakis.elf`
3. Connect and Program

## Troubleshooting

### "CMake: No Kit Selected"
- Use Command Palette: `CMake: Select a Kit`
- If no kits available, create one or use default

### "arm-none-eabi-gcc not found"
- Verify installation: `arm-none-eabi-gcc --version`
- Add to PATH if needed:
  - Windows: Add `C:\Program Files\GNU Arm Embedded Toolchain\arm-none-eabi\bin` to System PATH
  - Restart VS Code after PATH changes

### "Ninja not found"
- Either install Ninja or change CMake generator to "Unix Makefiles" in settings.json
- Change: `"cmake.generator": "Unix Makefiles"`

### "STM32_Programmer_CLI not found" (flashing task fails)
- Option 1: Add to PATH
- Option 2: Update tasks.json with full path:
  ```json
  "command": "C:\\Program Files\\STMicroelectronics\\STM32Cube\\STM32CubeProgrammer\\bin\\STM32_Programmer_CLI.exe"
  ```

### IntelliSense errors but build works
- Run `C_Cpp: Rescan Solutions` from Command Palette
- Restart VS Code
- Delete `.vscode/.intellisense/` folder if it exists

## Project Structure
```
Bakis/
├── Core/              # User code and main
│   ├── Inc/          # Headers
│   └── Src/          # Source files
├── Drivers/          # STM32 HAL and CMSIS
├── cmake/            # CMake configuration
│   ├── gcc-arm-none-eabi.cmake    # Toolchain file
│   └── stm32cubemx/   # CubeMX-generated CMake
├── CMakeLists.txt    # Project configuration
├── CMakePresets.json # Build presets
└── .vscode/          # VS Code configuration (NEW)
```

## Next Steps
1. Install all prerequisites (ARM GCC, CMake, Ninja)
2. Install VS Code extensions
3. Open project in VS Code
4. Select CMake preset (Debug/Release)
5. Build project with `Ctrl+Shift+B`
6. Connect ST-Link and flash with task

## Useful Resources
- [STM32 HAL Documentation](https://www.st.com/content/ccc/resource/technical/document/user_manual/2f/71/ba/f8/4a/3b/45/07/DM00154093.pdf)
- [CMake for Embedded](https://cmake.org/cmake/help/latest/manual/cmake-toolchains.7.html)
- [ARM GCC Documentation](https://developer.arm.com/documentation/100748/0619/)
