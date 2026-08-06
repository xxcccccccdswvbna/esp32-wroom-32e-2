#!/usr/bin/env python3
"""
美的风扇灯 ESPHome 配置生成器
用法：python generate.py
输出：ble_gateway.yaml（可直接编译）
"""
import json
import os

def load_devices():
    with open("devices.json", "r", encoding="utf-8") as f:
        return json.load(f)

def gen_header(config):
    return f"""# ===================== 项目配置 =====================
substitutions:
  project_name: "{config['project_name']}"
  build_time: "Unknown"

esphome:
  name: ble-2
  friendly_name: "${{project_name}}"
  comment: "${{project_name}} - Build: ${{build_time}}"
  includes:
    - midea_ble_controller.h

esp32:
  board: esp32dev
  flash_size: 4MB
  framework:
    type: esp-idf
    sdkconfig_options:
      CONFIG_FREERTOS_UNICORE: y
      CONFIG_BT_ENABLED: y
      CONFIG_BT_BLE_ENABLED: y

logger:
  baud_rate: 0
  level: INFO

wifi:
  ssid: "{config['wifi_ssid']}"
  password: "{config['wifi_password']}"
  fast_connect: true
  power_save_mode: none
  ap:
    ssid: "BLE-GW Fallback"
    password: "12345678"

captive_portal:
web_server:
  port: 80
ota:
  - platform: esphome
api:
  reboot_timeout: 0s
  services:
    - service: send_raw_hex
      variables:
        hex_data: string
      then:
        - lambda: 'id(tx).send_hex(hex_data);'

external_components:
  - source:
      type: local
      path: components

ble_tx:
  id: tx

esp32_ble:
  io_capability: none
  enable_on_boot: true

bluetooth_proxy:
  active: true
  cache_services: true
"""

def gen_globals(devices):
    """生成色温记忆全局变量"""
    lines = ["globals:"]
    for d in devices:
        lines.append(f"  - id: {d['name'].lower()}_ct_kelvin")
        lines.append("    type: int")
        lines.append("    restore_value: no")
        lines.append("    initial_value: '4600'")
    return "\n".join(lines)

def gen_mqtt(config, devices):
    """生成巴法云 MQTT 配置"""
    lines = [
        "mqtt:",
        "  id: bemfa",
        "  broker: bemfa.com",
        "  port: 9501",
        f"  username: \"{config['bemfa_uid']}\"",
        f"  password: \"{config['bemfa_uid']}\"",
        f"  client_id: \"{config['bemfa_uid']}\"",
        "  keepalive: 30s",
        "  discovery: false",
        "  birth_message:",
        "  will_message:",
        "  on_message:",
    ]
    
    for d in devices:
        name = d['name'].lower()
        mac = d['mac']
        lt = d['bemfa_light_topic']
        ft = d['bemfa_fan_topic']
        
        # 灯光 MQTT
        lines += [
            f"    - topic: \"{lt}\"",
            "      then:",
            "        - lambda: |-",
            f"            if(x == \"on\") {{",
            f"              int ct = id({name}_ct_kelvin);",
            f"              int brt = id({name}_brt).state;",
            f"              if(brt < 1) brt = 50;",
            f"              id(tx).send_hex(midea_light_on(\"{mac}\", brt, ct));",
            f"              id({name}_light_on).publish_state(true);",
            f"            }} else if(x == \"off\") {{",
            f"              id(tx).send_hex(midea_light_off(\"{mac}\"));",
            f"              id({name}_light_on).publish_state(false);",
            f"            }} else if(x.find(\"on#\") == 0) {{",
            f"              std::string rest = x.substr(3);",
            f"              size_t pos = rest.find('#');",
            f"              int brt = std::stoi(rest.substr(0, pos));",
            f"              int ct = id({name}_ct_kelvin);",
            f"              if(pos != std::string::npos) {{",
            f"                std::string ct_str = rest.substr(pos + 1);",
            f"                if(!ct_str.empty()) {{",
            f"                  int pct = std::stoi(ct_str);",
            f"                  if(pct >= 2700 && pct <= 6500) {{ ct = pct; id({name}_ct_kelvin) = ct; }}",
            f"                }}",
            f"              }}",
            f"              id(tx).send_hex(midea_light_on(\"{mac}\", brt, ct));",
            f"              id({name}_light_on).publish_state(true);",
            f"            }}",
        ]
        
        # 风扇 MQTT
        lines += [
            f"    - topic: \"{ft}\"",
            "      then:",
            "        - lambda: |-",
            f"            if(x == \"on\") {{",
            f"              int spd = id({name}_spd).state;",
            f"              if(spd < 1) spd = 1;",
            f"              id(tx).send_hex(midea_fan_on(\"{mac}\", spd));",
            f"              id({name}_fan_on).publish_state(true);",
            f"            }} else if(x == \"off\") {{",
            f"              id(tx).send_hex(midea_fan_off(\"{mac}\"));",
            f"              id({name}_fan_on).publish_state(false);",
            f"            }} else if(x.find(\"on#\") == 0) {{",
            f"              int s = std::stoi(x.substr(3));",
            f"              id(tx).send_hex(midea_fan_on(\"{mac}\", s));",
            f"              id({name}_fan_on).publish_state(true);",
            f"            }}",
        ]
    
    return "\n".join(lines)

