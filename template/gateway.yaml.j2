substitutions:
  project_name: "{{ gateway.name }}"
  build_time: "Unknown"

esphome:
  name: {{ gateway.name }}
  friendly_name: "${project_name}"
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
  ssid: "{{ gateway.wifi_ssid }}"
  password: "{{ gateway.wifi_password }}"
  fast_connect: true
  power_save_mode: none
  ap:
    ssid: "{{ gateway.name }} Fallback"
    password: "12345678"

captive_portal:
web_server:
  port: 80
ota:
  - platform: esphome
api:
  reboot_timeout: 0s

external_components:
  - source:
      type: local
      path: components

ble_gateway:
  id: gw

esp32_ble:
  io_capability: none
  enable_on_boot: true

bluetooth_proxy:
  active: true
  cache_services: true

globals:
{% for dev in devices %}
  - id: {{ dev.id }}_ct_kelvin
    type: int
    restore_value: no
    initial_value: '{{ dev.defaults.color_temp }}'
{% endfor %}

mqtt:
  id: bemfa
  broker: bemfa.com
  port: 9501
  username: "{{ gateway.bemfa_uid }}"
  password: "{{ gateway.bemfa_uid }}"
  client_id: "{{ gateway.bemfa_uid }}"
  keepalive: 30s
  discovery: false
  birth_message:
  will_message:
  on_message:
{% for dev in devices %}
    - topic: "{{ dev.bemfa.light }}"
      then:
        - lambda: |-
            auto *light = id({{ dev.id }}_light);
            if(x == "on"){
              int ct = id({{ dev.id }}_ct_kelvin);
              float bri = light->current_values.get_brightness();
              int brt = (bri > 0) ? (int)(bri * 100.0f) : {{ dev.defaults.brightness }};
              id(gw).send_hex(midea_light_on("{{ dev.mac }}", brt, ct));
              light->current_values.set_state(true);
              light->current_values.set_brightness(brt / 100.0f);
              light->current_values.set_color_temperature(1000000.0f / ct);
              light->publish_state();
            }else if(x == "off"){
              id(gw).send_hex(midea_light_off("{{ dev.mac }}"));
              light->current_values.set_state(false);
              light->publish_state();
            }else if(x.find("on#") == 0){
              std::string rest = x.substr(3);
              size_t pos = rest.find('#');
              int brt = std::stoi(rest.substr(0, pos));
              int ct = id({{ dev.id }}_ct_kelvin);
              if(pos != std::string::npos){
                std::string ct_str = rest.substr(pos+1);
                if(!ct_str.empty()){
                  int val = std::stoi(ct_str);
                  if(val >= 2700 && val <= 6500){
                    ct = val;
                    id({{ dev.id }}_ct_kelvin) = ct;
                  }
                }
              }
              id(gw).send_hex(midea_light_on("{{ dev.mac }}", brt, ct));
              light->current_values.set_state(true);
              light->current_values.set_brightness(brt / 100.0f);
              light->current_values.set_color_temperature(1000000.0f / ct);
              light->publish_state();
            }
    - topic: "{{ dev.bemfa.fan }}"
      then:
        - lambda: |-
            auto *fan = id({{ dev.id }}_fan);
            if(x == "on"){
              int spd = fan->speed;
              if(spd < 1) spd = {{ dev.defaults.fan_speed }};
              id(gw).send_hex(midea_fan_on("{{ dev.mac }}", spd));
              fan->state = true;
              fan->speed = spd;
              fan->publish_state();
            }else if(x == "off"){
              id(gw).send_hex(midea_fan_off("{{ dev.mac }}"));
              fan->state = false;
              fan->publish_state();
            }else if(x.find("on#") == 0){
              int s = std::stoi(x.substr(3));
              id(gw).send_hex(midea_fan_on("{{ dev.mac }}", s));
              fan->state = true;
              fan->speed = s;
              fan->publish_state();
            }
{% endfor %}

esp32_ble_tracker:
  scan_parameters:
    interval: 640ms
    window: 30ms
    active: false
  on_ble_advertise:
    - then:
        - lambda: |-
            auto datas = x.get_manufacturer_datas();
            for (auto &data : datas) {
              auto &raw = data.data;
              if (raw.size() < 20) continue;
{% for dev in devices %}
              uint8_t {{ dev.id }}_mac[6] = { {{ dev.mac | mac_to_bytes }} };
              for (int i = 2; i <= (int)raw.size() - 6; i++) {
                if (memcmp(&raw[i], {{ dev.id }}_mac, 6) == 0) {
                  bool light_on = (raw[i+7] == 0x01);
                  int brt = (int)(raw[i+12] / 255.0f * 100.0f);
                  int ct_k = 2700 + (6500-2700) * (int)(raw[i+13] / 255.0f * 100.0f) / 100;
                  uint8_t fan_state = raw[i+16];
                  bool fan_run = (fan_state & 0x01) != 0;
                  int fan_spd = fan_run ? (raw[i+17] + 1) : 0;

                  auto *light = id({{ dev.id }}_light);
                  auto *fan = id({{ dev.id }}_fan);
                  bool changed = (light->current_values.is_on() != light_on) ||
                                 (fan->state != fan_run) ||
                                 (fan->speed != fan_spd);
                  if(changed){
                    light->current_values.set_state(light_on);
                    light->current_values.set_brightness(brt / 100.0f);
                    light->current_values.set_color_temperature(1000000.0f / ct_k);
                    light->publish_state();
                    fan->state = fan_run;
                    fan->speed = fan_spd;
                    fan->publish_state();

                    if(light_on)
                      id(bemfa).publish("{{ dev.bemfa.light }}/up", "on#" + std::to_string(brt) + "#" + std::to_string(ct_k));
                    else
                      id(bemfa).publish("{{ dev.bemfa.light }}/up", "off");
                    if(fan_run)
                      id(bemfa).publish("{{ dev.bemfa.fan }}/up", "on#" + std::to_string(fan_spd));
                    else
                      id(bemfa).publish("{{ dev.bemfa.fan }}/up", "off");
                  }
                  break;
                }
              }
{% endfor %}
            }

