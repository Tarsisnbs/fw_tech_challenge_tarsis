# Firmware Challenge - Project Summary

## Repository Overview

This repository contains a complete, production-ready firmware technical challenge designed to evaluate mid-to-senior embedded firmware engineers. The challenge runs entirely on PC using Renode simulation — no physical hardware required.

**Target**: STM32F411RE with FreeRTOS (static allocation only)
**Platform**: Renode v1.15+ simulation
**Duration**: 7 days

## Directory Structure

```
fw-challenge/
├── platform/                       # Renode simulation configuration
│   ├── stm32f411.repl             # Platform description (READ ONLY)
│   └── run.resc                   # Boot script (READ ONLY)
│
├── tests/
│   └── public/                    # Visible to candidate
│       └── smoke.robot            # 6 basic smoke tests
│
├── firmware/
│   ├── CMakeLists.txt             # Build system (READ ONLY)
│   ├── STM32F411_FLASH.ld         # Linker script (READ ONLY)
│   ├── startup_stm32f411xe.s      # Startup code (READ ONLY)
│   │
│   ├── include/
│   │   ├── hal_uart.h             # UART HAL interface (READ ONLY)
│   │   ├── hal_gpio.h             # GPIO HAL interface (READ ONLY)
│   │   ├── fw_main.h              # Entry points (READ ONLY)
│   │   ├── stm32f411xe.h          # Register definitions (READ ONLY)
│   │   └── FreeRTOSConfig.h       # FreeRTOS config (READ ONLY)
│   │
│   ├── src/
│   │   ├── hal_uart.c             # UART HAL implementation (provided)
│   │   ├── hal_gpio.c             # GPIO HAL implementation (provided)
│   │   ├── main.c                 # Harness (provided)
│   │   ├── fw_main.c              # ⭐ CANDIDATE IMPLEMENTS HERE ⭐
│   │   └── malloc_wrapper.c       # Malloc tracker (provided)
│   │
│   └── freertos/                  # Downloaded by setup.sh
│       ├── include/               # FreeRTOS headers
│       ├── portable/              # ARM CM4F port + heap_none.c
│       └── *.c                    # FreeRTOS kernel source
│
├── scripts/
│   └── crc8.py                    # CRC8 calculator for tests
│
├── .github/
│   └── workflows/
│       └── grade.yml              # CI: build + size checks + public tests
│
├── setup.sh                       # Downloads FreeRTOS v10.6.2
├── verify_structure.sh            # Verifies repo completeness
├── .gitignore                     
├── README.md                      # Challenge instructions (candidate-facing)
├── SETUP.md                       # Detailed setup guide
├── EVALUATION_GUIDE.md            # Evaluator guide (gitignored, NOT in public repo)
└── PROJECT_SUMMARY.md             # This file
```

## Two-Repository Strategy

This challenge uses a dual-repository approach:

### Public Repository (Candidate Template)
- Contains: Everything EXCEPT `tests/private/` and `EVALUATION_GUIDE.md`
- `.gitignore` excludes private files
- CI runs only `tests/public/smoke.robot`
- **Configured as GitHub Template Repository**
- Candidates use "Use this template" to create their own copy
- Submission via:
  - **Option A:** Email repository URL (public or private)
  - **Option B:** Add evaluator as collaborator (for private repos)

### Private Repository (Evaluator)
- Contains: Everything including private tests
- Used to run `tests/private/edge_cases.robot` against submissions
- Results inform technical interview discussion

## Functional Requirements Summary

| Requirement | Description | Key Validation |
|-------------|-------------|----------------|
| **RF-1**: Packet RX | Binary protocol on UART2: `0xA5 0x5A [id] [len] [payload] [CRC8]` | CRC validation, noise resync |
| **RF-2**: Storage | 4 sensors × 8-slot ring buffers, statically allocated | Mutex protection |
| **RF-3**: Watchdog | PA5 toggles every 500ms unconditionally | No blocking on shared resources |
| **RF-4**: Fault detect | PA6 HIGH if registered sensor silent >2000ms | Per-sensor tracking, `registered` flag |
| **RF-5**: Shell | UART1 commands: STATUS, DUMP, RESET, ? | Newline-terminated ASCII |
| **RF-6**: GPIO trigger | PB0 falling edge → auto-DUMP on UART1 | Tests abstraction (shared function) |

## Resource Constraints (Auto-Verified)