def gen_ble_tracker(devices):
    """生成 BLE 扫描回调"""
    lines = [
        "esp32_ble_tracker:",
        "  scan_parameters:",
        "    interval: 640ms",
        "    window: 30ms",
        "    active: false",
        "  on_ble_advertise:",
        "    - then:",
        "        - lambda: |-",
        "            auto datas = x.get_manufacturer_datas();",
        "            for (auto &data : datas) {",
        "              auto &raw = data.data;",
        "              if (raw.size() < 20) continue;",
    ]
    
    for d in devices:
        name = d['name'].lower()
        mac = d['mac']
        # MAC 字节数组
        mac_bytes = ", ".join([f"0x{mac[i:i+2]}" for i in range(0, 12, 2)])
        
        lines += [
            f"              uint8_t {name}_mac[6] = {{{mac_bytes}}};",
            f"              for (int i = 2; i <= (int)raw.size() - 6; i++) {{",
            f"                if (memcmp(&raw[i], {name}_mac, 6) == 0) {{",
            f"                  bool light_on = (raw[i+7] == 0x01);",
            f"                  int brt = (int)(raw[i+12] / 255.0f * 100.0f);",
            f"                  int ct_k = 2700 + (6500-2700) * (int)(raw[i+13] / 255.0f * 100.0f) / 100;",
            f"                  uint8_t fs = raw[i+16];",
            f"                  bool fan_run = (fs & 0x01) != 0;",
            f"                  int fan_spd = fan_run ? (raw[i+17] + 1) : 0;",
            f"                  static bool {name}_ll=false, {name}_lfr=false;",
            f"                  static int {name}_lb=-1, {name}_lc=-1, {name}_lfs=-1;",
            f"                  static uint32_t {name}_lr=0, {name}_lch=0;",
            f"                  static uint8_t {name}_rc=0;",
            f"                  bool ch=(light_on!={name}_ll)||(brt!={name}_lb)||(ct_k!={name}_lc)||(fan_run!={name}_lfr)||(fan_spd!={name}_lfs);",
            f"                  if(ch){{{name}_lch=millis();{name}_rc=0;{name}_ll=light_on;{name}_lb=brt;{name}_lc=ct_k;{name}_lfr=fan_run;{name}_lfs=fan_spd;}}",
            f"                  bool hb=(millis()-{name}_lr>60000);",
            f"                  bool rt=(millis()-{name}_lch<10000)&&({name}_rc<3)&&(millis()-{name}_lr>3000);",
            f"                  if(ch||hb||rt){{",
            f"                    if(rt){name}_rc++;else {name}_rc=0; {name}_lr=millis();",
            f"                    if(light_on) id(bemfa).publish(\"{d['bemfa_light_topic']}/up\",\"on#\"+std::to_string(brt)+\"#\"+std::to_string(ct_k));",
            f"                    else id(bemfa).publish(\"{d['bemfa_light_topic']}/up\",\"off\");",
            f"                    if(fan_run) id(bemfa).publish(\"{d['bemfa_fan_topic']}/up\",\"on#\"+std::to_string(fan_spd));",
            f"                    else id(bemfa).publish(\"{d['bemfa_fan_topic']}/up\",\"off\");",
            f"                    id({name}_light_on).publish_state(light_on);",
            f"                    id({name}_brt).publish_state(brt);",
            f"                    id({name}_ct).publish_state(ct_k);",
            f"                    id({name}_fan_on).publish_state(fan_run);",
            f"                    id({name}_spd).publish_state(fan_spd);",
            f"                    id({name}_light_sw).publish_state(light_on);",
            f"                    id({name}_fan_sw).publish_state(fan_run);",
            f"                    id({name}_brt_n).publish_state(brt);",
            f"                    id({name}_ct_n).publish_state(ct_k);",
            f"                    id({name}_spd_n).publish_state(fan_spd);",
            f"                  }}",
            f"                  break;",
            f"                }}",
            f"              }}",
        ]
    
    lines.append("            }")
    return "\n".join(lines)