light:
{% for dev in devices %}
  - platform: template
    name: "{{ dev.name }} Light"
    id: {{ dev.id }}_light
    icon: mdi:ceiling-light
    color_temperature: true
    brightness: true
    cold_white_color_temperature: 6500 K
    warm_white_color_temperature: 2700 K
    restore_mode: RESTORE_DEFAULT_OFF
    turn_on_action:
      - lambda: |-
          auto *light = id({{ dev.id }}_light);
          float bri = light->current_values.get_brightness();
          int brt = (bri > 0) ? (int)(bri * 100.0f) : {{ dev.defaults.brightness }};
          float ct = light->current_values.get_color_temperature();
          int kelvin = (int)(1000000.0f / ct);
          id({{ dev.id }}_ct_kelvin) = kelvin;
          id(gw).send_hex(midea_light_on("{{ dev.mac }}", brt, kelvin));
    turn_off_action:
      - lambda: |-
          id(gw).send_hex(midea_light_off("{{ dev.mac }}"));
    on_brightness:
      then:
        - lambda: |-
            auto *light = id({{ dev.id }}_light);
            int brt = (int)(x * 100.0f);
            int ct = id({{ dev.id }}_ct_kelvin);
            id(gw).send_hex(midea_light_brightness("{{ dev.mac }}", brt, ct));
            light->current_values.set_brightness(x);
            light->publish_state();
    on_color_temperature:
      then:
        - lambda: |-
            auto *light = id({{ dev.id }}_light);
            int kelvin = (int)(1000000.0f / x);
            id({{ dev.id }}_ct_kelvin) = kelvin;
            id(gw).send_hex(midea_light_color_temp("{{ dev.mac }}", kelvin));
            light->current_values.set_color_temperature(x);
            light->publish_state();
{% endfor %}

fan:
{% for dev in devices %}
  - platform: template
    name: "{{ dev.name }} Fan"
    id: {{ dev.id }}_fan
    icon: mdi:fan
    speed_count: 6
    restore_mode: RESTORE_DEFAULT_OFF
    turn_on_action:
      - lambda: |-
          auto *fan = id({{ dev.id }}_fan);
          int spd = fan->speed;
          if(spd < 1) spd = {{ dev.defaults.fan_speed }};
          id(gw).send_hex(midea_fan_on("{{ dev.mac }}", spd));
          fan->state = true;
          fan->speed = spd;
          fan->publish_state();
    turn_off_action:
      - lambda: |-
          auto *fan = id({{ dev.id }}_fan);
          id(gw).send_hex(midea_fan_off("{{ dev.mac }}"));
          fan->state = false;
          fan->publish_state();
    on_speed_set:
      then:
        - lambda: |-
            auto *fan = id({{ dev.id }}_fan);
            id(gw).send_hex(midea_fan_on("{{ dev.mac }}", (int)x));
            fan->speed = (int)x;
            fan->state = true;
            fan->publish_state();
{% endfor %}

select:
{% for dev in devices if dev.timer %}
  - platform: template
    name: "{{ dev.name }} Light Timer"
    id: {{ dev.id }}_light_timer
    icon: mdi:timer
    optimistic: true
    options:
      - "关闭"
      - "1小时"
      - "2小时"
      - "3小时"
      - "4小时"
      - "5小时"
      - "6小时"
    initial_option: "关闭"
    on_value:
      then:
        - lambda: |-
            int mins = 0;
            if (x == "1小时") mins = 60;
            else if (x == "2小时") mins = 120;
            else if (x == "3小时") mins = 180;
            else if (x == "4小时") mins = 240;
            else if (x == "5小时") mins = 300;
            else if (x == "6小时") mins = 360;
            id(gw).send_hex(midea_timer("{{ dev.mac }}", mins));
  - platform: template
    name: "{{ dev.name }} Fan Timer"
    id: {{ dev.id }}_fan_timer
    icon: mdi:fan-clock
    optimistic: true
    options:
      - "关闭"
      - "1小时"
      - "2小时"
      - "3小时"
      - "4小时"
      - "5小时"
      - "6小时"
    initial_option: "关闭"
    on_value:
      then:
        - lambda: |-
            int mins = 0;
            if (x == "1小时") mins = 60;
            else if (x == "2小时") mins = 120;
            else if (x == "3小时") mins = 180;
            else if (x == "4小时") mins = 240;
            else if (x == "5小时") mins = 300;
            else if (x == "6小时") mins = 360;
            id(gw).send_hex(midea_timer("{{ dev.mac }}", mins));
{% endfor %}

sensor:
  - platform: uptime
    name: "Uptime"
    update_interval: 60s
  - platform: internal_temperature
    name: "ESP32 Temp"
    unit_of_measurement: "°C"
    accuracy_decimals: 1
    update_interval: 60s
  - platform: wifi_signal
    name: "WiFi dBm"
    id: wifi_signal_db
    update_interval: 60s
  - platform: copy
    source_id: wifi_signal_db
    name: "WiFi Pct"
    filters:
      - lambda: return min(max(2*(x+100.0),0.0),100.0);
    unit_of_measurement: "%"
    accuracy_decimals: 0

binary_sensor:
  - platform: status
    name: "Gateway Online"
    device_class: connectivity

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
