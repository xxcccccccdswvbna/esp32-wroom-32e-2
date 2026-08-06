#pragma once
#include <vector>
#include <string>
#include <cmath>
#include <cstring>
#include <cstdint>
#include <cstdio>
#include <algorithm>
#include <functional>
#include <esphome/core/hal.h>  // 【修复】替换 Arduino.h，使用 ESPHome 的 HAL 层

// ================================================================
//                    1. 美的 BLE 协议 - 指令生成 (核心协议)
// ================================================================

// ========== 加密表生成 ==========
inline std::vector<uint8_t> midea_encode_table(const std::string &mac) {
    uint8_t hx[6];
    for(int i=0; i<6; i++) 
        hx[i] = std::stoi(mac.substr(i*2, 2), nullptr, 16);
    
    std::vector<uint8_t> tb(16);
    int l=0, r=1;
    for(int i=0; i<15; i++) {
        tb[i] = (hx[l] + hx[r]) & 0xFF;
        r++;
        if(r==6) { l++; r=l+1; }
    }
    tb[15] = 0;
    for(int i=0; i<6; i++) tb[15] = (tb[15] + hx[i]) & 0xFF;
    return tb;
}

// ========== 生成单个广播包 ==========
inline std::string midea_make_packet(const std::string &mac, 
                                      const std::vector<uint8_t> &values, 
                                      int flag) {
    auto tb = midea_encode_table(mac);
    
    uint8_t ch=0x01, ver=0x01;
    uint8_t cmd_type = (values.size() > 1) ? 0x01 : 0x00;
    uint8_t chk = (ch + ver + cmd_type);
    for(auto v : values) chk = (chk + v) & 0xFF;
    
    std::vector<uint8_t> p = {ch, ver, cmd_type};
    for(auto v : values) p.push_back(v);
    while(p.size() < 15) p.push_back(0x00);
    p.push_back(chk);
    
    const char* H = "0123456789ABCDEF";
    auto th = [&](uint8_t b) -> std::string {
        return std::string(1, H[b>>4]) + std::string(1, H[b&0xF]);
    };
    
    std::string s = "0201021BFF114D19" + th(0x10 | (flag & 0x0F)) + mac + "01";
    int idx = flag & 0x0F;
    for(size_t i=1; i<16; i++) {
        s += th(p[i] ^ tb[idx % 16]);
        idx++;
    }
    return s;
}

// ========== 生成结束包 ==========
inline std::string midea_end_packet(const std::string &mac, int flag) {
    return midea_make_packet(mac, {0x00}, flag);
}

// ========== 生成完整命令（CMD + END） ==========
inline std::string midea_build_cmd(const std::string &mac, 
                                    const std::vector<uint8_t> &values, 
                                    int ctrl_f, int end_f) {
    return midea_make_packet(mac, values, ctrl_f) + "|" + midea_end_packet(mac, end_f);
}

// ========== 辅助函数 (用于发送指令) ==========
inline uint8_t midea_kelvin_to_val(int kelvin) {
    return std::max(0, std::min(255, (int)std::round((kelvin - 2700.0) * 255.0 / 3800.0)));
}

inline uint8_t midea_pct_to_val(int pct) {
    return std::round(pct * 255.0 / 100.0);
}

// ========== 灯光命令 ==========
inline std::string midea_light_on(const std::string &mac, int pct, int kelvin) {
    uint8_t bv = midea_pct_to_val(pct);
    uint8_t tv = midea_kelvin_to_val(kelvin);
    return midea_build_cmd(mac, {0x5B, bv, tv}, 2, 4);
}

inline std::string midea_light_off(const std::string &mac) {
    return midea_build_cmd(mac, {0x06}, 2, 4);
}

inline std::string midea_light_brightness(const std::string &mac, int pct, int current_ct) {
    uint8_t bv = midea_pct_to_val(pct);
    uint8_t tv = midea_kelvin_to_val(current_ct);
    return midea_build_cmd(mac, {0x5B, bv, tv}, 2, 4);
}

inline std::string midea_light_color_temp(const std::string &mac, int kelvin) {
    uint8_t tv = midea_kelvin_to_val(kelvin);
    return midea_build_cmd(mac, {0x55, tv}, 7, 12);
}

// ========== 风扇命令 ==========
inline std::string midea_fan_on(const std::string &mac, int speed) {
    uint8_t cmd; int cf, ef;
    switch(speed) {
        case 1: cmd=0x19; cf=10; ef=7;  break;
        case 2: cmd=0x1A; cf=0;  ef=5;  break;
        case 3: cmd=0x81; cf=13; ef=5;  break;
        case 4: cmd=0x88; cf=8;  ef=12; break;
        case 5: cmd=0x85; cf=10; ef=12; break;
        case 6: cmd=0x86; cf=5;  ef=1;  break;
        default: return "";
    }
    return midea_build_cmd(mac, {cmd}, cf, ef);
}

inline std::string midea_fan_off(const std::string &mac) {
    return midea_build_cmd(mac, {0x09}, 8, 15);
}