def gen_controls(devices):
    """生成 switch + number + sensor"""
    switch_lines = ["switch:"]
    number_lines = ["number:"]
    sensor_lines = ["sensor:"]
    binary_sensor_lines = ["binary_sensor:"]
    
    for d in devices:
        name = d['name'].lower()
        mac = d['mac']
        
        # 灯开关
        switch_lines += [
            f"  - platform: template",
            f"    name: \"{d['name']} Light\"",
            f"    id: {name}_light_sw",
            f"    icon: mdi:ceiling-light",
            f"    optimistic: false",
            f"    restore_mode: RESTORE_DEFAULT_OFF",
            f"    turn_on_action:",
            f"      - lambda: |-",
            f"          int ct = id({name}_ct_kelvin);",
            f"          int brt = id({name}_brt).state;",
            f"          if(brt < 1) brt = 50;",
            f"          id(tx).send_hex(midea_light_on(\"{mac}\", brt, ct));",
            f"          id({name}_light_on).publish_state(true);",
            f"    turn_off_action:",
            f"      - lambda: |-",
            f"          id(tx).send_hex(midea_light_off(\"{mac}\"));",
            f"          id({name}_light_on).publish_state(false);",
            f"    lambda: 'return id({name}_light_on).state;'",
        ]
        
        # 风扇开关
        switch_lines += [
            f"  - platform: template",
            f"    name: \"{d['name']} Fan\"",
            f"    id: {name}_fan_sw",
            f"    icon: mdi:fan",
            f"    optimistic: false",
            f"    restore_mode: RESTORE_DEFAULT_OFF",
            f"    turn_on_action:",
            f"      - lambda: |-",
            f"          int spd = id({name}_spd).state;",
            f"          if(spd < 1) spd = 1;",
            f"          id(tx).send_hex(midea_fan_on(\"{mac}\", spd));",
            f"          id({name}_fan_on).publish_state(true);",
            f"    turn_off_action:",
            f"      - lambda: |-",
            f"          id(tx).send_hex(midea_fan_off(\"{mac}\"));",
            f"          id({name}_fan_on).publish_state(false);",
            f"    lambda: 'return id({name}_fan_on).state;'",
        ]
        
        # 亮度滑块
        number_lines += [
            f"  - platform: template",
            f"    name: \"{d['name']} Brightness\"",
            f"    id: {name}_brt_n",
            f"    min_value: 1; max_value: 100; step: 1",
            f"    unit_of_measurement: \"%\"",
            f"    icon: mdi:brightness-percent",
            f"    set_action:",
            f"      - lambda: |-",
            f"          int brt = (int)x;",
            f"          int ct = id({name}_ct_kelvin);",
            f"          id(tx).send_hex(midea_light_brightness(\"{mac}\", brt, ct));",
            f"          id({name}_brt).publish_state(brt);",
            f"          id({name}_light_on).publish_state(true);",
            f"          id({name}_light_sw).publish_state(true);",
        ]
        
        # 色温滑块
        number_lines += [
            f"  - platform: template",
            f"    name: \"{d['name']} ColorTemp\"",
            f"    id: {name}_ct_n",
            f"    min_value: 2700; max_value: 6500; step: 100",
            f"    unit_of_measurement: \"K\"",
            f"    icon: mdi:thermometer",
            f"    set_action:",
            f"      - lambda: |-",
            f"          int kelvin = (int)x;",
            f"          id({name}_ct_kelvin) = kelvin;",
            f"          id(tx).send_hex(midea_light_color_temp(\"{mac}\", kelvin));",
            f"          id({name}_ct).publish_state(kelvin);",
        ]
        
        # 风扇档位
        number_lines += [
            f"  - platform: template",
            f"    name: \"{d['name']} Fan Speed\"",
            f"    id: {name}_spd_n",
            f"    min_value: 1; max_value: 6; step: 1",
            f"    icon: mdi:fan-speed-1",
            f"    set_action:",
            f"      - lambda: |-",
            f"          int spd = (int)x;",
            f"          id(tx).send_hex(midea_fan_on(\"{mac}\", spd));",
            f"          id({name}_spd).publish_state(spd);",
            f"          id({name}_fan_on).publish_state(true);",
            f"          id({name}_fan_sw).publish_state(true);",
        ]
        
        # 传感器
        sensor_lines += [
            f"  - platform: template; name: \"{d['name']} Brt\"; id: {name}_brt; unit_of_measurement: \"%\"; accuracy_decimals: 0",
            f"  - platform: template; name: \"{d['name']} CT\"; id: {name}_ct; unit_of_measurement: \"K\"",
            f"  - platform: template; name: \"{d['name']} Spd\"; id: {name}_spd",
        ]
        
        binary_sensor_lines += [
            f"  - platform: template; name: \"{d['name']} Light On\"; id: {name}_light_on; device_class: light",
            f"  - platform: template; name: \"{d['name']} Fan On\"; id: {name}_fan_on; device_class: running",
        ]
    
    # 公共传感器
    sensor_lines += [
        "  - platform: uptime; name: \"Uptime\"; update_interval: 60s",
        "  - platform: internal_temperature; name: \"ESP32 Temp\"; unit_of_measurement: \"°C\"; accuracy_decimals: 1; update_interval: 60s",
        "  - platform: wifi_signal; name: \"WiFi dBm\"; id: wifi_signal_db; update_interval: 60s",
        "  - platform: copy; source_id: wifi_signal_db; name: \"WiFi Pct\"",
        "    filters:",
        "      - lambda: return min(max(2*(x+100.0),0.0),100.0);",
        "    unit_of_measurement: \"%\"; accuracy_decimals: 0",
    ]
    binary_sensor_lines += [
        "  - platform: status; name: \"Gateway Online\"; device_class: connectivity",
    ]
    
    return "\n".join(switch_lines) + "\n\n" + "\n".join(number_lines) + "\n\n" + "\n".join(sensor_lines) + "\n\n" + "\n".join(binary_sensor_lines)

def gen_footer():
    return """
text_sensor:
  - platform: template
    name: "Build Time"
    lambda: 'return {"${build_time}"};'
    update_interval: 3600s
  - platform: version
    name: "ESPHome Ver"
    hide_timestamp: true
  - platform: wifi_info
    ip_address:
      name: "ESP IP"
    ssid:
      name: "ESP SSID"

button:
  - platform: restart
    name: "Restart GW"
    icon: mdi:restart
    entity_category: diagnostic
"""

def main():
    config = load_devices()
    devices = config['devices']
    
    output = gen_header(config)
    output += gen_globals(devices)
    output += gen_mqtt(config, devices)
    output += gen_ble_tracker(devices)
    output += gen_controls(devices)
    output += gen_footer()
    
    with open("ble_gateway.yaml", "w", encoding="utf-8") as f:
        f.write(output)
    
    print(f"✅ 已生成 ble_gateway.yaml（{len(devices)} 个设备）")

if __name__ == "__main__":
    main()
