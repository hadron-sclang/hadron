#!/usr/bin/python3

import HLangDClient

import argparse
import html
import os
import pathlib
import subprocess

def reg(reg):
    if reg == -1:
        return 'SP'
    elif reg == -2:
        return 'FP'
    elif reg == -3:
        return 'CONTEXT'
    return 'R{}'.format(reg)

def buildMachineCode(outFile, machineCode):
    outFile.write('\n<h3>debug machine code</h3>\n<span style="font-family: monospace">')
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
        elif inst[1] == 'movi_u':
            outFile.write('movi_u {}, 0x{:016x}<br>\n'.format(reg(inst[2]), inst[3]))
        elif inst[1] == 'bgei':
            outFile.write('bgei {}, {}, 0x{:08x}<br>\n'.format(reg(inst[2]), inst[3], inst[4]))
        elif inst[1] == 'beqi':
            outFile.write('beqi {}, {}, 0x{:08x}<br>\n'.format(reg(inst[2]), inst[3], inst[4]))
        elif inst[1] == 'jmp':
            outFile.write('jmp 0x{:08x}<br>\n'.format(inst[2]))
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

def slotToString(slot):
    if 'value' in slot:
        if slot['type'] == 'symbol':
            return "'{}'".format(sourceToHTML(slot['value']['string']))
        return str(slot['value'])
    return slot['type']

def vRegToString(lir):
    flags = ' | '.join(lir['typeFlags'])
    return '({}) VR{}'.format(flags, lir['value'])

def lirToString(lir):
    if lir['opcode'] == 'Branch':
        return 'Branch to Label {}'.format(lir['labelId'])
    elif lir['opcode'] == 'BranchIfTrue':
        return 'BranchIfTrue {} to Label {}'.format(lir['condition'], lir['labelId'])
    elif lir['opcode'] == 'BranchToRegister':
        return 'BranchToRegister VR{}'.format(lir['address'])
    elif lir['opcode'] == 'Label':
        # Labels get their own column for readability.
        return ''
    elif lir['opcode'] == 'LoadConstant':
        return '{} &#8592; {}'.format(vRegToString(lir), slotToString(lir['constant']))
    elif lir['opcode'] == 'LoadFromPointer':
        return '{} &#8592; *(VR{} + {})'.format(vRegToString(lir), lir['pointer'],
                lir['offset'])
    elif lir['opcode'] == 'LoadFromStack':
        if lir['useFramePointer']:
            return '{} &#8592; *(FP + {})'.format(vRegToString(lir), lir['offset'])
        else:
            return '{} &#8592; *(SP + {})'.format(vRegToString(lir), lir['offset'])
    elif lir['opcode'] == 'Phi':
        phi = '{} &#8592; &phi;('.format(vRegToString(lir))
        phi += ','.join(['VR{}'.format(x) for x in lir['inputs']])
        phi += ')'
        return phi
    elif lir['opcode'] == 'StoreToPointer':
        return '*(VR{} + {}) &#8592; {}'.format(lir['pointer'], lir['offset'], lir['toStore'])
    elif lir['opcode'] == 'StoreToStack':
        if lir['useFramePointer']:
            return '*(FP + {}) &#8592; {}'.format(lir['offset'], lir['toStore'])
        else:
            return '*(SP + {}) &#8592; {}'.format(lir['offset'], lir['toStore'])
    else:
        return 'unsupported lir opcode "{}"'.format(lir['opcode'])

def buildLinearFrame(outFile, linearFrame):
    outFile.write('\n<h3>linear frame</h3>\n\n<table>\n<tr><th>line</th><th>label</th><th>lir</th>')
    for i in range(0, len(linearFrame['valueLifetimes'])):
        outFile.write('<th>VR{}</th>'.format(i))
    outFile.write('</tr>\n')
    for i in range(0, len(linearFrame['instructions'])):
        outFile.write('<tr><td align="right">{}</td>'.format(i))
        inst = linearFrame['instructions'][i]
        if inst['opcode'] == 'Label':
            outFile.write('<td>label_{}:</td><td>'.format(inst['id']))
            for phi in inst['phis']:
                outFile.write('{}<br/>'.format(lirToString(phi)))
            outFile.write('</td>')
        else:
            outFile.write('<td></td><td>{}</td>'.format(lirToString(inst)))
        # want to visualize "ranges" and "usages" for each of the lifetime intervals. In value lifetimes also useful
        # to see what register they are associated with, and in register lifetimes what value, and in spill what value.
        for j in range(0, len(linearFrame['valueLifetimes'])):
            lifetime = findContainingLifetime(i, linearFrame['valueLifetimes'][j])
            if lifetime:
                tdClass = 'live'
                if i in lifetime['usages']:
                    tdClass = 'used'
                outFile.write('<td class="{}">R{}</td>'.format(tdClass, lifetime['registerNumber']))
            else:
                outFile.write('<td></td>')
        outFile.write('</tr>')
    outFile.write('\n</table>\n')

