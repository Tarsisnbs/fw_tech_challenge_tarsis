# Firmware Engineering Challenge

## Overview

You are tasked with implementing firmware for a sensor gateway node running on an STM32F411RE microcontroller. The system receives binary telemetry packets from up to 4 sensors over a UART interface, stores the data in memory, monitors for sensor failures, and provides a diagnostic command interface.

This challenge runs entirely on your PC using the Renode simulation platform — no physical hardware is required. The firmware must operate under strict resource constraints typical of production embedded systems.

## Quick Start

**New to this challenge?** Run the automated setup:

```bash
./setup.sh                    # Downloads FreeRTOS + creates heap_none.c
cd firmware
mkdir build
cd build
cmake ..
make -j$(nproc)
```

Then see [SETUP.md](SETUP.md) for detailed build and simulation instructions.

## System Requirements

### Prerequisites

Install the following tools on your development machine:

- **ARM GCC Toolchain**: `gcc-arm-none-eabi`
  - Ubuntu/Debian: `sudo apt install gcc-arm-none-eabi`
  - macOS: `brew install --cask gcc-arm-embedded`
  - Windows: Use WSL2 with Ubuntu

- **Renode** (v1.15.0+): Embedded systems simulator
  - Download: https://github.com/renode/renode/releases
  - Ubuntu: Download .deb and `sudo apt install ./renode_*.deb`
  - macOS: `brew install --cask renode`

- **CMake** (v3.15+)
  - Ubuntu/Debian: `sudo apt install cmake`
  - macOS: `brew install cmake`

- **Python 3** and Robot Framework (for testing)
  - `pip3 install robotframework robotframework-renode`

### Target Platform

- **MCU**: STM32F411RE (Cortex-M4, 512KB Flash, 128KB RAM)
- **RTOS**: FreeRTOS v10.x with static allocation only
- **Toolchain**: `arm-none-eabi-gcc`
- **Simulator**: Renode

## Building the Firmware

**First time setup:**
```bash
./setup.sh          # Downloads FreeRTOS v10.6.2 + creates heap_none.c
```

**Build:**
```bash
cd firmware
mkdir build         # Create build directory
cd build
cmake ..           # Configure build system
make -j$(nproc)    # Compile firmware
```

**Verify:**
```bash
arm-none-eabi-size firmware.elf   # Check RAM/Flash usage
ls -lh firmware.*                 # List build artifacts
```

The build produces:
- `firmware.elf` - Executable for Renode simulation
- `firmware.bin` - Raw binary image
- `firmware.hex` - Intel HEX format
- `firmware.map` - Memory map (linker output)

## Running the Simulation

From the repository root:

```bash
renode platform/run.resc
```

This starts Renode and loads your firmware. The diagnostic shell is available on TCP port 5555.

### Connecting to the Diagnostic Shell

The firmware provides an interactive command shell over UART1 (exposed as TCP port 5555):

**Linux / macOS:**
```bash
telnet localhost 5555
```

**Windows (without WSL):**
- Download PuTTY: https://www.putty.org/
- Set Connection Type: **Raw**
- Host Name: `localhost`
- Port: `5555`
- Click **Open**

**Windows (with WSL):**
```bash
wsl
telnet localhost 5555
```

Type commands like `STATUS`, `DUMP`, `?` and press Enter to interact with the firmware.

## What to Implement

Your task is to implement the firmware logic in `firmware/src/fw_main.c` (and any additional source files you create). The file currently contains only stub functions — you must implement the complete system behavior.

### Functional Requirements

#### 1. Binary Packet Reception (UART2)

The firmware receives binary sensor telemetry packets on UART2 (115200 baud). Each packet has the following format:

```
┌──────┬──────┬───────────┬─────────────┬─────────────┬──────┐
│ 0xA5 │ 0x5A │ sensor_id │ payload_len │ payload[N]  │ CRC8 │
└──────┴──────┴───────────┴─────────────┴─────────────┴──────┘
  (header)      (1 byte)    (1 byte)      (1-16 bytes)  (1 byte)
```

