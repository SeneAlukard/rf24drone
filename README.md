# rf24drone

A C++ ground base station program for communicating with NRF24L01+ drones over RF.  
Built with modular CMake structure and supports leader selection based on link quality metrics, telemetry polling, and debug printing.

---

## 📁 Project Structure

```
.
├── include/        # Public headers
├── src/            # Source files
├── build/          # CMake build output (ignored by git)
├── CMakeLists.txt  # Project build system
└── Makefile        # Optional: For compatibility or alternate build
```

---

## ⚙️ Requirements

- C++17 compiler (`g++` or `clang++`)
- [CMake](https://cmake.org/) ≥ 3.12
- [RF24 library](https://github.com/nRF24/RF24)
- (Optional) `make`

---

## 🚀 Building the Project

### 1. Clone the Repository

```bash
git clone git@github.com:SeneAlukard/rf24drone.git
cd rf24drone
git submodule update --init --recursive
```

### 2. Build with CMake

```bash
mkdir build
cd build
cmake ..
make
```

This will generate the executable `drone` in the `build/` directory and
create a symlink `./drone` at the project root for convenience. Test
executables are placed under `test/`.

### 3. (Optional) Symlink compile_commands.json

If you're using `clangd` or LSPs:

```bash
ln -sf build/compile_commands.json .
```

---

## 🧪 Running the Program

From either the build directory or the project root (via the symlink):

```bash
./drone
```

The test binary can be run from the `test/` folder:

```bash
./test/duplex_test
```

---

## 💡 Features

- NRF24L01+ RF communication with drones
- Modular Drone class design
- Leader selection based on link quality data
- Pretty vector printing via `printSet`
- `#pragma once` headers and project-wide include system
- CMake auto-symlinks `compile_commands.json` for LSP support

### Telemetry Link Quality

Every telemetry packet reports additional link statistics:

- `rpd` – result of `testRPD()` (1 if signal > -64 dBm)
- `retries` – number of auto retransmissions used
- `link_quality` – success rate percentage of telemetry sends

---

