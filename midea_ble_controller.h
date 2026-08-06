#pragma once
#include <string>
#include <cstring>
#include <cstdint>
#include <vector>
#include <functional>

// ================================================================
//                    美的 BLE 协议 - 指令生成
// ================================================================

// 计算校验和
static uint8_t midea_checksum(const std::vector<uint8_t>& data) {
    uint8_t sum = 0;
    for (size_t i = 0; i < data.size(); i++) sum ^= data[i];
    return sum;
}

// 将 MAC 字符串转为字节数组
static void mac_str_to_bytes(const std::string& mac_str, uint8_t* out) {
    for (int i = 0; i < 6; i++) {
        std::string byte_str = mac_str.substr(i * 2, 2);
        out[i] = (uint8_t)strtol(byte_str.c_str(), nullptr, 16);
    }
}

// 将字节数组转为 MAC 字符串
static std::string mac_bytes_to_str(const uint8_t* mac) {
    char buf[13];
    snprintf(buf, sizeof(buf), "%02X%02X%02X%02X%02X%02X",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    return std::string(buf);
}

// 亮度转换: 0-100% → 0-255
static uint8_t percent_to_raw(int percent) {
    if (percent < 1) percent = 1;
    if (percent > 100) percent = 100;
    return (uint8_t)(percent * 255.0f / 100.0f);
}

// 亮度转换: 0-255 → 0-100%
static int raw_to_percent(uint8_t raw) {
    int pct = (int)(raw / 255.0f * 100.0f);
    if (pct < 0) pct = 0;
    if (pct > 100) pct = 100;
    return pct;
}

// 色温转换: 0-255 → 2700K-6500K
static int raw_to_kelvin(uint8_t raw) {
    int pct = (int)(raw / 255.0f * 100.0f);
    if (pct < 0) pct = 0;
    if (pct > 100) pct = 100;
    return 2700 + (6500 - 2700) * pct / 100;
}

// 色温转换: 2700K-6500K → 0-255
static uint8_t kelvin_to_raw(int kelvin) {
    if (kelvin < 2700) kelvin = 2700;
    if (kelvin > 6500) kelvin = 6500;
    int pct = (kelvin - 2700) * 100 / (6500 - 2700);
    return (uint8_t)(pct * 255.0f / 100.0f);
}

// --- 美的 BLE 指令生成函数 ---

static std::string midea_light_on(const std::string& mac_str, int brightness, int color_temp) {
    uint8_t mac[6];
    mac_str_to_bytes(mac_str, mac);
    uint8_t brt_raw = percent_to_raw(brightness);
    uint8_t ct_raw = kelvin_to_raw(color_temp);
    
    std::vector<uint8_t> cmd = {
        0x81, 0x53, 0x01,
        mac[0], mac[1], mac[2], mac[3], mac[4], mac[5],
        0x08, 0x00,
        0x13,  // mode: 风扇正转 + LED开
        0x01, 0x00,
        brt_raw, ct_raw,
        0x00, 0x00,  // timer
        0x00,        // fan speed (保持)
        0x00         // checksum placeholder
    };
    cmd[cmd.size() - 1] = midea_checksum(cmd);
    
    std::string hex;
    for (uint8_t b : cmd) {
        char buf[3];
        snprintf(buf, sizeof(buf), "%02X", b);
        hex += buf;
    }
    return hex;
}

static std::string midea_light_off(const std::string& mac_str) {
    uint8_t mac[6];
    mac_str_to_bytes(mac_str, mac);
    
    std::vector<uint8_t> cmd = {
        0x81, 0x53, 0x01,
        mac[0], mac[1], mac[2], mac[3], mac[4], mac[5],
        0x08, 0x00,
        0x12,  // mode: 风扇正转 + LED关
        0x01, 0x00,
        0x00, 0x00,  // brightness, ct
        0x00, 0x00,  // timer
        0x00,        // fan speed
        0x00         // checksum
    };
    cmd[cmd.size() - 1] = midea_checksum(cmd);
    
    std::string hex;
    for (uint8_t b : cmd) {
        char buf[3];
        snprintf(buf, sizeof(buf), "%02X", b);
        hex += buf;
    }
    return hex;
}

static std::string midea_light_brightness(const std::string& mac_str, int brightness, int color_temp) {
    return midea_light_on(mac_str, brightness, color_temp);
}

static std::string midea_light_color_temp(const std::string& mac_str, int kelvin) {
    uint8_t mac[6];
    mac_str_to_bytes(mac_str, mac);
    uint8_t ct_raw = kelvin_to_raw(kelvin);
    
    std::vector<uint8_t> cmd = {
        0x81, 0x53, 0x01,
        mac[0], mac[1], mac[2], mac[3], mac[4], mac[5],
        0x08, 0x00,
        0x13,
        0x01, 0x00,
        0xFF, ct_raw,  // brightness保持最大, 色温改变
        0x00, 0x00,
        0x00,
        0x00
    };
    cmd[cmd.size() - 1] = midea_checksum(cmd);
    
    std::string hex;
    for (uint8_t b : cmd) {
        char buf[3];
        snprintf(buf, sizeof(buf), "%02X", b);
        hex += buf;
    }
    return hex;
}

static std::string midea_fan_on(const std::string& mac_str, int speed) {
    uint8_t mac[6];
    mac_str_to_bytes(mac_str, mac);
    if (speed < 1) speed = 1;
    if (speed > 6) speed = 6;
    uint8_t spd_raw = (uint8_t)(speed - 1);
    
    std::vector<uint8_t> cmd = {
        0x81, 0x53, 0x01,
        mac[0], mac[1], mac[2], mac[3], mac[4], mac[5],
        0x08, 0x00,
        0x13,  // mode: 风扇正转 + LED开
        0x01, 0x00,
        0xFF, 0x00,  // brightness, ct
        0x00, 0x00,  // timer
        spd_raw,     // fan speed
        0x00         // checksum
    };
    cmd[cmd.size() - 1] = midea_checksum(cmd);
    
    std::string hex;
    for (uint8_t b : cmd) {
        char buf[3];
        snprintf(buf, sizeof(buf), "%02X", b);
        hex += buf;
    }
    return hex;
}

static std::string midea_fan_off(const std::string& mac_str) {
    uint8_t mac[6];
    mac_str_to_bytes(mac_str, mac);
    
    std::vector<uint8_t> cmd = {
        0x81, 0x53, 0x01,
        mac[0], mac[1], mac[2], mac[3], mac[4], mac[5],
        0x08, 0x00,
        0x11,  // mode: 风扇关 + LED开
        0x01, 0x00,
        0xFF, 0x00,
        0x00, 0x00,
        0x00,
        0x00
    };
    cmd[cmd.size() - 1] = midea_checksum(cmd);
    
    std::string hex;
    for (uint8_t b : cmd) {
        char buf[3];
        snprintf(buf, sizeof(buf), "%02X", b);
        hex += buf;
    }
    return hex;
}


// ================================================================
//                    设备模型 (Device Model)
// ================================================================

#define DEVICE_COUNT 4

struct MideaDevice {
    // === 身份标识 ===
    const char* name;
    uint8_t mac[6];
    const char* light_topic;   // 巴法云灯主题
    const char* fan_topic;     // 巴法云风扇主题
    
    // === 灯状态 ===
    bool light_on = false;
    int brightness = 50;       // 1-100
    int color_temp = 4600;     // 2700-6500
    
    // === 风扇状态 ===
    bool fan_on = false;
    int fan_speed = 1;         // 1-6
    
    // === 内部计时 ===
    uint32_t last_publish_ms = 0;
    uint32_t last_change_ms = 0;
    uint8_t retry_count = 0;
    
    // === 方法 ===
    std::string mac_str() const {
        return mac_bytes_to_str(mac);
    }
    
    // 更新灯状态，返回是否有变化
    bool update_light(bool on, int brt, int ct) {
        bool changed = false;
        if (light_on != on) { light_on = on; changed = true; }
        if (brightness != brt) { brightness = brt; changed = true; }
        if (color_temp != ct) { color_temp = ct; changed = true; }
        if (changed) last_change_ms = millis();
        return changed;
    }
    
    // 更新风扇状态，返回是否有变化
    bool update_fan(bool on, int spd) {
        bool changed = false;
        if (fan_on != on) { fan_on = on; changed = true; }
        if (fan_speed != spd) { fan_speed = spd; changed = true; }
        if (changed) last_change_ms = millis();
        return changed;
    }
    
    // 生成灯的 BLE 开灯指令
    std::string cmd_light_on() const {
        return midea_light_on(mac_str(), brightness, color_temp);
    }
    
    // 生成灯的 BLE 关灯指令
    std::string cmd_light_off() const {
        return midea_light_off(mac_str());
    }
    
    // 生成风扇的 BLE 开启指令
    std::string cmd_fan_on() const {
        return midea_fan_on(mac_str(), fan_speed);
    }
    
    // 生成风扇的 BLE 关闭指令
    std::string cmd_fan_off() const {
        return midea_fan_off(mac_str());
    }
    
    // 生成亮度调节指令
    std::string cmd_brightness(int brt) const {
        return midea_light_brightness(mac_str(), brt, color_temp);
    }
    
    // 生成色温调节指令
    std::string cmd_color_temp(int ct) const {
        return midea_light_color_temp(mac_str(), ct);
    }
};

// === 全局设备数组 ===
static MideaDevice g_devices[DEVICE_COUNT] = {
    // 餐厅
    {
        .name = "Dining",
        .mac = {0x48, 0x80, 0x27, 0x70, 0x00, 0x00},
        .light_topic = "ctlight002",
        .fan_topic = "ctfan003",
        .light_on = false, .brightness = 100, .color_temp = 5588,
        .fan_on = false, .fan_speed = 1
    },
    // 次卧
    {
        .name = "Bedroom",
        .mac = {0xF0, 0xCF, 0x2D, 0x70, 0x00, 0x00},
        .light_topic = "cwlight002",
        .fan_topic = "cwfan003",
        .light_on = false, .brightness = 100, .color_temp = 4600,
        .fan_on = false, .fan_speed = 1
    },
    // 主卧
    {
        .name = "Master",
        .mac = {0x92, 0x26, 0x35, 0x70, 0x00, 0x00},
        .light_topic = "zzwlight002",
        .fan_topic = "zzwfan003",
        .light_on = false, .brightness = 50, .color_temp = 4600,
        .fan_on = false, .fan_speed = 1
    },
    // 客厅
    {
        .name = "Living Room",
        .mac = {0xCD, 0xAB, 0x38, 0x70, 0x00, 0x00},
        .light_topic = "kktlight002",
        .fan_topic = "kktfan003",
        .light_on = false, .brightness = 50, .color_temp = 4600,
        .fan_on = false, .fan_speed = 1
    }
};


// ================================================================
//                    BLE 发送队列
// ================================================================

struct BleQueueItem {
    std::string hex_data;
    uint32_t send_at_ms;
};

static std::vector<BleQueueItem> g_ble_queue;
static std::function<void(const std::string&)> g_ble_send_fn = nullptr;

// 设置 BLE 发送函数 (YAML 初始化时调用)
static void ble_set_send_function(std::function<void(const std::string&)> fn) {
    g_ble_send_fn = fn;
}

// 添加指令到队列 (delay_ms: 延迟多少毫秒后发送)
static void ble_queue_add(const std::string& hex, uint32_t delay_ms = 0) {
    BleQueueItem item;
    item.hex_data = hex;
    item.send_at_ms = millis() + delay_ms;
    g_ble_queue.push_back(item);
}

// 处理队列 (在 interval 中定时调用)
static void ble_queue_process() {
    if (g_ble_queue.empty()) return;
    if (!g_ble_send_fn) return;
    
    uint32_t now = millis();
    if (now >= g_ble_queue[0].send_at_ms) {
        g_ble_send_fn(g_ble_queue[0].hex_data);
        g_ble_queue.erase(g_ble_queue.begin());
    }
}

// 发送设备灯指令 (自动入队)
static void device_send_light(MideaDevice& dev) {
    if (dev.light_on) {
        ble_queue_add(dev.cmd_light_on(), 0);
    } else {
        ble_queue_add(dev.cmd_light_off(), 0);
    }
}

// 发送设备风扇指令 (自动入队)
static void device_send_fan(MideaDevice& dev) {
    if (dev.fan_on) {
        ble_queue_add(dev.cmd_fan_on(), 100);  // 延迟100ms，避免和灯指令冲突
    } else {
        ble_queue_add(dev.cmd_fan_off(), 100);
    }
}


// ================================================================
//                    MQTT 发布 (巴法云)
// ================================================================

static std::function<void(const char*, const char*)> g_mqtt_publish_fn = nullptr;

// 设置 MQTT 发布函数 (YAML 初始化时调用)
static void mqtt_set_publish_function(std::function<void(const char*, const char*)> fn) {
    g_mqtt_publish_fn = fn;
}

// 将单个设备状态发布到巴法云
static void device_publish_bemfa(MideaDevice& dev) {
    if (!g_mqtt_publish_fn) return;
    
    // 节流: 200ms 内不重复发布
    uint32_t now = millis();
    if (now - dev.last_publish_ms < 200) return;
    dev.last_publish_ms = now;
    
    // 灯状态
    std::string light_topic = std::string(dev.light_topic) + "/up";
    if (dev.light_on) {
        std::string payload = "on#" + std::to_string(dev.brightness) + "#" + std::to_string(dev.color_temp);
        g_mqtt_publish_fn(light_topic.c_str(), payload.c_str());
    } else {
        g_mqtt_publish_fn(light_topic.c_str(), "off");
    }
    
    // 风扇状态
    std::string fan_topic = std::string(dev.fan_topic) + "/up";
    if (dev.fan_on) {
        std::string payload = "on#" + std::to_string(dev.fan_speed);
        g_mqtt_publish_fn(fan_topic.c_str(), payload.c_str());
    } else {
        g_mqtt_publish_fn(fan_topic.c_str(), "off");
    }
}

// 发布所有设备状态 (用于 MQTT 重连同步)
static void publish_all_devices() {
    for (int i = 0; i < DEVICE_COUNT; i++) {
        g_devices[i].last_publish_ms = 0;  // 清除节流
        device_publish_bemfa(g_devices[i]);
    }
}


// ================================================================
//                    BLE 广播解析
// ================================================================

// 解析单个设备的 BLE 广播数据
// 返回: 找到的设备索引，-1 表示未匹配
static int parse_ble_device(const uint8_t* raw, size_t raw_size, int dev_idx) {
    MideaDevice& dev = g_devices[dev_idx];
    
    // 在广播数据中搜索 MAC 地址
    for (int i = 2; i <= (int)raw_size - 6; i++) {
        if (memcmp(&raw[i], dev.mac, 6) != 0) continue;
        
        int remaining = (int)raw_size - i;
        if (remaining < 16) return -1;
        
        // 解析状态
        bool light_on = false;
        int brt = 0;
        int ct_k = 2700;
        bool fan_run = false;
        int fan_spd = 0;
        
        // 尝试标准美的协议 (mode 字段)
        uint8_t mode = raw[i + 8];
        bool alt_valid = (mode == 0x10 || mode == 0x11 ||
                          mode == 0x12 || mode == 0x13 ||
                          mode == 0x20 || mode == 0x21 ||
                          mode == 0x32 || mode == 0x33);
        
        if (alt_valid) {
            light_on = (mode == 0x11 || mode == 0x13 || mode == 0x21 || mode == 0x33);
            fan_run = (mode == 0x12 || mode == 0x13 || mode == 0x20 || mode == 0x21 || mode == 0x32 || mode == 0x33);
            brt = raw_to_percent(raw[i + 11]);
            ct_k = raw_to_kelvin(raw[i + 12]);
            fan_spd = fan_run ? (raw[i + 15] + 1) : 0;
        } else {
            // 兜底: 原始偏移量解析
            if (remaining >= 8) light_on = (raw[i + 7] == 0x01);
            if (remaining >= 13) brt = raw_to_percent(raw[i + 12]);
            if (remaining >= 14) ct_k = raw_to_kelvin(raw[i + 13]);
            if (remaining >= 17) {
                uint8_t fs = raw[i + 16];
                fan_run = (fs & 0x01) != 0;
            }
            if (remaining >= 18 && fan_run) fan_spd = raw[i + 17] + 1;
        }
        
        // 风速修正
        if (fan_run) {
            if (fan_spd < 1 || fan_spd > 6) {
                int fixed = 0;
                if (remaining >= 16) {
                    int s15 = raw[i + 15] + 1;
                    if (s15 >= 1 && s15 <= 6) fixed = s15;
                }
                if (fixed == 0 && remaining >= 18) {
                    int s17 = raw[i + 17] + 1;
                    if (s17 >= 1 && s17 <= 6) fixed = s17;
                }
                fan_spd = (fixed >= 1 && fixed <= 6) ? fixed : 1;
            }
        } else {
            fan_spd = 0;
        }
        
        // 更新设备模型
        bool light_changed = dev.update_light(light_on, brt, ct_k);
        bool fan_changed = dev.update_fan(fan_run, fan_spd);
        
        // 如果有变化，发布到巴法云
        if (light_changed || fan_changed) {
            device_publish_bemfa(dev);
        }
        
        return dev_idx;
    }
    return -1;
}

// 解析 BLE 广播 (在 esp32_ble_tracker 的 lambda 中调用)
static void parse_ble_advertisement(const std::vector<uint8_t>& raw) {
    if (raw.size() < 20) return;
    
    for (int dev_idx = 0; dev_idx < DEVICE_COUNT; dev_idx++) {
        int found = parse_ble_device(raw.data(), raw.size(), dev_idx);
        if (found >= 0) return;  // 匹配到一个设备就返回
    }
}


// ================================================================
//                    MQTT 指令处理 (巴法云下行)
// ================================================================

// 处理灯的 MQTT 指令
static void handle_light_mqtt(int dev_idx, const std::string& payload) {
    if (dev_idx < 0 || dev_idx >= DEVICE_COUNT) return;
    MideaDevice& dev = g_devices[dev_idx];
    
    if (payload == "on") {
        dev.light_on = true;
        if (dev.brightness < 1) dev.brightness = 50;
        ble_queue_add(dev.cmd_light_on(), 0);
    } else if (payload == "off") {
        dev.light_on = false;
        ble_queue_add(dev.cmd_light_off(), 0);
    } else if (payload.find("on#") == 0) {
        std::string rest = payload.substr(3);
        size_t pos = rest.find('#');
        int brt = std::stoi(rest.substr(0, pos));
        int ct = dev.color_temp;
        if (pos != std::string::npos) {
            std::string ct_str = rest.substr(pos + 1);
            if (!ct_str.empty()) {
                int val = std::stoi(ct_str);
                if (val >= 2700 && val <= 6500) ct = val;
            }
        }
        dev.light_on = true;
        dev.brightness = brt;
        dev.color_temp = ct;
        ble_queue_add(dev.cmd_light_on(), 0);
    }
    
    device_publish_bemfa(dev);
}

// 处理风扇的 MQTT 指令
static void handle_fan_mqtt(int dev_idx, const std::string& payload) {
    if (dev_idx < 0 || dev_idx >= DEVICE_COUNT) return;
    MideaDevice& dev = g_devices[dev_idx];
    
    if (payload == "on") {
        dev.fan_on = true;
        if (dev.fan_speed < 1) dev.fan_speed = 1;
        ble_queue_add(dev.cmd_fan_on(), 0);
    } else if (payload == "off") {
        dev.fan_on = false;
        ble_queue_add(dev.cmd_fan_off(), 0);
    } else if (payload.find("on#") == 0) {
        int spd = std::stoi(payload.substr(3));
        if (spd < 1) spd = 1;
        if (spd > 6) spd = 6;
        dev.fan_on = true;
        dev.fan_speed = spd;
        ble_queue_add(dev.cmd_fan_on(), 0);
    }
    
    device_publish_bemfa(dev);
}


// ================================================================
//                    定时任务 (心跳 + 队列处理)
// ================================================================

// 在 interval 中调用: 处理 BLE 发送队列
static void tick_ble_queue() {
    ble_queue_process();
}

// 在 interval 中调用: 心跳上报 (每60秒)
static void tick_heartbeat() {
    static uint32_t last_heartbeat = 0;
    uint32_t now = millis();
    if (now - last_heartbeat < 60000) return;
    last_heartbeat = now;
    
    for (int i = 0; i < DEVICE_COUNT; i++) {
        g_devices[i].last_publish_ms = 0;  // 清除节流
        device_publish_bemfa(g_devices[i]);
    }
}