| Constraint | Limit | CI Check |
|------------|-------|----------|
| RAM (data+bss) | ≤ 6 KB | `arm-none-eabi-size` |
| Task stacks | ≤ 2 KB total | Static arrays |
| Dynamic alloc | ZERO | `heap_none.c` + malloc counter |
| FreeRTOS mode | Static only | `configSUPPORT_STATIC_ALLOCATION=1` |
| printf usage | FORBIDDEN | `grep -r "printf" firmware/src/` |

## Test Suites

### Public Tests (6 tests - visible to candidate)
1. **T01**: PA5 toggles after boot
2. **T02**: Valid packet is stored
3. **T03**: STATUS command responds
4. **T04**: RESET clears state
5. **T05**: Unknown command returns ERR
6. **T06**: Help command lists commands

## Evaluation Rubric

### Architecture
- Module boundaries and interfaces
- Separation of concerns (parser vs. storage vs. HAL)
- Code organization and structure


### Abstraction
- Domain-level wrappers vs. scattered HAL calls
- Transport independence (could UART2 be swapped easily?)

### Resource Constraints
- Static allocation discipline (all `*Static()` APIs used)
- Deliberate sizing vs. arbitrary magic numbers
- Stack size reasoning and documentation
- Minimal waste (no unnecessary globals)

## Setup and Build

### Quick Setup
```bash
./setup.sh              # Downloads FreeRTOS v10.6.2 + creates heap_none.c
cd firmware
mkdir build
cd build
cmake ..
make -j$(nproc)
```

### Run Simulation
```bash
renode platform/run.resc
```

### Connect Diagnostic Shell
```bash
telnet localhost 5555
```

### Run Tests
```bash
# Public tests (candidate can run)
cd tests/public
renode-test smoke.robot

# Private tests (evaluator only)
cd tests/private
renode-test other_cases.robot
```

## Key Technical Details

### Hardware Interfaces (Simulated)
- **UART2** (PA2/PA3): Sensor telemetry input, RX only, 115200 baud
- **UART1** (PA9/PA10): Diagnostic shell, TX+RX, 115200 baud
- **PA5**: Watchdog LED output (toggles every 500ms)
- **PA6**: Fault indicator output (HIGH when sensor timeout detected)
- **PB0**: Command trigger input (falling-edge IRQ → auto-DUMP)

### Binary Protocol Details
```
Header: 0xA5 0x5A (fixed)
Sensor ID: 0x00 to 0x03 (4 sensors)
Payload length: 1 to 16 bytes
CRC8: Dallas/Maxim polynomial 0x31
```

CRC computed over: `[0xA5] [0x5A] [sensor_id] [payload_len] [payload[0]...payload[N-1]]`

### Malloc Tracking
- `malloc_wrapper.c` intercepts `malloc()` calls
- Increments counter at fixed address `0x20000000`
- Test T14 reads this counter via `sysbus ReadDoubleWord 0x20000000`
- With `heap_none.c`, any `pvPortMalloc()` call causes hard fault

## Files Candidates Can Modify

✅ **Allowed**:
- `firmware/src/fw_main.c` — main implementation
- Create new `.c` and `.h` files in `firmware/src/`

❌ **Forbidden** (marked with `/* DO NOT MODIFY */`):
- All files in `platform/`
- All files in `firmware/include/` (HAL interfaces, config)
- `firmware/CMakeLists.txt`, linker script, startup code
- All files in `tests/`
- HAL implementation files (`hal_*.c`, `main.c`)

## CI/CD Pipeline

GitHub Actions workflow (`.github/workflows/grade.yml`):

1. Install ARM toolchain, Renode, Robot Framework
2. Build firmware
3. Check binary exists
4. **RAM check**: `.data + .bss ≤ 6144` bytes
5. **No-printf check**: `grep -r "printf" firmware/src/`
6. **Static allocation check**: `configSUPPORT_STATIC_ALLOCATION = 1`
7. Run `tests/public/smoke.robot`
8. Upload artifacts (firmware binaries, test results)

## References

- **STM32F411RE datasheet**: https://www.st.com/resource/en/reference_manual/dm00119316.pdf
- **FreeRTOS docs**: https://www.freertos.org/Documentation/RTOS_book.html
- **Renode docs**: https://renode.readthedocs.io/
- **CRC8 Dallas/Maxim**: Polynomial 0x31, see `scripts/crc8.py`

## Support

For questions or issues:
1. Check SETUP.md for troubleshooting
2. Review README.md for requirements
3. Check EVALUATION_GUIDE.md (evaluators only)
4. Open issue in repository

---

**Generated**: 2025-03-19
**Version**: 1.0