def valueToString(value):
    flags = ' | '.join(value['typeFlags'])
    if 'name' in value:
        return '({}) {}_{}'.format(flags, value['name'], value['id'])
    return '({}) _{}'.format(flags, value['id'])

def hirToString(hir):
    if hir['opcode'] == 'Branch':
        return 'Branch to Block {}'.format(hir['blockId'])
    elif hir['opcode'] == 'BranchIfTrue':
        return 'BranchIfTrue {} to Block {}'.format(valueToString(hir['condition']), hir['blockId'])
    elif hir['opcode'] == 'Constant':
        return '{} &#8592; {}'.format(valueToString(hir['value']), slotToString(hir['constant']))
    elif hir['opcode'] == 'ImportClassVariable':
        return '{} &#8592; ImportClassVar({})'.format(valueToString(hir['value']), hir['offset'])
    elif hir['opcode'] == 'ImportInstanceVariable':
        return '{} &#8592; ImportInstanceVar(this: {}, off: {})'.format(valueToString(hir['value']), hir['thisId'],
                hir['offset'])
    elif hir['opcode'] == 'ImportLocalVariable':
        # Re-using the name of the local value as the name of the imported value.
        return '{} &#8592; ImportLocalVar({}_{})'.format(valueToString(hir['value']), hir['value']['name'],
                hir['externalId'])
    elif hir['opcode'] == 'LoadArgument':
        return '{} &#8592; LoadArgument({})'.format(valueToString(hir['value']), hir['argIndex'])
    elif hir['opcode'] == 'Message':
        message = '{} &#8592; {}.{}('.format(valueToString(hir['value']), valueToString(hir['arguments'][0]),
                sourceToHTML(hir['selector']['string']))
        message += ','.join(valueToString(hir['arguments'][x]) for x in range(1, len(hir['arguments'])))
        message += ','.join('{}: {}'.format(valueToString(hir['keywordArguments'][x]),
                valueToString(hir ['keywordArguments'][x + 1])) for x in range(0, len(hir['keywordArguments']), 2))
        message += ')'
        return message
    elif hir['opcode'] == 'MethodReturn':
        return 'return'
    elif hir['opcode'] == 'Phi':
        phi = '{} &#8592; &phi;('.format(valueToString(hir['value']))
        phi += ','.join(['{}'.format(x['id']) for x in hir['inputs']])
        phi += ')'
        return phi
    elif hir['opcode'] == 'StoreReturn':
        return 'StoreReturn({})'.format(valueToString(hir['returnValue']))
    return 'unsupported hir opcode "{}"'.format(hir['opcode'])

def saveScope(scope, dotFile, prefix):
    dotFile.write('  subgraph cluster_{}_{} {{\n'.format(prefix, scope['scopeSerial']))
    subFrames = []
    for block in scope['blocks']:
        dotFile.write("""
    block_{}_{} [shape=plain label=<<table border="0" cellpadding="6" cellborder="1" cellspacing="0">
      <tr><td bgcolor="lightGray"><b>Block {}</b></td></tr>
""".format(prefix, block['id'], block['id']))
        for phi in block['phis']:
            dotFile.write('      <tr><td>{}</td></tr>\n'.format(hirToString(phi)))
        for hir in block['statements']:
            if hir['opcode'] == 'BlockLiteral':
                pfx = prefix + str(hir['value']['id'])
                subFrames.append((pfx, hir['frame']))
                dotFile.write('      <tr><td port="port_{}">{} &#8592; BlockLiteral</td></tr>\n'.format(pfx,
                        valueToString(hir['value'])))
            else:
                dotFile.write('      <tr><td>{}</td></tr>\n'.format(hirToString(hir)))
        dotFile.write('      </table>>]\n')
    for subScope in scope['subScopes']:
        saveScope(subScope, dotFile, prefix)
    for (pfx, sf) in subFrames:
        saveScope(sf['rootScope'], dotFile, pfx)
    dotFile.write('  }} // end of cluster_{}_{}\n'.format(prefix, scope['scopeSerial']))

