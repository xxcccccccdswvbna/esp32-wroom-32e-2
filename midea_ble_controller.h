#pragma once
#include <vector>
#include <string>
#include <cmath>

inline std::vector<uint8_t> midea_encode_table(const std::string &mac) {
    uint8_t hx[6];
    for(int i=0; i<6; i++) hx[i] = std::stoi(mac.substr(i*2, 2), nullptr, 16);
    std::vector<uint8_t> tb(16);
    int l=0, r=1;
    for(int i=0; i<15; i++) { tb[i] = (hx[l] + hx[r]) & 0xFF; r++; if(r==6) { l++; r=l+1; } }
    tb[15] = 0; for(int i=0; i<6; i++) tb[15] = (tb[15] + hx[i]) & 0xFF;
    return tb;
}

inline std::string midea_make_packet(const std::string &mac, const std::vector<uint8_t> &values, int flag) {
    auto tb = midea_encode_table(mac);
    uint8_t ch=0x01, ver=0x01, cmd_type=(values.size()>1)?0x01:0x00, chk=(ch+ver+cmd_type);
    for(auto v:values) chk=(chk+v)&0xFF;
    std::vector<uint8_t> p={ch,ver,cmd_type};
    for(auto v:values) p.push_back(v);
    while(p.size()<15) p.push_back(0x00);
    p.push_back(chk);
    const char* H="0123456789ABCDEF";
    auto th=[&](uint8_t b){return std::string(1,H[b>>4])+std::string(1,H[b&0xF]);};
    std::string s="0201021BFF114D19"+th(0x10|(flag&0x0F))+mac+"01";
    int idx=flag&0x0F;
    for(size_t i=1;i<16;i++){s+=th(p[i]^tb[idx%16]);idx++;}
    return s;
}

inline std::string midea_end_packet(const std::string &mac, int flag) { return midea_make_packet(mac,{0x00},flag); }

inline std::string midea_build_cmd(const std::string &mac, const std::vector<uint8_t> &values, int ctrl_f, int end_f) {
    return midea_make_packet(mac,values,ctrl_f)+"|"+midea_end_packet(mac,end_f);
}

inline uint8_t midea_kelvin_to_val(int kelvin) { return std::max(0,std::min(255,(int)std::round((kelvin-2700.0)*255.0/3800.0))); }
inline uint8_t midea_pct_to_val(int pct) { return std::round(pct*255.0/100.0); }

inline std::string midea_light_on(const std::string &mac, int pct, int kelvin) {
    uint8_t bv=midea_pct_to_val(pct), tv=midea_kelvin_to_val(kelvin);
    return midea_build_cmd(mac,{0x5B,bv,tv},2,4);
}
inline std::string midea_light_off(const std::string &mac) { return midea_build_cmd(mac,{0x06},2,4); }
inline std::string midea_light_brightness(const std::string &mac, int pct, int ct) {
    uint8_t bv=midea_pct_to_val(pct), tv=midea_kelvin_to_val(ct);
    return midea_build_cmd(mac,{0x5B,bv,tv},2,4);
}
inline std::string midea_light_color_temp(const std::string &mac, int kelvin) {
    return midea_build_cmd(mac,{0x55,midea_kelvin_to_val(kelvin)},7,12);
}

inline std::string midea_fan_on(const std::string &mac, int speed) {
    uint8_t cmd; int cf,ef;
    switch(speed){case 1:cmd=0x19;cf=10;ef=7;break;case 2:cmd=0x1A;cf=0;ef=5;break;case 3:cmd=0x81;cf=13;ef=5;break;case 4:cmd=0x88;cf=8;ef=12;break;case 5:cmd=0x85;cf=10;ef=12;break;case 6:cmd=0x86;cf=5;ef=1;break;default:return"";}
    return midea_build_cmd(mac,{cmd},cf,ef);
}
inline std::string midea_fan_off(const std::string &mac) { return midea_build_cmd(mac,{0x09},8,15); }
