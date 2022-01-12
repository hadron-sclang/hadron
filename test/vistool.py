#!/usr/bin/python3

import HLangDClient

import argparse
import html
import os
import subprocess

def reg(reg):
    if reg == -1:
        return 'SP'
    elif reg == -2:
        return 'CONTEXT'
    return 'GPR{}'.format(reg)

def buildMachineCode(outFile, machineCode):
    outFile.write('\n<h3>machine code</h3>\n<span style="font-family: monospace">')
    for inst in machineCode['instructions']:
        outFile.write('{:08x}: '.format(int(inst[0])))
        if inst[1] == 'addr':
            outFile.write('addr {}, {}, {}<br>\n'.format(reg(inst[2]), reg(inst[3]), reg(inst[4])))
        elif inst[1] == 'addi':
            outFile.write('addi {}, {}, {}<br>\n'.format(reg(inst[2]), reg(inst[3]), inst[4]))
        elif inst[1] == 'andi':
            outFile.write('andi {}, {}, 0x{:016x}<br>\n'.format(reg(inst[2]), reg(inst[3]), inst[4]))
        elif inst[1] == 'ori':
            outFile.write('ori {}, {}, 0x{:016x}<br>\n'.format(reg(inst[2]), reg(inst[3]), inst[4]))
        elif inst[1] == 'xorr':
            outFile.write('xorr {}, {}, {}<br>\n'.format(reg(inst[2]), reg(inst[3]), reg(inst[4])))
        elif inst[1] == 'movr':
            outFile.write('movr {}, {}<br>\n'.format(reg(inst[2]), reg(inst[3])))
        elif inst[1] == 'movi':
            outFile.write('movi {}, {}<br>\n'.format(reg(inst[2]), inst[3]))
        elif inst[1] == 'bgei':
            outFile.write('bgei {}, {}, {:08x}<br>\n'.format(reg(inst[2]), inst[3], inst[4]))
        elif inst[1] == 'beqi':
            outFile.write('beqi {}, {}, {:08x}<br>\n'.format(reg(inst[2]), inst[3], inst[4]))
        elif inst[1] == 'jmp':
            outFile.write('jmp {:08x}<br>\n'.format(inst[2]))
        elif inst[1] == 'jmpr':
            outFile.write('jmpr {}<br>\n'.format(reg(inst[2])))
        elif inst[1] == 'jmpi':
            outFile.write('jmpi {}<br>\n'.format(inst[2]))
        elif inst[1] == 'ldr_l':
            outFile.write('ldr_l {} {}<br>\n'.format(reg(inst[2]), reg(inst[3])))
        elif inst[1] == 'ldxi_w':
            outFile.write('ldxi_w {} {} {}<br>\n'.format(reg(inst[2]), reg(inst[3]), inst[4]))
        elif inst[1] == 'ldxi_i':
            outFile.write('ldxi_i {} {} {}<br>\n'.format(reg(inst[2]), reg(inst[3]), inst[4]))
        elif inst[1] == 'ldxi_l':
            outFile.write('ldxi_l {} {} {}<br>\n'.format(reg(inst[2]), reg(inst[3]), inst[4]))
        elif inst[1] == 'str_i':
            outFile.write('str_i {} {}<br>\n'.format(reg(inst[2]), reg(inst[3])))
        elif inst[1] == 'str_l':
            outFile.write('str_l {} {}<br>\n'.format(reg(inst[2]), reg(inst[3])))
        elif inst[1] == 'stxi_w':
            outFile.write('stxi_w {} {} {}<br>\n'.format(inst[2], reg(inst[3]), reg(inst[4])))
        elif inst[1] == 'stxi_i':
            outFile.write('stxi_i {} {} {}<br>\n'.format(inst[2], reg(inst[3]), reg(inst[4])))
        elif inst[1] == 'stxi_l':
            outFile.write('stxi_l {} {} {}<br>\n'.format(inst[2], reg(inst[3]), reg(inst[4])))
        elif inst[1] == 'ret':
            outFile.write('ret<br>\n')
        elif inst[1] == 'retr':
            outFile.write('retr {}<br>\n'.format(reg(inst[2])))
        elif inst[1] == 'reti':
            outFile.write('reti {}<br>\n'.format(inst[2]))
        else:
            outFile.write('unknown opcode: {}<br>\n'.format(inst[0]))
    outFile.write('\n</span>\n')