def saveBlockGraph(scope, dotFile, prefix):
    for block in scope['blocks']:
        for hir in block['statements']:
            if hir['opcode'] == 'BlockLiteral':
                pfx = prefix + str(hir['value']['id'])
                dotFile.write('    block_{}_{}:port_{} -> block_{}_0 [style="dashed"]\n'.format(prefix, block['id'],
                        pfx, pfx))
                saveBlockGraph(hir['frame']['rootScope'], dotFile, pfx)
        for succ in block['successors']:
            dotFile.write('    block_{}_{} -> block_{}_{}\n'.format(prefix, block['id'], prefix, succ))
    for subFrame in scope['subScopes']:
        saveBlockGraph(subFrame, dotFile, prefix)

def buildControlFlow(outFile, rootFrame, outputDir, name, num):
    # Build dotfile from parse tree nodes.
    dotPath = os.path.join(outputDir, 'controlFlow_' + num + '.dot')
    dotFile = open(dotPath, 'w')
    dotFile.write('digraph HadronControlFlow {\n  graph [fontname=helvetica];\n  node [fontname=helvetica];\n')
    saveScope(rootFrame['rootScope'], dotFile, '')
    # For whatever reason the connections between blocks have to all be specified at highest level scope, or some blocks
    # will end up in the wrong subframes. So we do a separate pass just to enumerate the connections.
    saveBlockGraph(rootFrame['rootScope'], dotFile, '')
    dotFile.write('}\n')
    dotFile.close()
    # Execute command to generate svg image.
    svgPath = os.path.join(outputDir, 'controlFlow_' + num + '.svg')
    svgFile = open(svgPath, 'w')
    subprocess.run(['dot', '-Tsvg', dotPath], stdout=svgFile)
    outFile.write(
"""
<h3>{} control flow graph</h3>
<img src="{}">
""".format(name, 'controlFlow_' + num + '.svg'))

