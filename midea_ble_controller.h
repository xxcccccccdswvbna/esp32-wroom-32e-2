#pragma once
#include <vector>
#include <string>
#include <sstream>
#include <iomanip>
#include <cmath>

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

    std::string build_payload(const std::vector<uint8_t> &values) {
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

    std::string build_broadcast(const std::vector<uint8_t> &values, uint8_t flag) {
        std::string payload = build_payload(values);
        std::string channel = payload.substr(0, 2);
        std::string encrypted = encrypt(payload.substr(2), flag);
        uint8_t enc_flag = 0x10 | (flag & 0x0F);
        std::stringstream ss;
        ss << "0201021BFF114D19" << std::setw(2) << std::setfill('0') << std::hex << (int)enc_flag
           << mac << channel << encrypted;
        return ss.str();
    }

    std::string make_command(const std::vector<uint8_t> &values, uint8_t ctrl_flag, uint8_t end_flag) {
        std::string ctrl = build_broadcast(values, ctrl_flag);
        std::string end = build_broadcast({0x00}, end_flag);
        return ctrl + "|" + end;
    }

    std::string make_fan_speed_command(int speed) {
        uint8_t cmd; uint8_t ctrl_flag, end_flag;
        switch (speed) {
            case 1: cmd = 0x19; ctrl_flag = 10; end_flag = 7; break;
            case 2: cmd = 0x1A; ctrl_flag = 0;  end_flag = 5; break;
            case 3: cmd = 0x81; ctrl_flag = 13; end_flag = 5; break;
            case 4: cmd = 0x88; ctrl_flag = 8;  end_flag = 12; break;
            case 5: cmd = 0x85; ctrl_flag = 10; end_flag = 12; break;
            case 6: cmd = 0x86; ctrl_flag = 5;  end_flag = 1; break;
            default: return "";
        }
        return make_command({cmd}, ctrl_flag, end_flag);
    }
};        ct = std::max(0, std::min(255, ct));
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
