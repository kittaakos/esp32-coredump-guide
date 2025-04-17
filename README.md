# ESP32 Core Dump Collection with PlatformIO (WROOM-32)

Guide to generating and decoding ESP32 core dumps using PlatformIO on macOS. It compiles, uploads, crashes, and extracts core dumps.

## Project Structure

```
project/
├── src/
│   └── main.cpp
├── platformio.ini
├── partitions.csv
└── README.md
```

## 1. PlatformIO Configuration

### `platformio.ini`

```ini
[env:esp32dev]
platform = espressif32
board = esp32dev
framework = arduino
build_flags =
  -D CONFIG_LOG_DEFAULT_LEVEL=3
  -D CONFIG_ESP_COREDUMP_ENABLE=1
  -D CONFIG_ESP_COREDUMP_DATA_FORMAT_ELF=1
  -D CONFIG_ESP_COREDUMP_FLASH=1
  -D CONFIG_ESP_COREDUMP_CHECKSUM_CRC32=1
  -D CONFIG_ESP_COREDUMP_LOG_LVL=0
  -D CONFIG_ESP_COREDUMP_USE_STACK_SIZE=1
  -D CONFIG_ESP_COREDUMP_STACK_SIZE=1792
  -D CONFIG_ESP_COREDUMP_MAX_TASKS_NUM=64
  -D CONFIG_ESP_COREDUMP_CHECK_BOOT=1
monitor_speed = 115200
board_build.partitions = partitions.csv
```

### `partitions.csv`

```csv
# Name,   Type, SubType, Offset,     Size,     Flags
nvs,      data, nvs,     0x9000,     0x4000,
otadata,  data, ota,     0xd000,     0x2000,
app0,     app,  ota_0,   0x10000,    0x1C0000,
app1,     app,  ota_1,   0x1D0000,   0x1C0000,
spiffs,   data, spiffs,  0x390000,   0x60000,
coredump, data, coredump,0x3F0000,   0x10000,
```

## 2. Crash Test Sketch

### `src/main.cpp`

```cpp
#include <Arduino.h>

void setup() {
  Serial.begin(115200);
  while (!Serial);  // Wait for serial to connect (optional on some boards)

  delay(3000);  // Let USB settle
  Serial.println("Starting crash test...");
  Serial.flush();  // Make sure message is out

  delay(100);  // Let the flush complete (important!)
  *((volatile int*)nullptr) = 42;  // Intentional crash
}

void loop() {}
```

## 3. Build, Upload, and Monitor

```bash
brew install platformio
```

```bash
pio run
pio run -t upload
pio device monitor
```

Once crashed, a Guru Meditation Error will print.

---

## 4. Tools: ESP-IDF

Install [`vscode-esp-idf-extension`](https://github.com/espressif/vscode-esp-idf-extension) to VS Code.

### Install `esp-coredump` via pipx:

```bash
brew install pipx

# To add ~/.local/bin to the PATH for pipx
pipx ensurepath
pipx install esp-coredump
```

## 5. ESP-IDF Environment Setup

```bash
source ~/esp/$ISP_IDF_VERSION/esp-idf/export.sh
```

## 6. Retrieve and Decode Core Dump

### Save the core dump to file:

```bash
pipx run esp-coredump info_corefile \
  --core "" \
  --core-format elf \
  --off 0x3F0000 \
  --save-core coredump.elf \
  .pio/build/esp32dev/firmware.elf
```

 > `0x3F0000` is the coredump offset
