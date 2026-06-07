#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"
#include <stdbool.h>
#include "ad.h"
#include "at.h"
#include "vm.h" // Avem nevoie de tipul Instr pentru generarea de cod

// Funcția principală de pornire
void parse(Token* tokens);

// Regulile gramaticale, semantice și de generare
bool unit();
bool structDef();
bool fnDef();
bool varDef();
bool typeBase(Type* t);
bool arrayDecl(Type* t);
bool fnParam();
bool stm(Instr** code); // Adăugat: parametru pentru codul generat
bool stmCompound(bool newDomain, Instr** code); // Adăugat: parametru pentru codul generat

// Toate expresiile primesc acum parametrul Instr **code pentru atașarea instrucțiunilor generate
bool expr(Ret* r, Instr** code);
bool exprAssign(Ret* r, Instr** code);
bool exprOr(Ret* r, Instr** code);
bool exprOrPrim(Ret* r, Instr** code);
bool exprAnd(Ret* r, Instr** code);
bool exprAndPrim(Ret* r, Instr** code);
bool exprEq(Ret* r, Instr** code);
bool exprEqPrim(Ret* r, Instr** code);
bool exprRel(Ret* r, Instr** code);
bool exprRelPrim(Ret* r, Instr** code);
bool exprAdd(Ret* r, Instr** code);
bool exprAddPrim(Ret* r, Instr** code);
bool exprMul(Ret* r, Instr** code);
bool exprMulPrim(Ret* r, Instr** code);
bool exprCast(Ret* r, Instr** code);
bool exprUnary(Ret* r, Instr** code);
bool exprPostfix(Ret* r, Instr** code);
bool exprPostfixPrim(Ret* r, Instr** code);
bool exprPrimary(Ret* r, Instr** code);

#endif