def saveNode(node, tokens, dotFile):
    if 'nodeType' not in node:
        print(node)

    dotFile.write("""  node_{} [shape=plain label=<<table border="0" cellborder="1" cellspacing="0">
    <tr><td bgcolor="lightGray"><b>{}</b></td></tr>
    <tr><td><font face="monospace">{}</font></td></tr>
    <tr><td port="next">next</td></tr>
""".format(node['serial'], node['nodeType'], tokens[node['tokenIndex']]['source']))

    # Empty
    if node['nodeType'] == 'Empty':
        dotFile.write('    </table>>]\n')

    # VarDef
    elif node['nodeType'] == 'VarDef':
        dotFile.write("""    <tr><td>hasReadAccessor: {}</td></tr>
    <tr><td>hasWriteAccessor: {}</td></tr>
    <tr><td port="initialValue">initialValue</td></tr></table>>]
""".format(node['hasReadAccessor'], node['hasWriteAccessor']))
        if 'initialValue' in node:
            dotFile.write('  node_{}:initialValue -> node_{}\n'.format(node['serial'], node['initialValue']['serial']))
            saveNode(node['initialValue'], tokens, dotFile)

    # VarList
    elif node['nodeType'] == 'VarList':
        dotFile.write('    <tr><td port="definitions">definitions</td></tr></table>>]\n')
        if node['definitions']:
            dotFile.write('  node_{}:definitions -> node_{}\n'.format(node['serial'], node['definitions']['serial']))
            saveNode(node['definitions'], tokens, dotFile)

    # ArgList
    elif node['nodeType'] == 'ArgList':
        dotFile.write('    <tr><td port="varList">varList</td></tr></table>>]\n')
        if node['varList']:
            dotFile.write('  node_{}:varList -> node_{}\n'.format(node['serial'], node['varList']['serial']))
            saveNode(node['varList'], tokens, dotFile)

    # Method
    elif node['nodeType'] == 'Method':
        dotFile.write("""    <tr><td>isClassMethod: {}</td></tr>
    <tr><td port="body">body</td></tr></table>>]
""".format(node['isClassMethod']))
        if node['body']:
            dotFile.write('  node_{}:body -> node_{}\n'.format(node['serial'], node['body']['serial']))
            saveNode(node['body'], tokens, dotFile)

    # ClassExt
    elif node['nodeType'] == 'ClassExt':
        dotFile.write('    <tr><td port="methods">methods</td></tr></table>>]\n')
        if node['methods']:
            dotFile.write('  node_{}:methods -> node_{}\n'.format(node['serial'], node['methods']['serial']))
            saveNode(node['methods'], tokens, dotFile)

    # Class
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

    # ReturnNode
    elif node['nodeType'] == 'ReturnNode':
        dotFile.write('    <tr><td port="valueExpr">valueExpr</td></tr></table>>]\n')
        if node['valueExpr']:
            dotFile.write('  node_{}:valueExpr -> node_{}\n'.format(node['serial'], node['valueExpr']['serial']))
            saveNode(node['valueExpr'], tokens, dotFile)

    # List
    elif node['nodeType'] == 'List':
        dotFile.write('    <tr><td port="elements">elements</td></tr></table>>]\n')
        if 'elements' in node:
            dotFile.write('  node_{}:elements -> node_{}\n'.format(node['serial'], node['elements']['serial']))
            saveNode(node['elements'], tokens, dotFile)

    # Block
    elif node['nodeType'] == 'Block':
        dotFile.write("""    <tr><td port="arguments">arguments</td></tr>
    <tr><td port="variables">variables</td></tr>
    <tr><td port="body">body</td></tr></table>>]
""")
        if 'arguments' in node:
            dotFile.write('  node_{}:arguments -> node_{}\n'.format(node['serial'], node['arguments']['serial']))
            saveNode(node['arguments'], tokens, dotFile)
        if 'variables' in node:
            dotFile.write('  node_{}:variables -> node_{}\n'.format(node['serial'], node['variables']['serial']))
            saveNode(node['variables'], tokens, dotFile)
        if 'body' in node:
            dotFile.write('  node_{}:body -> node_{}\n'.format(node['serial'], node['body']['serial']))
            saveNode(node['body'], tokens, dotFile)

    # Literal
    elif node['nodeType'] == 'Literal':
        if 'blockLiteral' in node:
            dotFile.write('    <tr><td port="blockLiteral">blockLiteral</td></tr></table>>]\n')
            dotFile.write('  node_{}:blockLiteral -> node_{}\n'.format(node['serial'], node['blockLiteral']['serial']))
            saveNode(node['blockLiteral'], tokens, dotFile)
        else:
            dotFile.write('    </table>>]\n')

    # Name
    elif node['nodeType'] == 'Name':
        dotFile.write('    <tr><td>isGlobal: {}</td></tr></table>>]\n'.format(node['isGlobal']))

    # ExprSeq
    elif node['nodeType'] == 'ExprSeq':
        dotFile.write('    <tr><td port="expr">expr</td></tr></table>>]\n')
        if 'expr' in node:
            dotFile.write('  node_{}:expr -> node_{}\n'.format(node['serial'], node['expr']['serial']))
            saveNode(node['expr'], tokens, dotFile)

    # Assign
    elif node['nodeType'] == 'Assign':
        dotFile.write("""    <tr><td port="name">name</td></tr>
    <tr><td port="value">value</td></tr></table>>]
""")
        if 'name' in node:
            dotFile.write('  node_{}:name -> node_{}\n'.format(node['serial'], node['name']['serial']))
            saveNode(node['name'], tokens, dotFile)
        if 'value' in node:
            dotFile.write('  node_{}:value -> node_{}\n'.format(node['serial'], node['value']['serial']))
            saveNode(node['value'], tokens, dotFile)

    # Setter
    elif node['nodeType'] == 'Setter':
        dotFile.write("""    <tr><td port="target">target</td></tr>
    <tr><td port="value">value</td></tr></table>>]
""")
        if 'target' in node:
            dotFile.write('  node_{}:target -> node_{}\n'.format(node['serial'], node['target']['serial']))
            saveNode(node['target'], tokens, dotFile)
        if 'value' in node:
            dotFile.write('  node_{}:value -> node_{}\n'.format(node['serial'], node['value']['serial']))
            saveNode(node['value'], tokens, dotFile)

    # Array Read
    elif node['nodeType'] == 'ArrayRead':
        dotFile.write("""    <tr><td port="targetArray">targetArray</td></tr>
    <tr><td port="indexArgument">indexArgument</td></tr></table>>]
""")
        if 'targetArray' in node:
            dotFile.write('  node_{}:targetArray -> node_{}\n'.format(node['serial'], node['targetArray']['serial']))
            saveNode(node['targetArray'], tokens, dotFile)
        if 'indexArgument' in node:
            dotFile.write('  node_{}:indexArgument -> node_{}\n'.format(node['serial'],
                    node['indexArgument']['serial']))
            saveNode(node['indexArgument'], tokens, dotFile)

    # Array Write
    elif node['nodeType'] == 'ArrayWrite':
        dotFile.write("""    <tr><td port="targetArray">targetArray</td></tr>
    <tr><td port="indexArgument">indexArgument</td></tr>
    <tr><td port="value">value</td></tr></table>>]
""")
        if 'targetArray' in node:
            dotFile.write('  node_{}:targetArray -> node_{}\n'.format(node['serial'], node['targetArray']['serial']))
            saveNode(node['targetArray'], tokens, dotFile)
        if 'indexArgument' in node:
            dotFile.write('  node_{}:indexArgument -> node_{}\n'.format(node['serial'],
                    node['indexArgument']['serial']))
            saveNode(node['indexArgument'], tokens, dotFile)
        if 'value' in node:
            dotFile.write('  node_{}:value -> node_{}\n'.format(node['serial'], node['value']['serial']))
            saveNode(node['value'], tokens, dotFile)

    # Call
    elif node['nodeType'] == 'Call':
        dotFile.write("""    <tr><td port="target">target</td></tr>
    <tr><td port="arguments">arguments</td></tr>
    <tr><td port="keywordArguments">keywordArguments</td></tr></table>>]
""")
        if 'target' in node:
            dotFile.write('  node_{}:target -> node_{}\n'.format(node['serial'], node['target']['serial']))
            saveNode(node['target'], tokens, dotFile)
        if 'arguments' in node:
            dotFile.write('  node_{}:arguments -> node_{}\n'.format(node['serial'], node['arguments']['serial']))
            saveNode(node['arguments'], tokens, dotFile)
        if 'keywordArguments' in node:
            dotFile.write('  node_{}:keywordArguments -> node_{}\n'.format(node['serial'],
                node['keywordArguments']['serial']))
            saveNode(node['keywordArguments'], tokens, dotFile)

    # BinopCall
    elif node['nodeType'] == 'BinopCall':
        dotFile.write("""    <tr><td port="leftHand">leftHand</td></tr>
    <tr><td port="rightHand">rightHand</td></tr>
    <tr><td port="adverb">adverb</td></tr></table>>]
""")
        if 'leftHand' in node:
            dotFile.write('  node_{}:leftHand -> node_{}\n'.format(node['serial'], node['leftHand']['serial']))
            saveNode(node['leftHand'], tokens, dotFile)
        if 'rightHand' in node:
            dotFile.write('  node_{}:rightHand -> node_{}\n'.format(node['serial'], node['rightHand']['serial']))
            saveNode(node['rightHand'], tokens, dotFile)
        if 'adverb' in node:
            dotFile.write('  node_{}:adverb -> node_{}\n'.format(node['serial'], node['adverb']['serial']))
            saveNode(node['adverb'], tokens, dotFile)

    # New
    elif node['nodeType'] == 'New':
        dotFile.write("""    <tr><td port="target">target</td></tr>
    <tr><td port="arguments">arguments</td></tr>
    <tr><td port="keywordArguments">keywordArguments</td></tr></table>>]
""")
        if 'target' in node:
            dotFile.write('  node_{}:target -> node_{}\n'.format(node['serial'], node['target']['serial']))
            saveNode(node['target'], tokens, dotFile)
        if 'arguments' in node:
            dotFile.write('  node_{}:arguments -> node_{}\n'.format(node['serial'], node['arguments']['serial']))
            saveNode(node['arguments'], tokens, dotFile)
        if 'keywordArguments' in node:
            dotFile.write('  node_{}:keywordArguments -> node_{}\n'.format(node['serial'],
                node['keywordArguments']['serial']))
            saveNode(node['keywordArguments'], tokens, dotFile)

    # If
    elif node['nodeType'] == 'If':
        dotFile.write("""    <tr><td port="condition">condition</td></tr>
    <tr><td port="trueBlock">trueBlock</td></tr>
    <tr><td port="falseBlock">falseBlock</td></tr></table>>]
""")
        if 'condition' in node:
            dotFile.write('  node_{}:condition -> node_{}\n'.format(node['serial'], node['condition']['serial']))
            saveNode(node['condition'], tokens, dotFile)
        if 'trueBlock' in node:
            dotFile.write('  node_{}:trueBlock -> node_{}\n'.format(node['serial'], node['trueBlock']['serial']))
            saveNode(node['trueBlock'], tokens, dotFile)
        if 'falseBlock' in node:
            dotFile.write('  node_{}:falseBlock -> node_{}\n'.format(node['serial'], node['falseBlock']['serial']))
            saveNode(node['falseBlock'], tokens, dotFile)

    if 'next' in node:
        dotFile.write('  node_{}:next -> node_{}\n'.format(node['serial'], node['next']['serial']))
        saveNode(node['next'], tokens, dotFile)

