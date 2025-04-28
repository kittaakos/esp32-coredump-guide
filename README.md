# ESP32 Core Dump Collection with Arduino CLI (WROOM-32)

Guide to generating and decoding ESP32 core dumps using [Arduino CLI](https://github.com/arduino/arduino-cli) and the [ESP32 Arduino core](https://espressif.github.io/arduino-esp32/).
This guide shows how to compile, upload, crash, and extract core dumps.
Based on [this Arduino Forum post](https://forum.arduino.cc/t/can-arduino-cli-customize-esp32-core-dumps-config-with-custom-parition-and-download-it/1375658/2).

### Install Arduino Core for ESP32

Install the ESP32 Arduino core:

```sh
./arduino-cli \
  core install esp32:esp32 \
  --additional-urls https://espressif.github.io/arduino-esp32/package_esp32_index.json
```

### Compile

Compile your project with coredump support:

```sh
./arduino-cli compile \
  --fqbn esp32:esp32:esp32da \
  --build-property "compiler.c.extra_flags=-D CONFIG_LOG_DEFAULT_LEVEL=3 \
-D CONFIG_ESP_COREDUMP_ENABLE=1 \
-D CONFIG_ESP_COREDUMP_DATA_FORMAT_ELF=1 \
-D CONFIG_ESP_COREDUMP_FLASH=1 \
-D CONFIG_ESP_COREDUMP_CHECKSUM_CRC32=1 \
-D CONFIG_ESP_COREDUMP_LOG_LVL=0 \
-D CONFIG_ESP_COREDUMP_USE_STACK_SIZE=1 \
-D CONFIG_ESP_COREDUMP_STACK_SIZE=1792 \
-D CONFIG_ESP_COREDUMP_MAX_TASKS_NUM=64 \
-D CONFIG_ESP_COREDUMP_CHECK_BOOT=1" \
  --build-property "compiler.cpp.extra_flags=-D CONFIG_LOG_DEFAULT_LEVEL=3 \
-D CONFIG_ESP_COREDUMP_ENABLE=1 \
-D CONFIG_ESP_COREDUMP_DATA_FORMAT_ELF=1 \
-D CONFIG_ESP_COREDUMP_FLASH=1 \
-D CONFIG_ESP_COREDUMP_CHECKSUM_CRC32=1 \
-D CONFIG_ESP_COREDUMP_LOG_LVL=0 \
-D CONFIG_ESP_COREDUMP_USE_STACK_SIZE=1 \
-D CONFIG_ESP_COREDUMP_STACK_SIZE=1792 \
-D CONFIG_ESP_COREDUMP_MAX_TASKS_NUM=64 \
-D CONFIG_ESP_COREDUMP_CHECK_BOOT=1" \
  src/esp32backtracetest
```

### Upload

Upload the firmware to your board:

```sh
./arduino-cli upload \
  --fqbn esp32:esp32:esp32da \
  --port /dev/cu.usbserial-0001 \
  src/esp32backtracetest
```

> **ⓘ** Use `./arduino-cli board list` to find your device's port.

### Monitor

Open a serial monitor (optional):

```sh
./arduino-cli monitor \
  --port /dev/cu.usbserial-0001 \
  --config baudrate=115200
```

### Find esptool

Find the path to `esptool`:

```sh
esptool_dir=$(./arduino-cli board details \
  --fqbn esp32:esp32:esp32da \
  --format json | \
  jq -r '.build_properties[]
    | select(startswith("runtime.tools.esptool_py.path="))
    | split("=")[1]'
)
```

### Read core dump

Read the core dump from flash:

```sh
$esptool_dir/esptool \
  --chip esp32 \
  --port /dev/cu.usbserial-0001 \
  --baud 115200 \
  read_flash 0x3F0000 0x10000 coredump.elf
```

> **ⓘ** `0x3F0000` is the coredump partition offset. `0x10000` is the partition size.

---

### Acknowledgment

Thanks to [ptillisch](https://forum.arduino.cc/u/ptillisch) for the helpful instructions.
