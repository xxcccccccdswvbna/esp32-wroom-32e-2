#pragma once
#include <vector>
#include <string>
#include <sstream>
#include <iomanip>
#include <cmath>

// 前置声明 ble_tx 组件，实际需要根据你的 ble_tx 头文件调整
// 假设 ble_tx 的 send_hex 方法可通过全局 id(tx) 访问，这里改为接收一个 send_hex 的函数指针
// 为简化，直接在方法中调用 id(tx).send_hex(...)，但 ESPHome lambda 中不能直接包含 id()，
// 因此我们将 send_hex 作为参数传递。这里使用模板/多态方式，但最简单的是传递一个 lambda。
// 由于 ESPHome 限制，我们改为在 YAML 的 lambda 中直接创建 MideaController 并调用 send_command，
// 而 send_command 内部需要调用 id(tx).send_hex。我们可以将 tx 作为参数传入方法中。

class MideaController {
public:
    std::string mac;

    MideaController(const std::string &mac_addr) : mac(mac_addr) {}

    std::vector<uint8_t> create_encode_table() {
        std::vector<uint8_t> hx;
        for (size_t i = 0; i < mac.length(); i += 2)
            hx.push_back(std::stoi(mac.substr(i, 2), nullptr, 16));
        std::vector<uint8_t> table;
        int l = 0, r = 1;
        while (l < 5) {
            table.push_back((hx[l] + hx[r]) & 0xFF);
            r++;
            if (r == 6) { l++; r = l + 1; }
        }
        uint16_t sum = 0;
        for (auto b : hx) sum += b;
        table.push_back(sum & 0xFF);
        return table;
    }

    std::string encrypt(const std::string &hex_str, uint8_t flag) {
        auto table = create_encode_table();
        std::vector<uint8_t> src;
        for (size_t i = 0; i < hex_str.length(); i += 2)
            src.push_back(std::stoi(hex_str.substr(i, 2), nullptr, 16));
        std::stringstream ss;
        size_t idx = flag & 0x0F;
        for (auto b : src) {
            ss << std::setw(2) << std::setfill('0') << std::hex
               << (b ^ table[idx % table.size()]);
            idx++;
        }
        return ss.str();
    }

    std::string build_payload(std::vector<uint8_t> values) {
        uint8_t ch = 0x01, ver = 0x01;
        uint8_t cmd_type = values.size() > 1 ? 0x01 : 0x00;
        uint8_t chk = (ch + ver + cmd_type);
        for (auto v : values) chk += v;
        chk &= 0xFF;
        std::vector<uint8_t> data = {ch, ver, cmd_type};
        data.insert(data.end(), values.begin(), values.end());
        while (data.size() < 15) data.push_back(0x00);
        data.push_back(chk);
        std::stringstream ss;
        for (auto b : data) ss << std::setw(2) << std::setfill('0') << std::hex << (int)b;
        return ss.str();
    }

    std::string build_broadcast(std::vector<uint8_t> values, uint8_t flag) {
        std::string payload = build_payload(values);
        std::string channel = payload.substr(0, 2);
        std::string encrypted = encrypt(payload.substr(2), flag);
        uint8_t enc_flag = 0x10 | (flag & 0x0F);
        std::stringstream ss;
        ss << "0201021BFF114D19" << std::setw(2) << std::setfill('0') << std::hex << (int)enc_flag
           << mac << channel << encrypted;
        return ss.str();
    }

    // send_hex 通过 ESPHome 的 id(tx).send_hex 调用，但由于无法直接传递 id，我们在 YAML 中调用时显式传入
    // 这里将 send_command 设计为接收一个函数指针或直接在外部拼接后调用 id(tx).send_hex
    // 简便起见，我们只在 YAML lambda 中使用这些方法，因此下面方法仅返回字符串，然后外部发送。
    std::string make_command(std::vector<uint8_t> values, uint8_t ctrl_flag, uint8_t end_flag) {
        std::string ctrl = build_broadcast(values, ctrl_flag);
        std::string end = build_broadcast({0x00}, end_flag);
        return ctrl + "|" + end;
    }

    // 便捷方法，返回完整命令字符串
    void send_light_toggle(ble_tx::BLETx *tx) { tx->send_hex(make_command({0x06}, 2, 4).c_str()); }
    void send_light_brightness(ble_tx::BLETx *tx, int percent) {
        uint8_t val = round(percent * 255.0 / 100.0);
        tx->send_hex(make_command({0x51, val}, 2, 4).c_str());
    }
    void send_light_color_temp(ble_tx::BLETx *tx, int kelvin) {
        int v = round((kelvin - 2700.0) * 255.0 / (6500.0 - 2700.0));
        v = std::max(0, std::min(255, v));
        tx->send_hex(make_command({0x55, (uint8_t)v}, 2, 4).c_str());
    }
    void send_light_brightness_color(ble_tx::BLETx *tx, int percent, int kelvin) {
        uint8_t brt = round(percent * 255.0 / 100.0);
        int ct = round((kelvin - 2700.0) * 255.0 / (6500.0 - 2700.0));
        ct = std::max(0, std::min(255, ct));
        tx->send_hex(make_command({0x5B, brt, (uint8_t)ct}, 2, 4).c_str());
    }
    void send_fan_toggle(ble_tx::BLETx *tx) { tx->send_hex(make_command({0x09}, 8, 15).c_str()); }
    void send_fan_speed(ble_tx::BLETx *tx, int speed) {
        uint8_t cmd; uint8_t ctrl_flag, end_flag;
        switch (speed) {
            case 1: cmd = 0x19; ctrl_flag = 10; end_flag = 7; break;
            case 2: cmd = 0x1A; ctrl_flag = 0;  end_flag = 5; break;
            case 3: cmd = 0x81; ctrl_flag = 13; end_flag = 5; break;
            case 4: cmd = 0x88; ctrl_flag = 8;  end_flag = 12; break;
            case 5: cmd = 0x85; ctrl_flag = 10; end_flag = 12; break;
            case 6: cmd = 0x86; ctrl_flag = 5;  end_flag = 1; break;
            default: return;
        }
        tx->send_hex(make_command({cmd}, ctrl_flag, end_flag).c_str());
    }
};
