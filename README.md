# ESP32 Coredump Guide: Arduino CLI + TraceBreaker (WROOM-32)

This guide shows how to configure an ESP32-WROOM-32 board to generate and decode coredumps using only the Arduino CLI, the ESP32 Arduino core, and TraceBreaker (`trbr`). You'll go through:

1. Install required tools ([Arduino CLI](https://arduino.github.io/arduino-cli/latest/installation/), [ESP32 core](#install-arduino-core-for-esp32), [`trbr`](https://github.com/dankeboy36/trbr?tab=readme-ov-file#installation)).
2. Enable ELF-formatted coredumps in your sketch.
3. Compile and upload firmware via Arduino CLI.
4. Trigger a crash to write the dump to flash.
5. Extract the raw flash partition containing the dump.
6. Decode and inspect the dump using `trbr`.

Adapted from [this Arduino Forum post](https://forum.arduino.cc/t/can-arduino-cli-customize-esp32-core-dumps-config-with-custom-parition-and-download-it/1375658/2).

### Prerequisites

Make sure the following command-line tools are available:

```sh
brew install jq pipx  # macOS
sudo apt install jq pipx  # Debian/Ubuntu
```

> **ⓘ** `jq` is used to extract values from the Arduino CLI JSON output. `pipx` is used to run Python-based tools like `esp-coredump` in isolation. `pipx` is optional if you do not want to use esp-coredump.

### Install Arduino Core for ESP32

Install the ESP32 Arduino core (one-time setup):

```sh
./arduino-cli \
  core install esp32:esp32 \
  --additional-urls https://espressif.github.io/arduino-esp32/package_esp32_index.json
```

### Setup

Set the environment variables, for example:

```sh
# sometimes the ESP-IDF chip does not match the Arduino FQBN's board ID
chip=esp32
board_id=esp32da
fqbn="esp32:esp32:$board_id"
port="/dev/cu.usbserial-0001"
```

> **ⓘ** Use the [`board search`](https://arduino.github.io/arduino-cli/latest/commands/arduino-cli_board_search/) command of the Arduino CLI to find your device's FQBN.

> **ⓘ** Use the [`board list`](https://arduino.github.io/arduino-cli/latest/commands/arduino-cli_board_list/) command of the Arduino CLI to find your device's port.

### Compile

Compile your project with coredump support:

```sh
elf_path=$(./arduino-cli compile \
  --fqbn $fqbn \
  --format json \
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
  src/esp32backtracetest | \
  jq -r '.builder_result.build_properties[]
    | select(startswith("debug.executable="))
    | split("=")[1]'
)
```

> **ⓘ** Save the compiled ELF to the `elf_path` variable.

### Upload

Upload the firmware to your board:

```sh
./arduino-cli upload \
  --fqbn $fqbn \
  --port $port \
  src/esp32backtracetest
```

> **⚠️** After uploading, allow the firmware to run for 10–15 seconds (or until a crash occurs) to ensure the coredump is written to flash. The crash must happen and be flushed before powering off or resetting the board.

### Monitor (optional)

Open a serial monitor:

```sh
./arduino-cli monitor \
  --port $port \
  --config baudrate=115200
```

### Find esptool

Find the path to `esptool`:

```sh
esptool_dir=$(./arduino-cli board details \
  --fqbn $fqbn \
  --format json | \
  jq -r '.build_properties[]
    | select(startswith("runtime.tools.esptool_py.path="))
    | split("=")[1]'
)
```

### Dump coredump partition from flash

Use `esptool` to read the flash partition that contains the coredump:

```sh
$esptool_dir/esptool \
  --chip $chip \
  --port $port \
  --baud 115200 \
  read_flash 0x3F0000 0x10000 coredump.raw
```

> **ⓘ** `0x3F0000` is the coredump partition offset. `0x10000` is the partition size.

### Decode coredump with `trbr`

After extracting the raw flash data (`coredump.raw`), decode it using `trbr`:

> **⚠️** The ELF used for decoding must match the binary running at the time of crash. SHA mismatches will result in decode failure.

```sh
./trbr decode \
  --input coredump.raw \
  --elf-path "$elf_path" \
  --fqbn $fqbn \
  --coredump-mode
```

<details><summary>Click to see the decode output</summary>
<p>

```
==================== THREADS INFO ====================
  ID  Target ID            Frame
 * 1  process 1073447304   0x400d15c5: functionC (num=42) at /Users/kittaakos/dev/sandbox/esp32-coredump-guide/src/esp32backtracetest/module2.cpp:9
   2  process 1073469216   0x400867ca: esp_cpu_wait_for_intr () at /home/runner/work/esp32-arduino-lib-builder/esp32-arduino-lib-builder/esp-idf/components/esp_hw_support/cpu.c:64
   3  process 1073469588   0x400867ca: esp_cpu_wait_for_intr () at /home/runner/work/esp32-arduino-lib-builder/esp32-arduino-lib-builder/esp-idf/components/esp_hw_support/cpu.c:64
   4  process 1073443696   0x4000bff0: ??
   5  process 1073445952   0x4000bff0: ??
   6  process 1073498360   0x40083160: esp_crosscore_int_send_yield (core_id=1) at /home/runner/work/esp32-arduino-lib-builder/esp32-arduino-lib-builder/esp-idf/components/esp_system/crosscore_int.c:121
   7  process 1073445240   0x4000bff0: ??

==================== THREAD 1 (TCB: 0x3ffb8188) ====================
0x400d15c5: functionC (num=42) at /Users/kittaakos/dev/sandbox/esp32-coredump-guide/src/esp32backtracetest/module2.cpp:9
0x400d15dd: functionB (ptr=0x3ffb221c) at /Users/kittaakos/dev/sandbox/esp32-coredump-guide/src/esp32backtracetest/module2.cpp:14
0x400d15b0: functionA (value=<optimized out>) at /Users/kittaakos/dev/sandbox/esp32-coredump-guide/src/esp32backtracetest/module1.cpp:7
0x400d1594: setup () at /Users/kittaakos/dev/sandbox/esp32-coredump-guide/src/esp32backtracetest/esp32backtracetest.ino:9
0x400d30b9: loopTask (pvParameters=0x0) at /Users/kittaakos/Library/Arduino15/packages/esp32/hardware/esp32/3.2.0/cores/esp32/main.cpp:59
0x40088ca4: vPortTaskWrapper (pxCode=0x400d309c <loopTask(void*)>, pvParameters=0x0) at /home/runner/work/esp32-arduino-lib-builder/esp32-arduino-lib-builder/esp-idf/components/freertos/FreeRTOS-Kernel/portable/xtensa/port.c:139

==================== THREAD 2 (TCB: 0x3ffbd720) ====================
0x400867ca: esp_cpu_wait_for_intr () at /home/runner/work/esp32-arduino-lib-builder/esp32-arduino-lib-builder/esp-idf/components/esp_hw_support/cpu.c:64
0x400d6715: esp_vApplicationIdleHook () at /home/runner/work/esp32-arduino-lib-builder/esp32-arduino-lib-builder/esp-idf/components/esp_system/freertos_hooks.c:58
0x40089c8e: prvIdleTask (pvParameters=0x0) at /home/runner/work/esp32-arduino-lib-builder/esp32-arduino-lib-builder/esp-idf/components/freertos/FreeRTOS-Kernel/tasks.c:4353
0x40088ca4: vPortTaskWrapper (pxCode=0x40089c18 <prvIdleTask>, pvParameters=0x0) at /home/runner/work/esp32-arduino-lib-builder/esp32-arduino-lib-builder/esp-idf/components/freertos/FreeRTOS-Kernel/portable/xtensa/port.c:139

==================== THREAD 3 (TCB: 0x3ffbd894) ====================
0x400867ca: esp_cpu_wait_for_intr () at /home/runner/work/esp32-arduino-lib-builder/esp32-arduino-lib-builder/esp-idf/components/esp_hw_support/cpu.c:64
0x400d6715: esp_vApplicationIdleHook () at /home/runner/work/esp32-arduino-lib-builder/esp32-arduino-lib-builder/esp-idf/components/esp_system/freertos_hooks.c:58
0x40089c8e: prvIdleTask (pvParameters=0x0) at /home/runner/work/esp32-arduino-lib-builder/esp32-arduino-lib-builder/esp-idf/components/freertos/FreeRTOS-Kernel/tasks.c:4353
0x40088ca4: vPortTaskWrapper (pxCode=0x40089c18 <prvIdleTask>, pvParameters=0x0) at /home/runner/work/esp32-arduino-lib-builder/esp32-arduino-lib-builder/esp-idf/components/freertos/FreeRTOS-Kernel/portable/xtensa/port.c:139

==================== THREAD 4 (TCB: 0x3ffb7370) ====================
0x4000bff0: ??
0x40088f11: vPortClearInterruptMaskFromISR () at /home/runner/work/esp32-arduino-lib-builder/esp32-arduino-lib-builder/esp-idf/components/freertos/FreeRTOS-Kernel/portable/xtensa/include/freertos/portmacro.h:560
0x40088f11: vPortExitCritical (mux=0x3ffbdd28 <xKernelLock>) at /home/runner/work/esp32-arduino-lib-builder/esp32-arduino-lib-builder/esp-idf/components/freertos/FreeRTOS-Kernel/portable/xtensa/port.c:514
0x4008ae69: ulTaskGenericNotifyTake (uxIndexToWait=0, xClearCountOnExit=1, xTicksToWait=4294967295) at /home/runner/work/esp32-arduino-lib-builder/esp32-arduino-lib-builder/esp-idf/components/freertos/FreeRTOS-Kernel/tasks.c:5765
0x4008231d: ipc_task (arg=<optimized out>) at /home/runner/work/esp32-arduino-lib-builder/esp32-arduino-lib-builder/esp-idf/components/esp_system/esp_ipc.c:62
0x40088ca4: vPortTaskWrapper (pxCode=0x400822dc <ipc_task>, pvParameters=0x0) at /home/runner/work/esp32-arduino-lib-builder/esp32-arduino-lib-builder/esp-idf/components/freertos/FreeRTOS-Kernel/portable/xtensa/port.c:139

==================== THREAD 5 (TCB: 0x3ffb7c40) ====================
0x4000bff0: ??
0x40088f11: vPortClearInterruptMaskFromISR () at /home/runner/work/esp32-arduino-lib-builder/esp32-arduino-lib-builder/esp-idf/components/freertos/FreeRTOS-Kernel/portable/xtensa/include/freertos/portmacro.h:560
0x40088f11: vPortExitCritical (mux=0x3ffbdd28 <xKernelLock>) at /home/runner/work/esp32-arduino-lib-builder/esp32-arduino-lib-builder/esp-idf/components/freertos/FreeRTOS-Kernel/portable/xtensa/port.c:514
0x4008ae69: ulTaskGenericNotifyTake (uxIndexToWait=0, xClearCountOnExit=1, xTicksToWait=4294967295) at /home/runner/work/esp32-arduino-lib-builder/esp32-arduino-lib-builder/esp-idf/components/freertos/FreeRTOS-Kernel/tasks.c:5765
0x400e1af2: timer_task (arg=<optimized out>) at /home/runner/work/esp32-arduino-lib-builder/esp32-arduino-lib-builder/esp-idf/components/esp_timer/src/esp_timer.c:459
0x40088ca4: vPortTaskWrapper (pxCode=0x400e1ae4 <timer_task>, pvParameters=0x0) at /home/runner/work/esp32-arduino-lib-builder/esp32-arduino-lib-builder/esp-idf/components/freertos/FreeRTOS-Kernel/portable/xtensa/port.c:139

==================== THREAD 6 (TCB: 0x3ffc48f8) ====================
0x40083160: esp_crosscore_int_send_yield (core_id=1) at /home/runner/work/esp32-arduino-lib-builder/esp32-arduino-lib-builder/esp-idf/components/esp_system/crosscore_int.c:121
0x40089604: prvProcessTimerOrBlockTask (xNextExpireTime=0, xListWasEmpty=<optimized out>) at /home/runner/work/esp32-arduino-lib-builder/esp32-arduino-lib-builder/esp-idf/components/xtensa/include/xt_utils.h:41
0x40089604: prvTimerTask (pvParameters=<optimized out>) at /home/runner/work/esp32-arduino-lib-builder/esp32-arduino-lib-builder/esp-idf/components/freertos/FreeRTOS-Kernel/timers.c:685
0x40088ca4: vPortTaskWrapper (pxCode=0x400894f4 <prvTimerTask>, pvParameters=0x0) at /home/runner/work/esp32-arduino-lib-builder/esp32-arduino-lib-builder/esp-idf/components/freertos/FreeRTOS-Kernel/portable/xtensa/port.c:139

==================== THREAD 7 (TCB: 0x3ffb7978) ====================
0x4000bff0: ??
0x40088f11: vPortClearInterruptMaskFromISR () at /home/runner/work/esp32-arduino-lib-builder/esp32-arduino-lib-builder/esp-idf/components/freertos/FreeRTOS-Kernel/portable/xtensa/include/freertos/portmacro.h:560
0x40088f11: vPortExitCritical (mux=0x3ffbdd28 <xKernelLock>) at /home/runner/work/esp32-arduino-lib-builder/esp32-arduino-lib-builder/esp-idf/components/freertos/FreeRTOS-Kernel/portable/xtensa/port.c:514
0x4008ae69: ulTaskGenericNotifyTake (uxIndexToWait=0, xClearCountOnExit=1, xTicksToWait=4294967295) at /home/runner/work/esp32-arduino-lib-builder/esp32-arduino-lib-builder/esp-idf/components/freertos/FreeRTOS-Kernel/tasks.c:5765
0x4008231d: ipc_task (arg=<optimized out>) at /home/runner/work/esp32-arduino-lib-builder/esp32-arduino-lib-builder/esp-idf/components/esp_system/esp_ipc.c:62
0x40088ca4: vPortTaskWrapper (pxCode=0x400822dc <ipc_task>, pvParameters=0x1) at /home/runner/work/esp32-arduino-lib-builder/esp32-arduino-lib-builder/esp-idf/components/freertos/FreeRTOS-Kernel/portable/xtensa/port.c:139
```

</p>
</details>

### Decode with esp-coredump (optional)

Compare the decode output with the official [esp-coredump](https://github.com/espressif/esp-coredump) one:

```sh
pipx run esp-coredump info_corefile \
  --core "" \
  --core-format elf \
  --off 0x3F0000 \
  --save-core coredump.elf \
  $elf_path
```

> **ⓘ** This step requires Python, `pipx`, and `esp-coredump` to be installed.

> You might need to set `ESP-IDF` to the shell. For example, `source ~/esp/v5.4.1/esp-idf/export.sh`.

---

### Acknowledgment

Thanks to [ptillisch](https://forum.arduino.cc/u/ptillisch) for the helpful instructions.
