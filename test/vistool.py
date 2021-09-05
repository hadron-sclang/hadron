#!/usr/bin/python3

import HLangDClient

import argparse
import html
import os

def saveNode(node, dotFile):
    dotFile.write("""  node_{} [shape=plain label=<<table border="0" cellborder="1" cellspacing="0">
    <tr><td bgcolor="lightGray"><b>{}</b></td></tr>
    <tr><td port="next">next</td></tr>
""".format(node['serial'], node['nodeType']))

    if node['nodeType'] == 'Empty':
        dotFile.write('    </table>>]\n')
    elif node['nodeType'] == 'VarDef':
        dotFile.write("""    <tr><td>hasReadAccessor: {}</td></tr>
    <tr><td>hasWriteAccessor: {}</td></tr>
    <tr><td port="initialValue">initialValue</td</tr></table>>]
""".format(node['hasReadAccessor'], node['hasWriteAccessor']))
        if node['initialValue']:
            dotFile.write('  node_{}:initialValue -> node_{}\n'.format(node['serial'], node['initialValue']['serial']))
            saveNode(node['initialValue'], dotFile)
    elif node['nodeType'] == 'VarList':
        dotFile.write('    <tr><td port="definitions">definitions</td></tr></table>>]\n')
        if node['definitions']:
            dotFile.write('  node_{}:definitions -> node_{}\n'.format(node['serial'], node['definitions']['serial']))
            saveNode(node['definitions'], dotFile)
    elif node['nodeType'] == 'ArgList':
        dotFile.write('    <tr><td port="varList">varList</td></tr></table>>]\n')
        if node['varList']:
            dotFile.write('  node_{}:varList -> node_{}\n'.format(node['serial'], node['varList']['serial']))
            saveNode(node['varList'], dotFile)
    elif node['nodeType'] == 'Method':
        dotFile.write("""    <tr><td>isClassMethod: {}</td></tr>
    <tr><td port="body">body</td></tr></table>>]
""".format(node['isClassMethod']))
        if node['body']:
            dotFile.write('  node_{}:body -> node_{}'.format(node['serial'], node['body']['serial']))
            saveNode(node['body'], dotFile)
    elif node['nodeType'] == 'ClassExt':
        pass
    elif node['nodeType'] == 'Class':
        pass
    elif node['nodeType'] == 'ReturnNode':
        pass
    elif node['nodeType'] == 'DynList':
        pass
    elif node['nodeType'] == 'Block':
        pass
    elif node['nodeType'] == 'Literal':
        pass
    elif node['nodeType'] == 'Name':
        pass
    elif node['nodeType'] == 'ExprSeq':
        pass
    elif node['nodeType'] == 'Assign':
        pass
    elif node['nodeType'] == 'Setter':
        pass
    elif node['nodeType'] == 'Call':
        pass
    elif node['nodeType'] == 'BinopCall':
        pass
    elif node['nodeType'] == 'If':
        pass

    if node['next']:
        dotFile.write('  node_{}:next -> node_{}\n'.format(node['serial'], node['next']['serial']))
        saveNode(node['next'], dotFile)

def buildParseTree(outFile, parseTree, tokens, outputDir):
    dotFile = open(os.path.join(outputDir, 'parseTree.dot'), 'w')
    dotFile.write('digraph HadronParseTree {\n')
    saveNode(parseTree, dotFile)
    dotFile.write('}\n')

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
    outFile.write(
"""
<h3>source</h3>
<div style="font-family: monospace">
""")
    lineNumber = 0
    lineStart = '<span style="color: black;">{}:</span> <span style="{}">'
    charNumber = 0
    for token in tokens:
        # process any source lines before token as text outside of any token
        while lineNumber < token['lineNumber']:
            outFile.write('{}{}</span></br>\n'.format(lineStart.format(lineNumber, styleForTokenType(0)),
                html.escape(source[lineNumber])))
            lineNumber += 1
            charNumber = 0

        # process any characters before token starts they are also outside, append them.
        line = ''
        if (charNumber == 0):
            if token['charNumber'] > 0:
                line = lineStart.format(lineNumber, styleForTokenType(0))
                line += html.escape(source[lineNumber][0:token['charNumber']])
                line += '</span><span style="{}">'.format(styleForTokenType(token['tokenType']))
            else:
                line = lineStart.format(lineNumber, styleForTokenType(token['tokenType']))
        else:
            line = '<span style="{}">'.format(styleForTokenType(token['tokenType']))

        tokenLength = 0
        charNumber = token['charNumber']
        tokenSource = ''
        while tokenLength < token['length']:
            lineRemain = min(len(source[lineNumber]) - charNumber, token['length'] - tokenLength)
            sourceLine = html.escape(source[lineNumber][charNumber : charNumber + lineRemain])
            tokenSource += sourceLine
            line += sourceLine
            tokenLength += lineRemain
            charNumber += lineRemain
            # Did we finish the line?
            if charNumber == len(source[lineNumber]) - 1:
                line += '</span><br>\n'
                tokenLength += 1
                charNumber = 0
                lineNumber += 1
                # Is there still token left over?
                if tokenLength < token['length']:
                    line += lineStart.format(lineNumber, styleForTokenType(token['tokenType']))

        if charNumber > 0:
            line += '</span>'
        outFile.write(line)
        token['source'] = tokenSource

    outFile.write(
"""</div>
"""
    )

def main(args):
    client = HLangDClient.HLangDClient()
    if not client.connect(args.hlangdPath):
        return -1
    print('connected to {} version {}'.format(client.serverName, client.serverVersion))
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
    deltaTokens = client.getSemanticTokens(args.inputFile)
    lineNumber = 0
    charNumber = 0
    tokens = []
    for i in range(int(len(deltaTokens) / 5)):
        if deltaTokens[i * 5] > 0:
            lineNumber += deltaTokens[i * 5]
            charNumber = deltaTokens[(i * 5) + 1]
        else:
            charNumber += deltaTokens[(i * 5) + 1]
        tokens.append({'lineNumber': lineNumber, 'charNumber': charNumber, 'length': deltaTokens[(i * 5) + 2],
            'tokenType': deltaTokens[(i * 5) + 3]})
    buildListing(outFile, tokens, source)
    parseTree = client.getParseTree(args.inputFile)
    buildParseTree(outFile, parseTree, tokens, args.outputDir)
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