#include "ble_tx.h"
#include "esphome/core/log.h"
#include "esp_gap_ble_api.h"
#include <cctype>

namespace esphome {
namespace ble_tx {

static const char *TAG = "ble_tx";
BLETx *global_ble_tx = nullptr;

void BLETx::setup() { global_ble_tx = this; ESP_LOGI(TAG, "ready"); }

void BLETx::loop() {
    const uint32_t now = millis();
    if(cool_){if(now-t1_<COOL_MS)return;cool_=false;}
    if(adv_&&now-t0_>=ADV_MS){esp_ble_gap_stop_advertising();adv_=false;t1_=now;cool_=true;if(!q_.empty()){t2_=now+GAP_MS;wait_=true;}}
    if(wait_&&now>=t2_){wait_=false;next_();}
}

void BLETx::send_hex(const std::string &hex) {
    size_t s=0;
    while(true){size_t p=hex.find('|',s);auto pkt=(p==std::string::npos)?hex.substr(s):hex.substr(s,p-s);if(!pkt.empty())q_.push_back(pkt);if(p==std::string::npos)break;s=p+1;}
    if(!adv_&&!wait_&&!cool_)next_();
}

void BLETx::next_() { if(q_.empty())return; auto pkt=std::move(q_.front()); q_.pop_front(); raw_(pkt); }

void BLETx::raw_(const std::string &h) {
    auto d=bytes_(h); if(d.size()<5)return;
    esp_ble_gap_config_adv_data_raw(d.data(),d.size());
    esp_ble_adv_params_t p{}; p.adv_int_min=0x40;p.adv_int_max=0x80;p.adv_type=ADV_TYPE_NONCONN_IND;p.channel_map=ADV_CHNL_ALL;
    esp_ble_gap_start_advertising(&p); t0_=millis(); adv_=true;
}

std::vector<uint8_t> BLETx::bytes_(const std::string &h) {
    std::vector<uint8_t> d; d.reserve(h.size()/2); char pair[3]{}; int n=0;
    for(char c:h){if(!std::isxdigit((unsigned char)c))continue;pair[n++]=c;if(n==2){d.push_back((uint8_t)strtol(pair,nullptr,16));n=0;}}
    return d;
}

}  // namespace ble_tx
}  // namespace esphome
