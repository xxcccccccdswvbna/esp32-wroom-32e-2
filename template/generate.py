#!/usr/bin/env python3
import yaml
import jinja2

def mac_to_bytes(mac_str):
    return ', '.join([f'0x{mac_str[i:i+2]}' for i in range(0, 12, 2)])

def main():
    with open('config/devices.yaml', 'r', encoding='utf-8') as f:
        config = yaml.safe_load(f)

    env = jinja2.Environment(
        loader=jinja2.FileSystemLoader('template'),
        trim_blocks=True,
        lstrip_blocks=True
    )
    env.filters['mac_to_bytes'] = mac_to_bytes

    template = env.get_template('gateway.yaml.j2')
    output = template.render(gateway=config['gateway'], devices=config['devices'])

    with open('ble_gateway.yaml', 'w', encoding='utf-8') as f:
        f.write(output)
    print("✅ ble_gateway.yaml generated successfully!")

if __name__ == '__main__':
    main()