def findContainingLifetime(i, lifetimeList):
    for lifetime in lifetimeList:
        for range in lifetime['ranges']:
            # these are assumed to be in sorted order, so if we encounter something starting after i we have a miss
            if i < range['from']:
                return None
            if i < range['to']:
                return lifetime
    return None

def buildLinearBlock(outFile, linearBlock):
    outFile.write('\n<h3>linear block</h3>\n\n<table>\n<tr><th>line</th><th>label</th><th>hir</th>')
    for i in range(0, len(linearBlock['valueLifetimes'])):
        outFile.write('<th>v<sub>{}</sub></th>'.format(i))
    outFile.write('</tr>\n')
    for i in range(0, len(linearBlock['instructions'])):
        outFile.write('<tr><td align="right">{}</td>'.format(i))
        inst = linearBlock['instructions'][i]
        if inst['opcode'] == 'Label':
            outFile.write('<td>label_{}:</td><td>'.format(inst['blockNumber']))
            for phi in inst['phis']:
                outFile.write('{}<br/>'.format(hirToString(phi)))
            outFile.write('</td>')
        else:
            outFile.write('<td></td><td>{}</td>'.format(hirToString(inst)))
        # want to visualize "ranges" and "usages" for each of the lifetime intervals. In value lifetimes also useful
        # to see what register they are associated with, and in register lifetimes what value, and in spill what value.
        for j in range(0, len(linearBlock['valueLifetimes'])):
            lifetime = findContainingLifetime(i, linearBlock['valueLifetimes'][j])
            if lifetime:
                tdClass = 'live'
                if i in lifetime['usages']:
                    tdClass = 'used'
                outFile.write('<td class="{}">GPR{}</td>'.format(tdClass, lifetime['registerNumber']))
            else:
                outFile.write('<td></td>')
        outFile.write('</tr>')
    outFile.write('\n</table>\n')

def slotToString(slot):
    if 'value' in slot:
        return str(slot['value'])
    return slot['type']

def valueToString(value):
    flags = ' | '.join(value['typeFlags'])
    return '({}) v<sub>{}</sub>'.format(flags, value['number'])

def hirToString(hir):
    if hir['opcode'] == 'LoadArgument':
        return '{} &#8592; LoadArgument({})'.format(valueToString(hir['value']), hir['index'])
    elif hir['opcode'] == 'LoadArgumentType':
        return '{} &#8592; LoadArgumentType({})'.format(valueToString(hir['value']), hir['index'])
    elif hir['opcode'] == 'Constant':
        return '{} &#8592; {}'.format(valueToString(hir['value']), slotToString(hir['constant']))
    elif hir['opcode'] == 'StoreReturn':
        return 'StoreReturn({}, {})'.format(valueToString(hir['returnValue'][0]), valueToString(hir['returnValue'][1]))
    elif hir['opcode'] == 'ResolveType':
        return '{} &#8592; ResolveType({})'.format(valueToString(hir['value']), valueToString(hir['typeOfValue']))
    elif hir['opcode'] == 'Phi':
        phi = '{} &#8592; &phi;('.format(valueToString(hir['value']))
        phi += ','.join([x['number'] for x in hir['inputs']])
        phi += ')'
        return phi
    elif hir['opcode'] == 'Branch':
        return 'Branch to {}'.format(hir['blockNumber'])
    elif hir['opcode'] == 'BranchIfZero':
        return '{} &#8592; BranchIfZero {},{} to {}'.format(valueToString(hir['value']),
            valueToString(hir['condition'][0]), valueToString(hir['condition'][1]), hir['blockNumber'])
    elif hir['opcode'] == 'Label':
        return 'Label {}:'.format(hir['blockNumber'])
    elif hir['opcode'] == 'DispatchCall':
        dispatch = '{} &#8592; DispatchCall({}'.format(valueToString(hir['value'], valueToString(hir['arguments'][0])))
        for i in range(1, len(hir['arguments'])):
            dispatch += ', {}'.format(valueToString(hir['arguments'][i]))
        for i in range(0, len(hir['keywordArguments']), step=2):
            dispatch += ', {}: {}'.format(valueToString(hir['keywordArguments'][i]),
                    valueToString(hir['keywordArguments'][i + 1]))
        return dispatch + ')'
    elif hir['opcode'] == 'DispatchLoadReturn':
        return '{} &#8592; LoadReturn()'.format(valueToString(hir['value']))
    elif hir['opcode'] == 'DispatchLoadReturnType':
        return '{} &#8592; LoadReturnType()'.format(valueToString(hir['value']))
    elif hir['opcode'] == 'DispatchCleanup':
        return 'DispatchCleanup()'
    return '<unsupported hir opcode "{}">'.format(hir['opcode'])