**Requirements:**
- **Header**: Must be exactly `0xA5 0x5A`
- **sensor_id**: `0x00` to `0x03` (4 sensors maximum)
- **payload_len**: `1` to `16` bytes
- **payload**: Variable-length sensor data
- **CRC8**: Dallas/Maxim polynomial (0x31), computed over all bytes including header, sensor_id, payload_len, and payload
- **Error handling**:
  - Packets with invalid CRC must be **silently discarded**
  - Arbitrary noise bytes before a valid header must be **ignored**
  - Parser must re-sync on the next valid `0xA5 0xA` header

**Implementation notes:**
- UART2 delivers one byte at a time via an ISR callback
- Use a FreeRTOS queue to hand bytes from ISR to a task for processing
- The byte-by-byte parser must handle stream re-synchronization

#### 2. Sensor Data Storage

For each valid packet received, store the payload in a per-sensor ring buffer:

- **4 sensors** maximum (IDs `0x00` to `0x03`)
- **8 slots per sensor** (ring buffer)
- All storage must be **statically allocated** (no `malloc`)

Per sensor, track:
- `last_payload`: Most recent payload received
- `sample_count`: Total number of valid packets ever received (not capped at 8)
- `last_rx_tick`: FreeRTOS tick count when last packet arrived
- `registered`: Boolean — has this sensor sent at least one packet?

#### 3. Watchdog LED (PA5)

A dedicated FreeRTOS task must toggle GPIO PA5 every **500 ms** unconditionally.

**Requirements:**
- This task must NEVER block on any shared resource
- If PA5 stops toggling for >1000ms, it indicates scheduler starvation (test will fail)
- Task stack: ≤ 512 bytes

#### 4. Fault Detection (PA6)

A dedicated fault monitor task checks periodically (every **200 ms**) whether any registered sensor has been silent for more than **2000 ms**:

- **If fault detected**: Set GPIO PA6 HIGH
- **When all faults cleared**: Set GPIO PA6 LOW
- **Important**: A sensor that has never sent any packet (`registered == false`) must NEVER trigger a fault

#### 5. Diagnostic Shell (UART1)

UART1 operates as a bidirectional ASCII command shell. Commands are newline-terminated (`\n`).

| Command  | Response Format | Description |
|----------|----------------|-------------|
| `STATUS` | `OK sensors=N fault=F\n` | N = count of registered sensors, F = 0 or 1 |
| `DUMP`   | `S<id> last=<hex4> count=<N>\n` for each sensor, then `END\n` | List all registered sensors |
| `RESET`  | `OK\n` | Clear all sensor state (counts, timestamps, registered flags) |
| `?`      | List of commands, then `END\n` | Help |
| (unknown) | `ERR unknown\n` | Any unrecognized command |

**DUMP format details:**
- `<id>`: Sensor ID (0-3)
- `<hex4>`: First two bytes of most recent payload, zero-padded (e.g., `ABCD`)
- `<N>`: Total historical packet count (NOT capped at 8)
- Unregistered sensors (never seen) must NOT appear in output
- Must acquire sensor store mutex before reading

#### 6. GPIO Trigger (PB0)

When GPIO PB0 receives a **falling-edge interrupt**, the firmware must automatically emit a `DUMP` response on UART1 (identical to receiving the `DUMP` command).

---

### Key Architectural Considerations

- **Separation of concerns**: Keep protocol parsing separate from UART hardware access
- **Thread safety**: Use mutexes for shared resources (sensor store, UART1 TX)
- **ISR discipline**: Callbacks run in interrupt context - use queues to communicate with tasks
- **Modularity**: Consider creating separate modules for protocol parsing, command handling, and storage

## Resource Constraints

These constraints are **verified automatically by CI**:

| Constraint | Limit | Enforcement |
|------------|-------|-------------|
| Total RAM (`.data` + `.bss`) | ≤ 6 KB | `arm-none-eabi-size` check |
| Total static stack allocation | ≤ 2 KB | Declared static arrays |
| Dynamic allocation after `fw_init()` | **ZERO** | `heap_none.c` causes hard fault on `pvPortMalloc` |
| FreeRTOS allocation mode | Static only | `configSUPPORT_STATIC_ALLOCATION = 1` |
| Use of `printf` | **FORBIDDEN** | `grep` check (use `hal_uart1_send_str` instead) |

**All FreeRTOS objects must use static APIs:**
- `xTaskCreateStatic()` (NOT `xTaskCreate()`)
- `xQueueCreateStatic()` (NOT `xQueueCreate()`)
- `xSemaphoreCreateMutexStatic()` (NOT `xSemaphoreCreateMutex()`)

