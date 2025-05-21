# rf24server

A C++ ground base station program for communicating with NRF24L01+ drones over RF.  
Built with modular CMake structure and supports automatic leader selection, telemetry polling, and debug printing.

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
git clone https://github.com/SeneAlukard/rf24server.git
cd rf24server
```

### 2. Build with CMake

```bash
mkdir build
cd build
cmake ..
make
```

This will generate the executable `gbs_program` in the `build/` directory.

### 3. (Optional) Symlink compile_commands.json

If you're using `clangd` or LSPs:

```bash
ln -sf build/compile_commands.json .
```

---

## 🧪 Running the Program

From the build folder:

```bash
./gbs_program
```

Or from the root:

```bash
./build/gbs_program
```

---

## 💡 Features

- NRF24L01+ RF communication with drones
- Modular Drone class design
- Leader selection algorithm based on RSSI
- Pretty vector printing via `printSet`
- `#pragma once` headers and project-wide include system
- CMake auto-symlinks `compile_commands.json` for LSP support

---

## 🛑 Git Ignore Policy

We ignore:

- `build/` directory
- `obj/`, `*.o`, and binaries
- `compile_commands.json`
- Editor-specific files (`.cache/`, `.vscode/`)

---

## 📜 License

MIT License 

---