def saveFrame(frame, dotFile):
    dotFile.write('  subgraph cluster_{} {{\n'.format(frame['frameSerial']))
    for block in frame['blocks']:
        dotFile.write(
"""   block_{} [shape=plain label=<<table border="0" cellpadding="6" cellborder="1" cellspacing="0">
      <tr><td bgcolor="lightGray"><b>Block {}</b></td></tr>
""".format(block['number'], block['number']))
        for phi in block['phis']:
            dotFile.write('      <tr><td>{}</td></tr>\n'.format(hirToString(phi)))
        for hir in block['statements']:
            dotFile.write('      <tr><td>{}</td></tr>\n'.format(hirToString(hir)))
        dotFile.write('      </table>>]\n')
    for subFrame in frame['subFrames']:
        saveFrame(subFrame, dotFile)
    dotFile.write('  }} // end of cluster_{}\n'.format(frame['frameSerial']))

def saveBlockGraph(frame, dotFile):
    for block in frame['blocks']:
        for succ in block['successors']:
            dotFile.write('    block_{} -> block_{}\n'.format(block['number'], succ))
    for subFrame in frame['subFrames']:
        saveBlockGraph(subFrame, dotFile)

def buildControlFlow(outFile, rootFrame, outputDir):
    # Build dotfile from parse tree nodes.
    dotPath = os.path.join(outputDir, 'controlFlow.dot')
    dotFile = open(dotPath, 'w')
    dotFile.write('digraph HadronControlFlow {\n  graph [fontname=helvetica];\n  node [fontname=helvetica];\n')
    saveFrame(rootFrame, dotFile)
    # For whatever reason the connections between blocks have to all be specified at highest level scope, or some blocks
    # will end up in the wrong subframes. So we do a separate pass just to enumerate the connections.
    saveBlockGraph(rootFrame, dotFile)
    dotFile.write('}\n')
    dotFile.close()
    # Execute command to generate svg image.
    svgPath = os.path.join(outputDir, 'controlFlow.svg')
    svgFile = open(svgPath, 'w')
    subprocess.run(['dot', '-Tsvg', dotPath], stdout=svgFile)
    outFile.write(
"""
<h3>control flow graph</h3>
<img src="controlFlow.svg">
""")

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
    <tr><td port="falseBlock">falseBlock</td></tr></table>>]
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
    dotFile.write('digraph HadronParseTree {\n  graph [fontname=helvetica];\n  node [fontname=helvetica];\n')
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
        "color: lightgrey;",        #  0: kEmpty
        "color: red;",              #  1: kInterpret
        "color: blue;",             #  2: kLiteral
        "color: green;",            #  3: kPrimitive
        "color: orange;",           #  4: kPlus
        "color: orange;",           #  5: kMinus
        "color: orange;",           #  6: kAsterisk
        "color: red;",              #  7: kAssign
        "color: orange;",           #  8: kLessThan
        "color: orange;",           #  9: kGreaterThan
        "color: orange;",           # 10: kPipe
        "color: orange;",           # 11: kReadWriteVar
        "color: orange;",           # 12: kLeftArrow
        "color: orange;",           # 13: kBinop
        "color: black;",            # 14: kKeyword
        "color: grey;",             # 15: kOpenParen
        "color: grey;",             # 16: kCloseParen
        "color: grey;",             # 17: kOpenCurly
        "color: grey;",             # 18: kCloseCurly
        "color: grey;",             # 19: kOpenSquare
        "color: grey;",             # 20: kCloseSquare
        "color: grey;",             # 21: kComma
        "color: grey;",             # 22: kSemicolon
        "color: grey;",             # 23: kColon
        "color: grey;",             # 24: kCaret
        "color: grey;",             # 25: kTilde
        "color: grey;",             # 26: kHash
        "color: grey;",             # 27: kGrave
        "color: grey;",             # 28: kVar
        "color: grey;",             # 29: kArg
        "color: grey;",             # 30: kConst
        "color: black;",            # 31: kClassVar
        "color: black;",            # 32: kIdentifier
        "color: black;",            # 33: kClassName
        "color: grey;",             # 34: kDot
        "color: grey;",             # 35: kDotDot
        "color: grey;",             # 36: kEllipses
        "color: grey;",             # 37: kCurryArgument
        "color: yellow;",           # 38: kIf
    ]
    if typeIndex >= len(tokenTypeStyles):
        print("bad typeIndex of {} for styleForTokenType".format(typeIndex))
        return "color: grey;"
    return tokenTypeStyles[typeIndex]

