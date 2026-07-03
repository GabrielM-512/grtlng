#pragma once
#include "parser.h"


ExprNode *expression(Parser *parser);
ExprNode *parseExprPrec(Parser *parser);
ExprNode *parseExprPrecRight(Parser *parser);