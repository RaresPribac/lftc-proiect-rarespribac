#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"
#include <stdbool.h>
#include "ad.h"
#include "at.h" // Adăugat: avem nevoie de definirea structurii Ret pentru analiza de tipuri [cite: 217]

// Funcția principală de pornire
void parse(Token* tokens);

// Regulile gramaticale și semantice
bool unit();
bool structDef();
bool fnDef();
bool varDef();
bool typeBase(Type* t);
bool arrayDecl(Type* t);
bool fnParam();
bool stm();
bool stmCompound(bool newDomain);

// Actualizat: Toate expresiile primesc acum un pointer la Ret pentru analiza de tipuri 
bool expr(Ret* r);
bool exprAssign(Ret* r);
bool exprOr(Ret* r);
bool exprOrPrim(Ret* r);
bool exprAnd(Ret* r);
bool exprAndPrim(Ret* r);
bool exprEq(Ret* r);
bool exprEqPrim(Ret* r);
bool exprRel(Ret* r);
bool exprRelPrim(Ret* r);
bool exprAdd(Ret* r);
bool exprAddPrim(Ret* r);
bool exprMul(Ret* r);
bool exprMulPrim(Ret* r);
bool exprCast(Ret* r);
bool exprUnary(Ret* r);
bool exprPostfix(Ret* r);
bool exprPostfixPrim(Ret* r);
bool exprPrimary(Ret* r);

#endif