def sourceToHTML(source):
    return html.escape(source).replace('\t', '&nbsp;&nbsp;&nbsp;&nbsp').replace(' ', '&nbsp;')

def buildListing(outFile, tokens, source):
    lineNumberPadding = str(len(str(len(source) + 1)))
    outFile.write(
"""
<h3>source</h3>
<div style="font-family: monospace">
""")
    lineNumber = 0
    lineStart = '<span style="color: black;">{:0>' + lineNumberPadding + '}:</span> <span style="{}">'
    charNumber = 0
    for token in tokens:
        # process any source lines before token as text outside of any token
        while lineNumber < token['lineNumber']:
            if charNumber == 0:
                outFile.write('{}{}</span></br>\n'.format(lineStart.format(lineNumber + 1, styleForTokenType(0)),
                    sourceToHTML(source[lineNumber])))
            else:
                outFile.write('<span style="{}">{}</span></br>\n'.format(
                    styleForTokenType(0),
                    sourceToHTML(source[lineNumber][charNumber:])))
            lineNumber += 1
            charNumber = 0

        # process any characters before token starts they are also outside, append them.
        line = ''
        if (charNumber == 0):
            if token['charNumber'] > 0:
                line = lineStart.format(lineNumber + 1, styleForTokenType(0))
                line += sourceToHTML(source[lineNumber][0:token['charNumber']])
                line += '</span><span style="{}">'.format(styleForTokenType(token['tokenType']))
            else:
                line = lineStart.format(lineNumber + 1, styleForTokenType(token['tokenType']))
        else:
            line = '{}<span style="{}">'.format(
                    sourceToHTML(source[lineNumber][charNumber:token['charNumber']]),
                    styleForTokenType(token['tokenType']))

        tokenLength = 0
        charNumber = token['charNumber']
        tokenSource = ''
        while tokenLength < token['length']:
            lineRemain = min(len(source[lineNumber]) - charNumber, token['length'] - tokenLength)
            sourceLine = sourceToHTML(source[lineNumber][charNumber : charNumber + lineRemain])
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
                    line += lineStart.format(lineNumber + 1, styleForTokenType(token['tokenType']))

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
  td.used {{ background-color: grey; color: white }}
  td.live {{ background-color: lightGrey; color: black }}
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
    diagnostics = client.getDiagnostics(args.inputFile)

    for unit in diagnostics['compilationUnits']:
        name = unit['name']
        buildParseTree(outFile, unit['parseTree'], tokens, args.outputDir)
        buildControlFlow(outFile, unit['rootFrame'], args.outputDir)
        buildLinearBlock(outFile, unit['linearBlock'])
        buildMachineCode(outFile, unit['machineCode'])
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