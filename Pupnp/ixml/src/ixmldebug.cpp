// Copyright (C) 2022+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2024-10-27
// Taken from authors who haven't made a note.

/*!
 * \file
 */

// #include "upnpconfig.h"

#include "ixmldebug.hpp"

#include <stdarg.h>
#include <stdio.h>
// #include <umock/stdio.hpp>

#ifdef DEBUG
void IxmlPrintf(const char* DbgFileName, int DbgLineNo,
                const char* FunctionName, const char* FmtStr, ...) {
    va_list ArgList;

    FILE* fp = stdout;
    fprintf(fp, "(%s::%s), line %d", DbgFileName, FunctionName, DbgLineNo);
    if (FmtStr) {
        fprintf(fp, ": ");
        va_start(ArgList, FmtStr);
        vfprintf(fp, FmtStr, ArgList);
        // umock::stdio_h.fflush(fp);
        fflush(fp);
        va_end(ArgList);
    } else {
        fprintf(fp, "\n");
    }
}

void printNodes(IXML_Node* tmpRoot, int depth) {
    unsigned long i;
    IXML_NodeList* NodeList1;
    IXML_Node* ChildNode1;
    unsigned short NodeType;
    const DOMString NodeValue;
    const DOMString NodeName;
    NodeList1 = ixmlNode_getChildNodes(tmpRoot);
    for (i = 0; i < 100; ++i) {
        ChildNode1 = ixmlNodeList_item(NodeList1, i);
        if (ChildNode1 == NULL) {
            break;
        }

        printNodes(ChildNode1, depth + 1);
        NodeType = ixmlNode_getNodeType(ChildNode1);
        NodeValue = ixmlNode_getNodeValue(ChildNode1);
        NodeName = ixmlNode_getNodeName(ChildNode1);
        IxmlPrintf(__FILE__, __LINE__, "printNodes",
                   "DEPTH-%2d-IXML_Node Type %d, "
                   "IXML_Node Name: %s, IXML_Node Value: %s\n",
                   depth, NodeType, NodeName, NodeValue);
    }
}

#endif