// ================================================================
//                    2. 辅助函数 (用于解析 BLE 广播)
// ================================================================

static std::string mac_bytes_to_str(const uint8_t* mac) {
    char buf[13];
    snprintf(buf, sizeof(buf), "%02X%02X%02X%02X%02X%02X",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    return std::string(buf);
}

static int raw_to_percent(uint8_t raw) { 
    int pct = (int)(raw / 255.0f * 100.0f);
    return (pct < 0) ? 0 : ((pct > 100) ? 100 : pct);
}

static int raw_to_kelvin(uint8_t raw) { 
    int pct = (int)(raw / 255.0f * 100.0f);
    if (pct < 0) pct = 0;
    if (pct > 100) pct = 100;
    return 2700 + (6500 - 2700) * pct / 100; 
}


// ================================================================
//                    3. 设备模型 (Device Model)
// ================================================================

#define DEVICE_COUNT 4

struct MideaDevice {
    const char* name;
    uint8_t mac[6];
    const char* light_topic;
    const char* fan_topic;
    
    bool light_on = false;
    int brightness = 50;
    int color_temp = 4600;
    bool fan_on = false;
    int fan_speed = 1;
    
    uint32_t last_publish_ms = 0;
    
    std::string mac_str() const { return mac_bytes_to_str(mac); }
    
    // 调用底层协议函数生成指令
    std::string cmd_light_on() const { return midea_light_on(mac_str(), brightness, color_temp); }
    std::string cmd_light_off() const { return midea_light_off(mac_str()); }
    std::string cmd_fan_on() const { return midea_fan_on(mac_str(), fan_speed); }
    std::string cmd_fan_off() const { return midea_fan_off(mac_str()); }
    std::string cmd_brightness(int brt) const { return midea_light_brightness(mac_str(), brt, color_temp); }
    std::string cmd_color_temp(int ct) const { return midea_light_color_temp(mac_str(), ct); }

    bool update_light(bool on, int brt, int ct) {
        bool c = (light_on != on) || (brightness != brt) || (color_temp != ct);
        if (c) { light_on = on; brightness = brt; color_temp = ct; }
        return c;
    }
    
    bool update_fan(bool on, int spd) {
        bool c = (fan_on != on) || (fan_speed != spd);
        if (c) { fan_on = on; fan_speed = spd; }
        return c;
    }
};

static MideaDevice g_devices[DEVICE_COUNT] = {
    {"Dining", {0x48, 0x80, 0x27, 0x70, 0x00, 0x00}, "ctlight002", "ctfan003", false, 100, 5588, false, 1, 0},
    {"Bedroom", {0xF0, 0xCF, 0x2D, 0x70, 0x00, 0x00}, "cwlight002", "cwfan003", false, 100, 4600, false, 1, 0},
    {"Master", {0x92, 0x26, 0x35, 0x70, 0x00, 0x00}, "zzwlight002", "zzwfan003", false, 50, 4600, false, 1, 0},
    {"Living Room", {0xCD, 0xAB, 0x38, 0x70, 0x00, 0x00}, "kktlight002", "kktfan003", false, 50, 4600, false, 1, 0}
};


// ================================================================
//                    4. BLE 队列 & MQTT 发布管理
// ================================================================

struct BleQueueItem { std::string hex; uint32_t send_at; };
static std::vector<BleQueueItem> g_ble_queue;
static std::function<void(const std::string&)> g_ble_send_fn = nullptr;
static std::function<void(const char*, const char*)> g_mqtt_pub_fn = nullptr;

static void ble_set_send_function(std::function<void(const std::string&)> fn) { g_ble_send_fn = fn; }
static void mqtt_set_publish_function(std::function<void(const char*, const char*)> fn) { g_mqtt_pub_fn = fn; }

static void ble_queue_add(const std::string& hex, uint32_t delay_ms = 0) {
    // 【修复】使用 esphome::millis()
    g_ble_queue.push_back({hex, esphome::millis() + delay_ms});
}

static void ble_queue_process() {
    // 【修复】使用 esphome::millis()
    if (!g_ble_queue.empty() && esphome::millis() >= g_ble_queue[0].send_at && g_ble_send_fn) {
        g_ble_send_fn(g_ble_queue[0].hex);
        g_ble_queue.erase(g_ble_queue.begin());
    }
}

static void device_publish_bemfa(MideaDevice& dev) {
    if (!g_mqtt_pub_fn) return;
    // 【修复】使用 esphome::millis()
    if (esphome::millis() - dev.last_publish_ms < 200) return; 
    dev.last_publish_ms = esphome::millis();
    
    std::string lt = std::string(dev.light_topic) + "/up";
    if (dev.light_on) {
        std::string payload = "on#" + std::to_string(dev.brightness) + "#" + std::to_string(dev.color_temp);
        g_mqtt_pub_fn(lt.c_str(), payload.c_str());
    } else {
        g_mqtt_pub_fn(lt.c_str(), "off");
    }
    
    std::string ft = std::string(dev.fan_topic) + "/up";
    if (dev.fan_on) {
        std::string payload = "on#" + std::to_string(dev.fan_speed);
        g_mqtt_pub_fn(ft.c_str(), payload.c_str());
    } else {
        g_mqtt_pub_fn(ft.c_str(), "off");
    }
}

static void publish_all_devices() {
    for (int i = 0; i < DEVICE_COUNT; i++) {
        g_devices[i].last_publish_ms = 0;
        device_publish_bemfa(g_devices[i]);
    }
}


// ================================================================
//                    5. BLE 广播解析
// ================================================================

static int parse_ble_device(const uint8_t* raw, size_t sz, int idx) {
    MideaDevice& dev = g_devices[idx];
    for (int i = 2; i <= (int)sz - 6; i++) {
        if (memcmp(&raw[i], dev.mac, 6) != 0) continue;
        int rem = (int)sz - i;
        if (rem < 16) return -1;
        
        bool l=false; int b=0, c=2700; bool f=false; int s=0;
        uint8_t mode = raw[i+8];
        bool alt = (mode==0x10||mode==0x11||mode==0x12||mode==0x13||mode==0x20||mode==0x21||mode==0x32||mode==0x33);
        
        if (alt) {
            l = (mode==0x11||mode==0x13||mode==0x21||mode==0x33);
            f = (mode==0x12||mode==0x13||mode==0x20||mode==0x21||mode==0x32||mode==0x33);
            b = raw_to_percent(raw[i+11]); 
            c = raw_to_kelvin(raw[i+12]);
            s = f ? (raw[i+15]+1) : 0;
        } else {
            if(rem>=8) l=(raw[i+7]==0x01);
            if(rem>=13) b=raw_to_percent(raw[i+12]);
            if(rem>=14) c=raw_to_kelvin(raw[i+13]);
            if(rem>=17) { f=(raw[i+16]&0x01)!=0; }
            if(rem>=18&&f) s=raw[i+17]+1;
        }
        
        // 风速防乱跳修正
        if(f && (s<1||s>6)) { 
            int fs=0; 
            if(rem>=16){int t=raw[i+15]+1; if(t>=1&&t<=6)fs=t;} 
            if(fs==0&&rem>=18){int t=raw[i+17]+1; if(t>=1&&t<=6)fs=t;} 
            s=(fs>=1&&fs<=6)?fs:1; 
        } else if(!f) {
            s=0;
        }

        bool lc = dev.update_light(l, b, c);
        bool fc = dev.update_fan(f, s);
        if (lc || fc) device_publish_bemfa(dev);
        return idx;
    }
    return -1;
}

static void parse_ble_advertisement(const std::vector<uint8_t>& raw) {
    if (raw.size() < 20) return;
    for (int i = 0; i < DEVICE_COUNT; i++) {
        if (parse_ble_device(raw.data(), raw.size(), i) >= 0) return;
    }
}


// ================================================================
//                    6. MQTT 下行指令处理
// ================================================================

static void handle_light_mqtt(int idx, const std::string& p) {
    if (idx < 0 || idx >= DEVICE_COUNT) return;
    MideaDevice& dev = g_devices[idx];
    if (p == "on") { 
        dev.light_on = true; 
        if(dev.brightness<1)dev.brightness=50; 
        ble_queue_add(dev.cmd_light_on()); 
    }
    else if (p == "off") { 
        dev.light_on = false; 
        ble_queue_add(dev.cmd_light_off()); 
    }
    else if (p.find("on#") == 0) {
        std::string r = p.substr(3); size_t pos = r.find('#');
        dev.brightness = std::stoi(r.substr(0, pos));
        if(pos!=std::string::npos){
            int v=std::stoi(r.substr(pos+1)); 
            if(v>=2700&&v<=6500)dev.color_temp=v;
        }
        dev.light_on = true; 
        ble_queue_add(dev.cmd_light_on());
    }
    device_publish_bemfa(dev);
}

static void handle_fan_mqtt(int idx, const std::string& p) {
    if (idx < 0 || idx >= DEVICE_COUNT) return;
    MideaDevice& dev = g_devices[idx];
    if (p == "on") { 
        dev.fan_on = true; 
        if(dev.fan_speed<1)dev.fan_speed=1; 
        ble_queue_add(dev.cmd_fan_on()); 
    }
    else if (p == "off") { 
        dev.fan_on = false; 
        ble_queue_add(dev.cmd_fan_off()); 
    }
    else if (p.find("on#") == 0) { 
        int s=std::stoi(p.substr(3)); 
        if(s<1)s=1; if(s>6)s=6; 
        dev.fan_on=true; 
        dev.fan_speed=s; 
        ble_queue_add(dev.cmd_fan_on()); 
    }
    device_publish_bemfa(dev);
}


// ================================================================
//                    7. 定时任务调度
// ================================================================

static void tick_ble_queue() { 
    ble_queue_process(); 
}

static void tick_heartbeat() {
    static uint32_t last_hb = 0;
    // 【修复】使用 esphome::millis()
    if (esphome::millis() - last_hb < 60000) return;
    last_hb = esphome::millis();
    publish_all_devices();
}
