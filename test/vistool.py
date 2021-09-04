#!/usr/bin/python3

import HLangDClient

import argparse
import os

def buildListing(outFile, tokens, source):
    lineNumber = 0
    outFile.write(
"""
<h3>source</h3>
<div style="font-family: monospace">
""")
    lineNumber = 0
    for line in source:
        outFile.write("{}: {}<br>\n".format(lineNumber, line))
        lineNumber += 1
    outFile.write(
"""</div>
"""
    )



def main(args):
    client = HLangDClient.HLangDClient()
    if not client.connect(args.hlangdPath):
        return -1
    print('connected to {} version {}'.format(client.serverName, client.serverVersion))
    parseTree = client.getParseTree(args.inputFile)
    outFile = open(os.path.join(args.outputDir, 'index.html'), 'w')
    outFile.write(
"""<html>
<head>
  <title>hadron vis report for {}</title></head>
  <style>
  body {{ font-family: Verdana }}
  </style>
<body>
  <h2>hadron vis report for '{}'</h2>
""".format(args.inputFile, args.inputFile))
    # read raw file
    with open(args.inputFile, 'r') as inFile:
        source = inFile.readlines()
    tokens = client.getSemanticTokens(args.inputFile)
    buildListing(outFile, tokens, source)
    outFile.write(
"""</body>
</html>
""")

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Hadron visualization tool.')
    parser.add_argument('--inputFile', help='input SuperCollider file to visualize', required=True)
    parser.add_argument('--outputDir', help='output directory for visualization report', required=True)
    parser.add_argument('--hlangdPath', help='path to hlangd binary', default='build/src/hlangd')
    args = parser.parse_args()
    main(args)