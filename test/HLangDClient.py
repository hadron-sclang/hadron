#!/usr/bin/python3

import json
import subprocess
import time

class HLangDClient:
    """Wrapper class around the hlangd server process."""
    def __init__(self):
        self.hlangd = None
        self.receiveBuffer = ""
        self.serverName = None
        self.serverVersion = None
        self.idSerial = 0
        self.tokenTypes = []

    def _sendMessage(self, jsonString):
        input = "Content-Length: {}\r\n\r\n{}\n".format(len(jsonString), jsonString).encode('utf-8')
        self.hlangd.stdin.write(input)
        self.hlangd.stdin.flush()

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
        while True:
            nextOffset = data.find("\r\n", offset)
            while nextOffset == -1:
                data += self._receiveData()
                nextOffset = data.find("\r\n", offset)
            line = data[offset:nextOffset + 2].strip()
            if len(line) == 0:
                offset = nextOffset + 2
                break
            elif line.startswith('Content-Length:'):
                contentLength = int(line[len('Content-Length:'):])
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

    # returns True if connection established and server initialized, False otherwise.
    def connect(self, hlangdPath, waitForDebugger):
        self.hlangd = subprocess.Popen([hlangdPath], stdin=subprocess.PIPE, stdout=subprocess.PIPE)
        if waitForDebugger:
            input('hlangd running on pid {}, press enter to continue..'.format(self.hlangd.pid))
        else:
            # TODO: find a better way to wait for hlangd to open. Without this sleep there's a race where the python can
            # send the initialize message too early, so it is never received by hlangd, which means that the script
            # deadlocks waiting for a response to the lost init message.
            time.sleep(1)
        self._sendMessage(json.dumps({'jsonrpc': '2.0', 'id': self.idSerial, 'method': 'initialize', 'params': {}}))
        self.idSerial += 1
        result = self._receiveMessage()
        if not result or 'result' not in result:
            print("recevied bad result from initalize response")
            return False
        serverInfo = result['result']['serverInfo']
        self.tokenTypes = result['result']['capabilities']['semanticTokensProvider']['legend']['tokenTypes']
        self.serverName = serverInfo['name']
        self.serverVersion = serverInfo['version']
        self._sendMessage(json.dumps({'jsonrpc': '2.0', 'method': 'initialized'}))
        return True

    def getSemanticTokens(self, filePath):
        self._sendMessage(json.dumps({'jsonrpc': '2.0', 'method': 'textDocument/semanticTokens/full',
                'params': {'textDocument': {'uri': filePath}}}))
        self.idSerial += 1
        result = self._receiveMessage()
        if not result or 'result' not in result:
            print("received bad result from semanticTokens response")
            return None
        data = result['result']['data']
        return data

    def getDiagnostics(self, filePath):
        self._sendMessage(json.dumps({'jsonrpc': '2.0', 'id': self.idSerial, 'method': 'hadron/compilationDiagnostics',
            'params': {'textDocument': {'uri': filePath}}}))
        self.idSerial += 1
        result = self._receiveMessage()
        if not result or 'result' not in result:
            print('received bad result from parseTree response')
            return None
        return result['result']
