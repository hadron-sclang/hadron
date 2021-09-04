#!/usr/bin/python3

import HLangDClient

import argparse
import os

def styleForTokenType(typeIndex):
    tokenTypeStyles = [
        "color: lightgrey;",        # kEmpty
        "color: red;",              # kInterpret
        "color: blue;",             # kLiteral
        "color: green;",            # kPrimitive
        "color: orange;",           # kPlus
        "color: orange;",           # kMinus
        "color: orange;",           # kAsterisk
        "color: red;",              # kAssign
        "color: orange;",           # kLessThan
        "color: orange;",           # kGreaterThan
        "color: orange;",           # kPipe
        "color: orange;",           # kReadWriteVar
        "color: orange;",           # kLeftArrow
        "color: orange;",           # kBinop
        "color: black;",            # kKeyword
        "color: grey;",             # kOpenParen
        "color: grey;",             # kCloseParen
        "color: grey;",             # kOpenCurly
        "color: grey;",             # kCloseCurly
        "color: grey;",             # kOpenSquare
        "color: grey;",             # kCloseSquare
        "color: grey;",             # kComma
        "color: grey;",             # kSemicolon
        "color: grey;",             # kColon
        "color: grey;",             # kCaret
        "color: grey;",             # kTilde
        "color: grey;",             # kHash
        "color: grey;",             # kGrave
        "color: grey;",             # kVar
        "color: grey;",             # kArg
        "color: grey;",             # kConst
        "color: black;",            # kClassVar
        "color: grey;",             # kDot
        "color: grey;",             # kDotDot
        "color: grey;",             # kEllipses
        "color: grey;",             # kCurryArgument
        "color: yellow;",           # kIf
    ]
    return tokenTypeStyles[typeIndex]

def buildListing(outFile, tokens, source):
    lineNumber = 0
    outFile.write(
"""
<h3>source</h3>
<div style="font-family: monospace">
""")
    lineNumber = 0
    tokenIndex = 0
    tokenStyle = styleForTokenType(0)
    for line in source:
        outFile.write('<span style="color: black;">{}:</span> '.format(lineNumber))
        if lineNumber < tokens[tokenIndex]:
            outFile.write('<span style="{}">{}</span><br>\n'.format(tokenStyle, line))
        else:
            
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