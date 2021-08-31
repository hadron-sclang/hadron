#!/usr/bin/python3

import argparse
import json
import subprocess

class HLangDClient:
    """Wrapper class around the hlangd server process."""
    def __init__(self):
        self.hlangd = None
        self.receiveBuffer = ""

    def _sendMessage(self, jsonString):
        input = "Content-Length: {}\r\n\r\n{}".format(len(jsonString), jsonString).encode('utf-8')
        self.hlangd.stdin.write(input)

    # Lowest level receive function, returns string data received from stdout.
    def _receiveData(self):
        if len(self.receiveBuffer):
            buff = self.receiveBuffer
            self.receiveBuffer = ""
            return buff
        result = self.hlangd.stdout.readline()
        return result.decode('utf-8')

    # returns a parsed json object received from server or None on error.
    def _receiveMessage(self):
        # First parse headers to extract Content-Length: tag
        data = self._receiveData()
        offset = 0
        contentLength = 0
        print(data)
        while True:
            nextOffset = data.find("\r\n", offset)
            print(nextOffset)
            while nextOffset == -1:
                data += self._receiveData()
                nextOffset = data.find("\r\n", offset)
            line = data[offset:nextOffset + 2]
            if line.startswith('Content-Length:'):
                contentLength = int(line[len('Content-Length:'):])
            elif len(line) == 2 and data[nextOffset + 2:nextOffset + 4] == "\r\n":
                offset = nextOffset + 4
                break
            offset = nextOffset + 2

        if contentLength == 0:
            return None

        # Through header parsing with a contentLength value, truncate header from data then read until we have whole
        # message.
        data = data[offset:]
        while len(data) < contentLength:
            data += self._receiveData()

        # Store any excess data back in the receive buffer.
        if len(data) > contentLength:
            self.receiveBuffer += data[contentLength:]
            data = data[:contentLength]

        return json.loads(data)

    def connect(self, hlangdPath):
        self.hlangd = subprocess.Popen([hlangdPath], stdin=subprocess.PIPE, stdout=subprocess.PIPE)

        initialize = json.dumps({'jsonrpc': '2.0', 'id': 0, 'method': 'initialize', 'params': {}})
        self._sendMessage(initialize)
        result = self._receiveMessage()
        print(result)

def main(args):
    client = HLangDClient()
    client.connect(args.hlangdPath)

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Hadron visualization tool.')
    parser.add_argument('--inputFile', help='input SuperCollider file to visualize', required=True)
    parser.add_argument('--outputDir', help='output directory for visualization report', required=True)
    parser.add_argument('--hlangdPath', help='path to hlangd binary', default='build/src/hlangd')
    args = parser.parse_args()
    main(args)