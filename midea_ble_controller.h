#pragma once
#include <vector>
#include <string>
#include <cmath>

inline std::string midea_build_cmd(const std::string &mac, 
                                    const std::vector<uint8_t> &values, 
                                    int ctrl_f, int end_f) {
    uint8_t hx[6];
    for(int i=0; i<6; i++) 
        hx[i] = std::stoi(mac.substr(i*2, 2), nullptr, 16);
    
    uint8_t tb[16];
    int l=0, r=1;
    for(int i=0; i<15; i++) {
        tb[i] = (hx[l] + hx[r]) & 0xFF;
        r++;
        if(r==6) { l++; r=l+1; }
    }
    tb[15] = 0;
    for(int i=0; i<6; i++) tb[15] = (tb[15] + hx[i]) & 0xFF;
    
    auto make_pkt = [&](int flag) -> std::string {
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
    };
    
    return make_pkt(ctrl_f) + "|" + make_pkt(end_f);
}

inline void midea_fan_params(int speed, uint8_t &cmd, int &cf, int &ef) {
    switch(speed) {
        case 1: cmd=0x19; cf=10; ef=7;  break;
        case 2: cmd=0x1A; cf=0;  ef=5;  break;
        case 3: cmd=0x81; cf=13; ef=5;  break;
        case 4: cmd=0x88; cf=8;  ef=12; break;
        case 5: cmd=0x85; cf=10; ef=12; break;
        case 6: cmd=0x86; cf=5;  ef=1;  break;
    }
}

inline uint8_t midea_kelvin_to_val(int kelvin) {
    return std::max(0, std::min(255, 
        (int)std::round((kelvin - 2700.0) * 255.0 / 3800.0)));
}

inline uint8_t midea_pct_to_val(int pct) {
    return std::round(pct * 255.0 / 100.0);
}
