#!/usr/bin/python3

import HLangDClient

import argparse
import html
import os
import subprocess

def saveNode(node, tokens, dotFile):
    dotFile.write("""  node_{} [shape=plain label=<<table border="0" cellborder="1" cellspacing="0">
    <tr><td bgcolor="lightGray"><b>{}</b></td></tr>
    <tr><td><font face="monospace">{}</font></td></tr>
    <tr><td port="next">next</td></tr>
""".format(node['serial'], node['nodeType'], tokens[node['tokenIndex']]['source']))

    if node['nodeType'] == 'Empty':
        dotFile.write('    </table>>]\n')

    elif node['nodeType'] == 'VarDef':
        dotFile.write("""    <tr><td>hasReadAccessor: {}</td></tr>
    <tr><td>hasWriteAccessor: {}</td></tr>
    <tr><td port="initialValue">initialValue</td</tr></table>>]
""".format(node['hasReadAccessor'], node['hasWriteAccessor']))
        if node['initialValue']:
            dotFile.write('  node_{}:initialValue -> node_{}\n'.format(node['serial'], node['initialValue']['serial']))
            saveNode(node['initialValue'], tokens, dotFile)

    elif node['nodeType'] == 'VarList':
        dotFile.write('    <tr><td port="definitions">definitions</td></tr></table>>]\n')
        if node['definitions']:
            dotFile.write('  node_{}:definitions -> node_{}\n'.format(node['serial'], node['definitions']['serial']))
            saveNode(node['definitions'], tokens, dotFile)

    elif node['nodeType'] == 'ArgList':
        dotFile.write('    <tr><td port="varList">varList</td></tr></table>>]\n')
        if node['varList']:
            dotFile.write('  node_{}:varList -> node_{}\n'.format(node['serial'], node['varList']['serial']))
            saveNode(node['varList'], tokens, dotFile)

    elif node['nodeType'] == 'Method':
        dotFile.write("""    <tr><td>isClassMethod: {}</td></tr>
    <tr><td port="body">body</td></tr></table>>]
""".format(node['isClassMethod']))
        if node['body']:
            dotFile.write('  node_{}:body -> node_{}\n'.format(node['serial'], node['body']['serial']))
            saveNode(node['body'], tokens, dotFile)

    elif node['nodeType'] == 'ClassExt':
        dotFile.write('    <tr><td port="methods">methods</td></tr></table>>]\n')
        if node['methods']:
            dotFile.write('  node_{}:methods -> node_{}\n'.format(node['serial'], node['methods']['serial']))
            saveNode(node['methods'], tokens, dotFile)

    elif node['nodeType'] == 'Class':
        if node['superClassNameIndex']:
            dotFile.write('    <tr><td>superClassName: {}</td></tr>\n'.format(tokens[node['superClassNameIndex']]))
        if node['optionalNameIndex']:
            dotFile.write('    <tr><td>optionalName: {}</td></tr>\n'.format(tokens[node['optionalClassNameIndex']]))
        dotFile.write("""    <tr><td port="variables">variables</td></tr>
    <tr><td port="methods">methods</td></tr></table>>]
""")
        if node['variables']:
            dotFile.write('  node_{}:variables -> node_{}\n'.format(node['serial'], node['variables']['serial']))
            saveNode(node['variables'], tokens, dotFile)
        if node['methods']:
            dotFile.write('  node_{}:methods -> node_{}\n'.format(node['serial'], node['methods']['serial']))
            saveNode(node['methods'], tokens, dotFile)

    elif node['nodeType'] == 'ReturnNode':
        dotFile.write('    <tr><td port="valueExpr">valueExpr</td></tr></table>>]\n')
        if node['valueExpr']:
            dotFile.write('  node_{}:valueExpr -> node_{}\n'.format(node['serial'], node['valueExpr']['serial']))
            saveNode(node['valueExpr'], tokens, dotFile)

    elif node['nodeType'] == 'DynList':
        dotFile.write('    <tr><td port="elements">elements</td></tr></table>>]\n')
        if node['elements']:
            dotFile.write('  node_{}:elements -> node_{}\n'.format(node['serial'], node['elements']['serial']))
            saveNode(node['elements'], tokens, dotFile)

    elif node['nodeType'] == 'Block':
        dotFile.write("""    <tr><td port="arguments">arguments</td></tr>
    <tr><td port="variables">variables</td></tr>
    <tr><td port="body">body</td></tr></table>>]
""")
        if node['arguments']:
            dotFile.write('  node_{}:arguments -> node_{}\n'.format(node['serial'], node['arguments']['serial']))
            saveNode(node['arguments'], tokens, dotFile)
        if node['variables']:
            dotFile.write('  node_{}:variables -> node_{}\n'.format(node['serial'], node['variables']['serial']))
            saveNode(node['arguments'], tokens, dotFile)
        if node['body']:
            dotFile.write('  node_{}:body -> node_{}\n'.format(node['serial'], node['body']['serial']))
            saveNode(node['body'], tokens, dotFile)

    elif node['nodeType'] == 'Literal':
        if node['blockLiteral']:
            dotFile.write('    <tr><td port="blockLiteral">blockLiteral</td></tr></table>>]\n')
            dotFile.write('  node_{}:blockLiteral -> node_{}\n'.format(node['serial'], node['blockLiteral']['serial']))
            saveNode(node['blockLiteral'], tokens, dotFile)
        else:
            dotFile.write('    </table>>]\n')

    elif node['nodeType'] == 'Name':
        dotFile.write('    <tr><td>isGlobal: {}</td></tr></table>>]\n'.format(node['isGlobal']))

    elif node['nodeType'] == 'ExprSeq':
        dotFile.write('    <tr><td port="expr">expr</td></tr></table>>]\n')
        if node['expr']:
            dotFile.write('  node_{}:expr -> node_{}\n'.format(node['serial'], node['expr']['serial']))
            saveNode(node['expr'], tokens, dotFile)

    elif node['nodeType'] == 'Assign':
        dotFile.write("""    <tr><td port="name">name</td></tr>
    <tr><td port="value">value</td></tr></table>>]
""")
        if node['name']:
            dotFile.write('  node_{}:name -> node_{}\n'.format(node['serial'], node['name']['serial']))
            saveNode(node['name'], tokens, dotFile)
        if node['value']:
            dotFile.write('  node_{}:value -> node_{}\n'.format(node['serial'], node['value']['serial']))
            saveNode(node['value'], tokens, dotFile)

    elif node['nodeType'] == 'Setter':
        dotFile.write("""    <tr><td port="target">target</td></tr>
    <tr><td port="value">value</td></tr></table>>]
""")
        if node['target']:
            dotFile.write('  node_{}:target -> node_{}\n'.format(node['serial'], node['target']['serial']))
            saveNode(node['target'], tokens, dotFile)
        if node['value']:
            dotFile.write('  node_{}:value -> node_{}\n'.format(node['serial'], node['value']['serial']))
            saveNode(node['value'], tokens, dotFile)

    elif node['nodeType'] == 'Call':
        dotFile.write("""    <tr><td port="target">target</td></tr>
    <tr><td port="arguments">arguments</td></tr>
    <tr><td port="keywordArguments">keywordArguments</td></tr></table>>]
""")
        if node['target']:
            dotFile.write('  node_{}:target -> node_{}\n'.format(node['serial'], node['target']['serial']))
            saveNode(node['target'], tokens, dotFile)
        if node['arguments']:
            dotFile.write('  node_{}:arguments -> node_{}\n'.format(node['serial'], node['arguments']['serial']))
            saveNode(node['arguments'], tokens, dotFile)
        if node['keywordArguments']:
            dotFile.write('  node_{}:keywordArguments -> node_{}\n'.format(node['serial'],
                node['keywordArguments']['serial']))
            saveNode(node['keywordArguments'], tokens, dotFile)

    elif node['nodeType'] == 'BinopCall':
        dotFile.write("""    <tr><td port="leftHand">leftHand</td></tr>
    <tr><td port="rightHand">rightHand</td></tr>
    <tr><td port="adverb">adverb</td></tr><table>>]
""")
        if node['leftHand']:
            dotFile.write('  node_{}:leftHand -> node_{}\n'.format(node['serial'], node['leftHand']['serial']))
            saveNode(node['leftHand'], tokens, dotFile)
        if node['rightHand']:
            dotFile.write('  node_{}:rightHand -> node_{}\n'.format(node['serial'], node['rightHand']['serial']))
            saveNode(node['rightHand'], tokens, dotFile)
        if node['adverb']:
            dotFile.write('  node_{}:adverb -> node_{}\n'.format(node['serial'], node['adverb']['serial']))
            saveNode(node['adverb'], tokens, dotFile)

    elif node['nodeType'] == 'If':
        dotFile.write("""    <tr><td port="condition">condition</td></tr>
    <tr><td port="trueBlock">trueBlock</td></tr>
    <tr><td port="falseBlock">falseBlock</td></tr><table>>]
""")
        if node['condition']:
            dotFile.write('  node_{}:condition -> node_{}\n'.format(node['serial'], node['condition']['serial']))
            saveNode(node['condition'], tokens, dotFile)
        if node['trueBlock']:
            dotFile.write('  node_{}:trueBlock -> node_{}\n'.format(node['serial'], node['trueBlock']['serial']))
            saveNode(node['trueBlock'], tokens, dotFile)
        if node['falseBlock']:
            dotFile.write('  node_{}:falseBlock -> node_{}\n'.format(node['serial'], node['falseBlock']['serial']))
            saveNode(node['falseBlock'], tokens, dotFile)

    if node['next']:
        dotFile.write('  node_{}:next -> node_{}\n'.format(node['serial'], node['next']['serial']))
        saveNode(node['next'], tokens, dotFile)