def buildParseTree(outFile, parseTree, tokens, outputDir, name, num):
    # Build dotfile from parse tree nodes.
    dotPath = os.path.join(outputDir, 'parseTree_' + num + '.dot')
    dotFile = open(dotPath, 'w')
    dotFile.write('digraph HadronParseTree {\n  graph [fontname=helvetica];\n  node [fontname=helvetica];\n')
    saveNode(parseTree, tokens, dotFile)
    dotFile.write('}\n')
    dotFile.close()
    # Execute command to generate svg image.
    svgPath = os.path.join(outputDir, 'parseTree_' + num + '.svg')
    svgFile = open(svgPath, 'w')
    # print('processing {} to {}'.format(dotPath, svgPath))
    subprocess.run(['dot', '-Tsvg', dotPath], stdout=svgFile)
    outFile.write("""
<h3>{} parse tree</h3>
<img src="{}">
""".format(name, 'parseTree_' + num + '.svg'))

def saveAST(ast, dotFile):
    # Sequences need special handling for the table, so check for them first.
    if ast['astType'] == 'Sequence':
        if 'sequence' in ast:
            colspan = max(1, len(ast['sequence']))
            dotFile.write("""  ast_{} [shape=plain label=<<table border="0" cellborder="1" cellspacing="0">
    <tr><td colspan="{}" bgcolor="lightGray"><b>{}</b></td></tr>
    <tr>""".format(ast['serial'], colspan, ast['astType']))
            for i in range(0, len(ast['sequence'])):
                dotFile.write('<td port="seq_{}">&nbsp;</td>'.format(i))
            dotFile.write('</tr></table>>]\n')
            i = 0
            for seq in ast['sequence']:
                dotFile.write('  ast_{}:seq_{} -> ast_{}\n'.format(ast['serial'], i, seq['serial']))
                i += 1
                saveAST(seq, dotFile)
        else:
            dotFile.write("""  ast_{} [shape=plain label=<<table border="0" cellborder="1" cellspacing="0">
    <tr><td bgcolor="lightGray"><b>{}</b></td></tr></table>>]
""".format(ast['serial'], ast['astType']))
        return

    dotFile.write("""  ast_{} [shape=plain label=<<table border="0" cellborder="1" cellspacing="0">
    <tr><td bgcolor="lightGray"><b>{}</b></td></tr>
""".format(ast['serial'], ast['astType']))

    # Assign
    if ast['astType'] == 'Assign':
        dotFile.write("""    <tr><td port="name">name</td></tr>
    <tr><td port="value">value</td></tr></table>>]
  ast_{}:name -> ast_{}
  ast_{}:value -> ast_{}
""".format(ast['serial'], ast['name']['serial'], ast['serial'], ast['value']['serial']))
        saveAST(ast['name'], dotFile)
        saveAST(ast['value'], dotFile)

    # Block
    elif ast['astType'] == 'Block':
        dotFile.write("""    <tr><td port="statements">statements</td></tr></table>>]
  ast_{}:statements -> ast_{}
""".format(ast['serial'], ast['statements']['serial']))
        saveAST(ast['statements'], dotFile)

    # Constant
    elif ast['astType'] == 'Constant':
        dotFile.write('    <tr><td>value: {}</td></tr></table>>]\n'.format(slotToString(ast['constant'])))

    # Define
    elif ast['astType'] == 'Define':
        dotFile.write("""    <tr><td port="name">name</td></tr>
    <tr><td port="value">value</td></tr></table>>]
  ast_{}:name -> ast_{}
  ast_{}:value -> ast_{}
""".format(ast['serial'], ast['name']['serial'], ast['serial'], ast['value']['serial']))
        saveAST(ast['name'], dotFile)
        saveAST(ast['value'], dotFile)

    # If
    elif ast['astType'] == 'If':
        dotFile.write("""    <tr><td port="condition">condition</td></tr>
    <tr><td port="trueBlock">trueBlock</td></tr>
    <tr><td port="falseBlock">falseBlock</td></tr></table>>]
  ast_{}:condition -> ast_{}
  ast_{}:trueBlock -> ast_{}
  ast_{}:falseBlock -> ast_{}
""".format(ast['serial'], ast['condition']['serial'], ast['serial'], ast['trueBlock']['serial'], ast['serial'],
            ast['falseBlock']['serial']))
        saveAST(ast['condition'], dotFile)
        saveAST(ast['trueBlock'], dotFile)
        saveAST(ast['falseBlock'], dotFile)

    # Message
    elif ast['astType'] == 'Message':
        dotFile.write("""    <tr><td>selector: {}</td></tr>
    <tr><td port="arguments">arguments</td></tr>
    <tr><td port="keywordArguments">keywordArguments</td></tr></table>>]
  ast_{}:arguments -> ast_{}
""".format(sourceToHTML(ast['selector']['string']), ast['serial'], ast['arguments']['serial']))
        saveAST(ast['arguments'], dotFile)
        if 'sequence' in ast['keywordArguments']:
            dotFile.write('  ast_{}:keywordArguments -> ast_{}'.format(ast['serial'],
                    ast['keywordArguments']['serial']))
            saveAST(ast['keywordArguments'], dotFile)

    # MethodReturn
    elif ast['astType'] == 'MethodReturn':
        dotFile.write("""    <tr><td port="value">value</td></tr></table>>]
  ast_{}:value -> ast_{}
""".format(ast['serial'], ast['value']['serial']))
        saveAST(ast['value'], dotFile)

    # Name
    elif ast['astType'] == 'Name':
        dotFile.write('    <tr><td>name: {}</td></tr></table>>]\n'.format(ast['name']['string']))

    else:
        dotFile.write('    </table>>]\n')

