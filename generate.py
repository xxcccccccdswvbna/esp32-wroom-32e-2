#!/usr/bin/env python3
import yaml
from jinja2 import Environment, FileSystemLoader
import os

def main():
    # 读取设备列表
    with open('devices.yaml', 'r', encoding='utf-8') as f:
        data = yaml.safe_load(f)
    devices = data['devices']
    
    # 设置 Jinja2 环境
    env = Environment(loader=FileSystemLoader('.'))
    
    # 生成主配置
    template_main = env.get_template('ble_gateway.yaml.j2')
    output_main = template_main.render(devices=devices)
    with open('ble_gateway.yaml', 'w', encoding='utf-8') as f:
        f.write(output_main)
    
    # 生成头文件
    template_header = env.get_template('midea_ble_controller.h.j2')
    output_header = template_header.render(devices=devices)
    with open('midea_ble_controller.h', 'w', encoding='utf-8') as f:
        f.write(output_header)
    
    print("✅ Configuration generated successfully.")
    print(f"   Devices: {len(devices)}")

if __name__ == '__main__':
    main()