## Files You Can Modify

**You MAY modify:**
- `firmware/src/fw_main.c` — Your main implementation
- Create additional `.c` and `.h` files in `firmware/src/` as needed

**You MUST NOT modify:**
- Any file marked with `/* DO NOT MODIFY */` at the top
- `platform/` directory (Renode configuration)
- `firmware/include/hal_*.h` (HAL interfaces)
- `firmware/CMakeLists.txt` (build system)
- `firmware/STM32F411_FLASH.ld` (linker script)
- `firmware/startup_stm32f411xe.s` (startup code)
- `tests/public/` (test suite)

## Running Tests

A public smoke test suite verifies basic functionality:

```bash
cd tests/public
renode-test smoke.robot
```

**IMPORTANT**: A public smoke test suite is provided in `tests/public/`. It verifies basic functionality and is intended to help you confirm your firmware is on the right track. Additional private tests covering system behavior under stress will be run during evaluation. Writing code that only passes the visible tests is not sufficient — design for correctness, not for the tests you can see.

## Submission

### Step 1: Create Your Repository

Click the **"Use this template"** button at the top of this repository to create your own copy:

1. Name your repository (e.g., `fw-challenge-yourname`)
2. Choose visibility:
   - **Public** if you want to showcase your work
   - **Private** if you prefer to keep it confidential
3. Click "Create repository from template"

### Step 2: Implement Your Solution

```bash
# Clone your repository
git clone https://github.com/yourusername/fw-challenge-yourname
cd fw-challenge-yourname

# Set up and build
./setup.sh
cd firmware/build
cmake ..
make

# Implement your solution in firmware/src/fw_main.c
# (You can create additional .c/.h files as needed)
```

### Step 3: Verify Your Solution

```bash
# Run public tests
cd tests/public
renode-test smoke.robot

# All 6 tests should pass ✓
```

Ensure your repository includes:
- Working firmware that builds successfully
- All public tests passing
- Clean commit history with meaningful messages

### Step 4: Submit Your Work

Choose **one** of the following submission methods:

#### Option A: Email Repository URL

1. Push your final solution to your repository
2. Email us at **[hiring@yourcompany.com]**:
   - **Subject:** Firmware Challenge Submission - [Your Name]
   - **Body:**
     ```
     Name: [Your Full Name]
     Repository: https://github.com/yourusername/fw-challenge-yourname
     Completion Date: [Date]

     Notes: [Optional - anything you'd like us to know]
     ```

If your repository is **private**, also complete **Option B** below so we can access it.

#### Option B: Add Evaluator as Collaborator

If your repository is private:

1. Go to your repository on GitHub
2. Navigate to: **Settings** → **Collaborators** → **Add people**
3. Add our evaluator: **[evaluator-github-username]**
   - We'll send you this username via email
4. Grant **Read** access (default)
5. Email us confirming access has been granted

**Both options are acceptable** - choose what you're most comfortable with!

## Evaluation Criteria

Your submission will be evaluated on:

1. **Correctness** — Does the firmware meet all functional requirements?
2. **Architecture** — Is the code well-structured with clear module boundaries?
3. **Resource discipline** — Does the code respect memory and stack constraints?
4. **Edge case handling** — Does it handle protocol errors, timeouts, and race conditions?
5. **Code quality** — Is the code readable, maintainable, and well-documented?

## Tips

- Start simple: Get the watchdog LED toggling first (verifies scheduler is running)
- Test incrementally: Validate each requirement before moving to the next
- Use the diagnostic shell: Send commands via telnet to debug your implementation
- Check resource usage: Run `arm-none-eabi-size firmware/build/firmware.elf` frequently
- Remember static allocation: Declare all task stacks, queues, and mutexes as static arrays

## Resources

- **FreeRTOS documentation**: https://www.freertos.org/Documentation/RTOS_book.html
- **STM32F411RE reference manual**: https://www.st.com/resource/en/reference_manual/dm00119316.pdf
- **Renode documentation**: https://renode.readthedocs.io/
- **CRC8 calculator** (for testing): `scripts/crc8.py`

## Support

If you encounter build issues, simulation problems, or have questions about requirements, please open an issue in this repository.

Good luck!
