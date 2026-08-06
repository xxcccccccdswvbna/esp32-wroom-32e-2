#pragma once
#include <vector>
#include <string>
#include <cmath>

// ========== 1. 加密表生成（基于设备 MAC） ==========
// 每个设备的 MAC 地址生成唯一的 16 字节加密表
// 算法：MAC 的 6 个字节两两相加取低 8 位，最后追加总和对 256 取模
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

// ========== 2. 生成单个 BLE 广播包 ==========
// mac: 设备 MAC 地址（倒序，如 "488027700000"）
// values: 控制命令值列表（如 {0x06} 表示开关，{0x51, 0x80} 表示亮度50%）
// flag: 加密表起始索引（0-15），不同命令使用不同 flag 配对
inline std::string midea_make_packet(const std::string &mac, 
                                      const std::vector<uint8_t> &values, 
                                      int flag) {
    auto tb = midea_encode_table(mac);
    
    // 构建明文 Payload：Channel(01) + Version(01) + CmdType + Values + 填充 + Checksum
    uint8_t ch=0x01, ver=0x01;
    uint8_t cmd_type = (values.size() > 1) ? 0x01 : 0x00;  // 单值命令=0x00，多值=0x01
    uint8_t chk = (ch + ver + cmd_type);
    for(auto v : values) chk = (chk + v) & 0xFF;           // 累加校验和
    
    std::vector<uint8_t> p = {ch, ver, cmd_type};
    for(auto v : values) p.push_back(v);
    while(p.size() < 15) p.push_back(0x00);                 // 填充到 15 字节
    p.push_back(chk);                                       // 追加校验和
    
    // XOR 加密（Channel 不加密，从第 1 字节开始加密）
    const char* H = "0123456789ABCDEF";
    auto th = [&](uint8_t b) -> std::string {
        return std::string(1, H[b>>4]) + std::string(1, H[b&0xF]);
    };
    
    // 组装完整广播包：固定头 + EncryptFlag + MAC + Channel(01) + 加密数据
    std::string s = "0201021BFF114D19" + th(0x10 | (flag & 0x0F)) + mac + "01";
    int idx = flag & 0x0F;
    for(size_t i=1; i<16; i++) {
        s += th(p[i] ^ tb[idx % 16]);
        idx++;
    }
    return s;
}

// ========== 3. 生成结束包（END 释放包） ==========
// 每个控制命令后必须发送结束包，否则下次命令可能无效
// 结束包固定使用 values = {0x00}
inline std::string midea_end_packet(const std::string &mac, int flag) {
    return midea_make_packet(mac, {0x00}, flag);
}

// ========== 4. 生成完整命令（控制包 + 结束包） ==========
// 返回格式：控制包HEX|结束包HEX，由 ble_tx 组件拆分发送
inline std::string midea_build_cmd(const std::string &mac, 
                                    const std::vector<uint8_t> &values, 
                                    int ctrl_f, int end_f) {
    return midea_make_packet(mac, values, ctrl_f) + "|" + midea_end_packet(mac, end_f);
}

// ========== 5. 辅助转换函数 ==========
// 色温 Kelvin -> 协议值（0-255）
inline uint8_t midea_kelvin_to_val(int kelvin) {
    return std::max(0, std::min(255, (int)std::round((kelvin - 2700.0) * 255.0 / 3800.0)));
}

// 亮度百分比 -> 协议值（0-255）
inline uint8_t midea_pct_to_val(int pct) {
    return std::round(pct * 255.0 / 100.0);
}

// ========== 6. 灯光控制命令 ==========
// 开灯（带亮度和色温）- 使用组合命令 0x5B
inline std::string midea_light_on(const std::string &mac, int pct, int kelvin) {
    uint8_t bv = midea_pct_to_val(pct);
    uint8_t tv = midea_kelvin_to_val(kelvin);
    return midea_build_cmd(mac, {0x5B, bv, tv}, 2, 4);  // flag: 控制2/结束4
}

// 关灯
inline std::string midea_light_off(const std::string &mac) {
    return midea_build_cmd(mac, {0x06}, 2, 4);
}

// 调节亮度（保持当前色温）
inline std::string midea_light_brightness(const std::string &mac, int pct, int current_ct) {
    uint8_t bv = midea_pct_to_val(pct);
    uint8_t tv = midea_kelvin_to_val(current_ct);
    return midea_build_cmd(mac, {0x5B, bv, tv}, 2, 4);
}

// 调节色温
inline std::string midea_light_color_temp(const std::string &mac, int kelvin) {
    uint8_t tv = midea_kelvin_to_val(kelvin);
    return midea_build_cmd(mac, {0x55, tv}, 7, 12);  // flag: 控制7/结束12
}

// ========== 7. 风扇控制命令 ==========
// 风扇开（指定档位）
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

// 风扇关（toggle 模式）
inline std::string midea_fan_off(const std::string &mac) {
    return midea_build_cmd(mac, {0x09}, 8, 15);
}

// ========== 8. 定时命令 ==========
// 定时器（分钟）
inline std::string midea_timer(const std::string &mac, int minutes) {
    uint8_t cmd;
    switch(minutes) {
        case 0:   cmd=0x50; break;  // 取消定时
        case 60:  cmd=0x52; break;  // 1小时
        case 120: cmd=0x53; break;  // 2小时
        case 180: cmd=0x54; break;  // 3小时
        case 240: cmd=0x56; break;  // 4小时
        case 300: cmd=0x57; break;  // 5小时
        case 360: cmd=0x58; break;  // 6小时
        default: return "";
    }
    return midea_build_cmd(mac, {cmd}, 2, 4);
}
