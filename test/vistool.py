#!/usr/bin/python3

import argparse
import json
import subprocess

def addHeader(jsonString):
    return ('Content-Length: ' + str(len(jsonString)) + "\r\n\r\n" + jsonString).encode('utf-8')

def main(args):
    hlangd = subprocess.Popen([args.hlangdPath], stdin=subprocess.PIPE, stdout=subprocess.PIPE)
    initialize = json.dumps({'jsonrpc': '2.0', 'id': 0, 'method': 'initialize', 'params': {}})
    result = hlangd.communicate(addHeader(initialize))[0]
    print(addHeader(initialize))
    print(result)

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Hadron visualization tool.')
    parser.add_argument('--inputFile', help='input SuperCollider file to visualize', required=True)
    parser.add_argument('--ouputDir', help='output directory for visualization report', required=True)
    parser.add_argument('--hlangdPath', help='path to hlangd binary', default='build/src/hlangd')
    args = parser.parse_args()
    main(args)