def buildParseTree(outFile, parseTree, tokens, outputDir):
    # Build dotfile from parse tree nodes.
    dotPath = os.path.join(outputDir, 'parseTree.dot')
    dotFile = open(dotPath, 'w')
    dotFile.write('digraph HadronParseTree {\n')
    saveNode(parseTree, tokens, dotFile)
    dotFile.write('}\n')
    dotFile.close()
    # Execute command to generate svg image.
    svgPath = os.path.join(outputDir, 'parseTree.svg')
    svgFile = open(svgPath, 'w')
    subprocess.run(['dot', '-Tsvg', dotPath], stdout=svgFile)
    outFile.write(
"""
<h3>parse tree</h3>
<img src="parseTree.svg">
""")

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

    outFile.write('</div>\n')

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
    controlFlow = client.getControlFlow(args.inputFile)
    print(controlFlow)
    # buildControlFlow(outFile, controlFlow, args.outputDir)
    outFile.write("""
</body>
</html>
""")

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Hadron visualization tool.')
    parser.add_argument('--inputFile', help='input SuperCollider file to visualize', required=True)
    parser.add_argument('--outputDir', help='output directory for visualization report', required=True)
    parser.add_argument('--hlangdPath', help='path to hlangd binary', default='build/src/hlangd')
    args = parser.parse_args()
    main(args)