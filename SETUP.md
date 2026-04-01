# Setup Instructions

## Quick Start

1. **Install prerequisites** (see below)
2. **Run setup script** to download FreeRTOS and create required files:
   ```bash
   ./setup.sh
   ```
   This will:
   - Download FreeRTOS v10.6.2
   - Create `heap_none.c` (disables dynamic allocation)

3. **Build firmware**:
   ```bash
   cd firmware
   mkdir build
   cd build
   cmake ..
   make -j$(nproc)
   ```

   **Note:** The build directory must be created manually. The setup script only downloads dependencies.

4. **Verify build**:
   ```bash
   ls -lh firmware.elf          # Should see ~60KB file
   arm-none-eabi-size firmware.elf   # Check RAM usage
   ```

5. **Run simulation** (from repository root):
   ```bash
   renode platform/run.resc
   ```

6. **Connect to diagnostic shell** (in another terminal):
   ```bash
   telnet localhost 5555
   ```

   **Note:** With the stub firmware (before implementation), the connection will succeed but you won't see any output or responses. This is expected - the firmware must be implemented to initialize UART and process commands. Once you implement `fw_init()` and the diagnostic shell handler, you'll see command responses.

## Detailed Prerequisites

### Ubuntu / Debian

```bash
# ARM toolchain
sudo apt-get update
sudo apt-get install -y gcc-arm-none-eabi binutils-arm-none-eabi

# CMake and build tools
sudo apt-get install -y cmake make

# Python and Robot Framework
sudo apt-get install -y python3 python3-pip
pip3 install robotframework robotframework-renode

# Renode (download latest .deb from GitHub)
wget https://github.com/renode/renode/releases/download/v1.15.0/renode_1.15.0_amd64.deb
sudo apt-get install -y ./renode_1.15.0_amd64.deb

# Verify installation
arm-none-eabi-gcc --version
cmake --version
renode --version

# Now run setup and build
cd /path/to/fw-challenge
./setup.sh
cd firmware && mkdir build && cd build
cmake .. && make -j$(nproc)
```

### macOS

```bash
# Install Homebrew if not already installed
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

# ARM toolchain
brew install --cask gcc-arm-embedded

# CMake
brew install cmake

# Python and Robot Framework
pip3 install robotframework robotframework-renode

# Renode
brew install --cask renode

# Verify installation
arm-none-eabi-gcc --version
cmake --version
renode --version
```

### Windows (via WSL2)

1. **Install WSL2** with Ubuntu:
   ```powershell
   wsl --install -d Ubuntu-22.04
   ```

2. **Inside WSL terminal**, follow Ubuntu instructions above

3. **For Renode GUI** (optional), install VcXsrv on Windows:
   - Download: https://sourceforge.net/projects/vcxsrv/
   - Run XLaunch with "Disable access control" checked

## Troubleshooting

### "arm-none-eabi-gcc: command not found"

- **Ubuntu**: Run `sudo apt-get install gcc-arm-none-eabi`
- **macOS**: Run `brew install --cask gcc-arm-embedded`
- **WSL**: Ensure you ran the setup commands inside the WSL terminal, not Windows PowerShell

### "renode: command not found"

- **Ubuntu**: Download the .deb manually from https://github.com/renode/renode/releases
- **macOS**: Run `brew install --cask renode`
- Verify with `renode --version`

### CMake errors about missing FreeRTOS files

Run the setup script:
```bash
./setup.sh
```

This downloads FreeRTOS v10.6.2 into `firmware/freertos/` and creates the required `heap_none.c` file.

### Build fails: "does not appear to contain CMakeLists.txt"

You must create the build directory and run cmake from inside it:

```bash
cd firmware
mkdir build       # Create build directory first!
cd build
cmake ..          # Parent directory contains CMakeLists.txt
make
```

**Wrong:**
```bash
cd firmware
cmake ..          # ERROR - CMakeLists.txt is in current dir, not parent
```

**Correct:**
```bash
cd firmware
mkdir build
cd build
cmake ..          # Now ".." correctly points to firmware/
make
```

### "telnet: command not found"

- **Ubuntu**: `sudo apt-get install telnet`
- **macOS**: telnet is built-in, use `nc localhost 5555` as alternative
- **Windows**: Use PuTTY (Raw mode, localhost:5555) or install telnet in WSL

### Build fails with "No such file or directory: 'heap_none.c'"

The FreeRTOS portable memory manager is missing. Ensure you ran `./setup.sh` and that `firmware/freertos/portable/MemMang/heap_none.c` exists.

### Renode simulation starts but firmware doesn't respond

1. Check that `firmware/build/firmware.elf` was created successfully
2. Verify it's loaded: In Renode console, type `sysbus GetSymbolAddress "Reset_Handler"`
3. Check UART1 is connected: `connector Show`
4. Re-run build: `cd firmware/build && make clean && make`

### "Permission denied" when running scripts

Make scripts executable:
```bash
chmod +x setup.sh scripts/crc8.py
```

## Manual FreeRTOS Setup (if setup.sh fails)

If the automated setup script doesn't work, download FreeRTOS manually:

1. Download FreeRTOS Kernel v10.6.2:
   ```bash
   wget https://github.com/FreeRTOS/FreeRTOS-Kernel/archive/refs/tags/V10.6.2.tar.gz
   ```

2. Extract to `firmware/freertos/`:
   ```bash
   mkdir -p firmware/freertos
   tar -xzf V10.6.2.tar.gz -C firmware/freertos --strip-components=1
   ```

3. Verify structure:
   ```
   firmware/freertos/
   ├── include/
   │   ├── FreeRTOS.h
   │   ├── task.h
   │   ├── queue.h
   │   └── ...
   ├── portable/
   │   ├── GCC/
   │   │   └── ARM_CM4F/
   │   │       ├── port.c
   │   │       └── portmacro.h
   │   └── MemMang/
   │       └── heap_none.c
   └── *.c (tasks.c, queue.c, list.c, timers.c)
   ```

## Next Steps

After setup is complete:

1. **Read the README.md** for challenge requirements
2. **Implement firmware** in `firmware/src/fw_main.c`
3. **Run tests**: `cd tests/public && renode-test smoke.robot`
4. **Iterate** until all tests pass

## Getting Help

If you encounter issues not covered here:

1. Check the [Renode documentation](https://renode.readthedocs.io/)
2. Review [FreeRTOS documentation](https://www.freertos.org/)
3. Open an issue in this repository with:
   - Your OS and versions (`uname -a`, `arm-none-eabi-gcc --version`)
   - Full error output
   - Steps to reproduce
