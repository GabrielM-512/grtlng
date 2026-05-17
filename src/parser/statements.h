#pragma once
#include "parser.h"

StmtNode *parseStmt(Parser *parser);
StmtNode *blockStmt(Parser *parser);