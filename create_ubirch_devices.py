import argparse
import json

parser = argparse.ArgumentParser()

parser.add_argument('filename', type=str, help='name of input json-file')
parser.add_argument('--stage', type=str, default='prod', help='stage to use')
parser.add_argument('--out', type=str, help='name of output csv-file')

args = parser.parse_args()

if args.stage == 'prod':
    backend_public_key = '74BIrQbAKFrwF3AJOBgwxGzsAl0B2GCF51pPAEHC5pA='
elif args.stage == 'dev' or args.stage == 'demo':
    backend_public_key = 'okA7krya3TZbPNEv8SDQIGR/hOppg/mLxMh+D0vozWY='
else:
    raise Exception('Unknown stage')

out_filename = args.filename.split('.')[0] + '.csv' \
    if args.out is None else args.out

CSV_HEAD_TEMPLATE = '''key,type,encoding,value
key_storage,namespace,,
server_key,data,base64,{backend_public_key}
'''

CSV_DEVICE_TEMPLATE = '''{short_name},namespace,,
state,data,hex2bin,03
uuid,data,hex2bin,{uuid}
pw,data,binary,{password}
pre_sign,data,hex2bin,00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
'''

with open(args.filename, 'r') as _f:
    devices = json.loads(_f.read())

if type(devices) == dict:
    devices = [devices]

csv = CSV_HEAD_TEMPLATE.format(backend_public_key=backend_public_key)

for dev in devices:
    dev['uuid'] = dev['uuid'].replace('-', '')
    csv += CSV_DEVICE_TEMPLATE.format(short_name=dev['short_name'],
                                      uuid=dev['uuid'],
                                      password=dev['password'])

with open(out_filename, 'w') as _f:
    _f.write(csv)
