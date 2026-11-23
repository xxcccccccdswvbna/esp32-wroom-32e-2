# HomLux / Midea fan-light ESPHome upgrade

This is a **minimal project-based update** for the user's uploaded `esp32-wroom-32e-2-zhuamn-1` ESPHome project, using the official `homlux_mini` source as a protocol reference.

## Included changes

- Preserves existing four MAC addresses, HA entities, MQTT/Bemfa topics, room packages, scanner/proxy, web server, API, Wi-Fi and OTA settings.
- Replaces `components/ble_tx/ble_tx.cpp` and `.h` with a single-job-at-a-time sender that:
  - accepts the existing `controlHex|endHex` interface;
  - validates raw packet length (5..31 bytes);
  - uses 80 ms settling delay after raw adv-data config;
  - broadcasts control for 1200 ms, stops, waits 150 ms, broadcasts END for 600 ms, stops, then waits 200 ms before the next command;
  - checks synchronous ESP-IDF BLE API return values, retries twice with backoff, and drains the queue after failures;
  - does not register/replace the ESPHome GAP callback, to avoid disrupting BLE scanner/proxy callbacks.
- Aligns light-on to official `0x06` toggle, brightness to `0x51` plus scaled value, and color temperature to `0x55` plus scaled value (removes the old mixed `0x5B` shortcut from the normal HA light/brightness path).
- Bemfa `on#brightness#kelvin` now queues light toggle, brightness, and color-temperature as three separate official-style commands.
- Adds HA momentary buttons per room for fan-light command codes: natural wind, reverse, display toggle, fan timer cancel, and 1..6 hour timer-off.

## Important limitations / validation status

- This is protocol-aligned for the legacy manufacturer-data packet format (0x4D11 / VBCV 0x19) already used by this project's C++ controller. It is not a complete port of HomLux V2 (0x06A8) or connected BLE GATT behavior.
- The official mini-program's advertising start/stop callbacks cannot be reproduced fully here without replacing ESPHome's GAP callback, which this package deliberately avoids because the device also scans and runs Bluetooth proxy. This implementation uses conservative timed sequencing and synchronous API return checks. Async GAP failures may still be missed.
- ESP32 hardware validation is not performed in this environment. This is an integration candidate, not a claimed verified firmware.
- User `secrets.yaml` is excluded from this ZIP. Copy your existing file into the project folder before compiling.

## Install
1. Back up the active ESPHome project.
2. Extract this archive and copy/merge the files over the active project directory. Do not overwrite your `secrets.yaml` with any example.
3. Compile first, then flash.
4. Test one room at a time: light toggle, fan toggle, 1..6 speed, brightness, color temperature, natural wind, reverse, display toggle, timer cancel and 1-hour timer. Then test consecutive control from web, HA and Bemfa.
5. Check logs for `homlux_ble_tx`: queue, packet length, synchronous start/stop errors.

## Timing

Each paired control command takes roughly 2.15 seconds plus scheduling overhead (control 1200 ms + 150 ms transition + END 600 ms + 200 ms queue gap). The outer command dispatcher is paced at 2300 ms to prevent its 50 ms poll from flooding the sender. This is deliberately slower than the old 200 ms one-shot packet and can increase responsiveness reliability, but rapid bursts will be serialized.
