# rf24drone

A lightweight NRF24L01+ drone application. Each node can run as a leader or
follower, joins the network with a `JoinRequest`/`JoinResponse` handshake and can
dynamically change roles. Telemetry is transmitted only after receiving
`PermissionToSend` packets to avoid collisions on half‑duplex radios.

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

### Reading raw MPU6050 data

An extra example is provided to print sensor values over I2C:

```bash
make mpu_terminal
./examples/mpu_terminal
```

---

## 💡 Features

- NRF24L01+ RF communication for a small swarm
- Join/response handshake assigns IDs and channel
- Heartbeat & leader announcement packets for dynamic role changes
- Telemetry sent only after `PermissionToSend`
- Commands ignored if older than 3 seconds
- Telemetry packets contain link quality stats (`rpd`, `retries`, `link_quality`)
- CMake auto-symlinks `compile_commands.json` for LSP support

---