def buildAST(outFile, ast, outputDir, name, num):
    dotPath = os.path.join(outputDir, 'ast_' + num + '.dot')
    dotFile = open(dotPath, 'w')
    dotFile.write('digraph HadronAST {\n  graph [fontname=helvetica];\n  node [fontname=helvetica];\n')
    saveAST(ast, dotFile)
    dotFile.write('}\n')
    dotFile.close()
    svgPath = os.path.join(outputDir, 'ast_' + num + '.svg')
    svgFile = open(svgPath, 'w')
    # print('processing {} to {}'.format(dotPath, svgPath))
    subprocess.run(['dot', '-Tsvg', dotPath], stdout=svgFile)
    outFile.write(
"""
<h3>{} abstract syntax tree</h3>
<img src="{}">
""".format(name, 'ast_' + num + '.svg'))

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
    es = html.escape(source)
    es = es.replace('\t', '&nbsp;&nbsp;&nbsp;&nbsp;')
    es = es.replace(' ', '&nbsp;')
    es = es.replace('{', '&#123;')
    es = es.replace('[', '&#91;')
    return es

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
    path = pathlib.Path(args.outputDir)
    path.mkdir(parents=True, exist_ok=True)
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
    diagnostics = client.getDiagnostics(args.inputFile, args.stopAfter)
    unitNum = 0
    for unit in diagnostics['compilationUnits']:
        name = unit['name']
        num = str(unitNum)
        unitNum += 1
        outFile.write("<h2>{}</h2>\n".format(name))
        buildParseTree(outFile, unit['parseTree'], tokens, args.outputDir, name, num)
        buildAST(outFile, unit['ast'], args.outputDir, name, num)

        if 'rootFrame' in unit:
            buildControlFlow(outFile, unit['rootFrame'], args.outputDir, name, num)

        if 'linearFrame' in unit:
            buildLinearFrame(outFile, unit['linearFrame'])

        if 'machineCode' in unit:
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
    parser.add_argument('--stopAfter', help='stop compilation after phase, a number from 1-7', default=7)
    args = parser.parse_args()
    main(args)