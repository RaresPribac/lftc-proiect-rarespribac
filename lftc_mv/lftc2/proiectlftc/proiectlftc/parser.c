#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>
#include "parser.h"
#include "ad.h"
#include "at.h" // Includem analiza de tipuri

Symbol* owner = NULL;
Token* iTk;
Token* consumedTk = NULL;

void tkerr(const char* fmt, ...) {
    fprintf(stderr, "error in line %d: ", iTk->line);
    va_list va;
    va_start(va, fmt);
    vfprintf(stderr, fmt, va);
    va_end(va);
    fprintf(stderr, "\n");
    exit(EXIT_FAILURE);
}

bool consume(int code) {
    if (iTk->code == code) {
        consumedTk = iTk;
        iTk = iTk->next;
        return true;
    }
    return false;
}

// Prototipuri actualizate pentru analiza de tipuri
bool typeBase(Type* t);
bool arrayDecl(Type* t);
bool varDef();
bool structDef();
bool fnDef();
bool fnParam();
bool stm();
bool stmCompound(bool newDomain);
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

bool typeBase(Type* t) {
    t->n = -1;
    if (consume(TYPE_INT)) {
        t->tb = TB_INT;
        return true;
    }
    if (consume(TYPE_DOUBLE)) {
        t->tb = TB_DOUBLE;
        return true;
    }
    if (consume(TYPE_CHAR)) {
        t->tb = TB_CHAR;
        return true;
    }
    if (consume(STRUCT)) {
        if (consume(ID)) {
            Token* tkName = consumedTk;
            t->tb = TB_STRUCT;
            t->s = findSymbol(tkName->text);
            if (!t->s) tkerr("structura nedefinita: %s", tkName->text);
            return true;
        }
        else tkerr("lipseste numele structurii");
    }
    return false;
}

bool arrayDecl(Type* t) {
    if (consume(LBRACKET)) {
        if (consume(INT)) {
            Token* tkSize = consumedTk;
            t->n = tkSize->i;
        }
        else {
            t->n = 0;
        }
        if (consume(RBRACKET)) {
            return true;
        }
        else tkerr("lipseste ] din declararea vectorului");
    }
    return false;
}

bool varDef() {
    Type t;
    if (typeBase(&t)) {
        if (consume(ID)) {
            Token* tkName = consumedTk;
            if (arrayDecl(&t)) {
                if (t.n == 0) tkerr("a vector variable must have a specified dimension");
            }
            if (consume(SEMICOLON)) {
                Symbol* var = findSymbolInDomain(symTable, tkName->text);
                if (var) tkerr("symbol redefinition: %s", tkName->text);

                var = newSymbol(tkName->text, SK_VAR);
                var->type = t;
                var->owner = owner;

                addSymbolToDomain(symTable, var);

                if (owner) {
                    switch (owner->kind) {
                    case SK_FN:
                        var->varIdx = symbolsLen(owner->fn.locals);
                        addSymbolToList(&owner->fn.locals, dupSymbol(var));
                        break;
                    case SK_STRUCT:
                        var->varIdx = typeSize(&owner->type);
                        addSymbolToList(&owner->structMembers, dupSymbol(var));
                        break;
                    default: break;
                    }
                }
                else {
                    var->varMem = safeAlloc(typeSize(&t));
                }
                return true;
            }
            else tkerr("lipseste ; la declararea variabilei");
        }
        else tkerr("lipseste numele variabilei");
    }
    return false;
}

bool structDef() {
    if (consume(STRUCT)) {
        if (consume(ID)) {
            Token* tkName = consumedTk;
            if (consume(LACC)) {
                Symbol* s = findSymbolInDomain(symTable, tkName->text);
                if (s) tkerr("symbol redefinition: %s", tkName->text);
                s = addSymbolToDomain(symTable, newSymbol(tkName->text, SK_STRUCT));
                s->type.tb = TB_STRUCT;
                s->type.s = s;
                s->type.n = -1;
                pushDomain();
                owner = s;

                while (varDef());

                if (consume(RACC)) {
                    if (consume(SEMICOLON)) {
                        owner = NULL;
                        dropDomain();
                        return true;
                    }
                    else tkerr("lipseste ; dupa STRUCT");
                }
                else tkerr("lipseste } la STRUCT");
            }
        }
    }
    return false;
}

bool fnParam() {
    Type t;
    if (typeBase(&t)) {
        if (consume(ID)) {
            Token* tkName = consumedTk;
            if (arrayDecl(&t)) {
                t.n = 0;
            }

            Symbol* param = findSymbolInDomain(symTable, tkName->text);
            if (param) tkerr("symbol redefinition: %s", tkName->text);

            param = newSymbol(tkName->text, SK_PARAM);
            param->type = t;
            param->owner = owner;
            param->paramIdx = symbolsLen(owner->fn.params);

            addSymbolToDomain(symTable, param);
            addSymbolToList(&owner->fn.params, dupSymbol(param));
            return true;
        }
        else tkerr("Lipseste numele parametrului");
    }
    return false;
}

bool fnDef() {
    Type t;
    Token* start = iTk;
    if (typeBase(&t) || consume(VOID)) {
        if (consumedTk->code == VOID) t.tb = TB_VOID;
        if (consume(ID)) {
            Token* tkName = consumedTk;
            if (consume(LPAR)) {
                Symbol* fn = findSymbolInDomain(symTable, tkName->text);
                if (fn) tkerr("symbol redefinition: %s", tkName->text);
                fn = newSymbol(tkName->text, SK_FN);
                fn->type = t;
                addSymbolToDomain(symTable, fn);
                owner = fn;
                pushDomain();

                if (fnParam()) {
                    while (consume(COMMA)) {
                        if (!fnParam()) tkerr("lipseste parametru dupa ,");
                    }
                }
                if (consume(RPAR)) {
                    if (stmCompound(false)) {
                        dropDomain();
                        owner = NULL;
                        return true;
                    }
                    else tkerr("lipseste corpul functiei");
                }
                else tkerr("lipseste ) la definitia functiei");
            }
        }
    }
    iTk = start;
    return false;
}

bool stm() {
    Ret rCond, rExpr; // Atribute folosite pentru colectarea tipurilor
    if (stmCompound(true)) return true;
    if (consume(IF)) {
        if (consume(LPAR)) {
            if (expr(&rCond)) {
                if (!canBeScalar(&rCond)) tkerr("the if condition must be a scalar value"); [cite:14]
                    if (consume(RPAR)) {
                        if (stm()) {
                            if (consume(ELSE)) {
                                if (!stm()) tkerr("lipseste instructiune dupa ELSE");
                            }
                            return true;
                        }
                    }
                    else tkerr("lipseste ) dupa conditia IF");
            }
        }
    }
    if (consume(WHILE)) {
        if (consume(LPAR)) {
            if (expr(&rCond)) {
                if (!canBeScalar(&rCond)) tkerr("the while condition must be a scalar value"); [cite:16]
                    if (consume(RPAR)) {
                        if (stm()) return true;
                    }
            }
        }
    }
    if (consume(RETURN)) {
        if (expr(&rExpr)) {
            if (owner->type.tb == TB_VOID) tkerr("a void function cannot return a value"); [cite:19]
                if (!canBeScalar(&rExpr)) tkerr("the return value must be a scalar value"); [cite:20]
                    if (!convTo(&rExpr.type, &owner->type)) tkerr("cannot convert the return expression type to the function return type"); [cite:21, 22]
        }
        else {
            if (owner->type.tb != TB_VOID) tkerr("a non-void function must return a value"); [cite:23]
        }
        if (consume(SEMICOLON)) return true;
    }
    if (expr(&rExpr)) {
        if (consume(SEMICOLON)) return true;
    }
    else if (consume(SEMICOLON)) return true;
    return false;
}

bool stmCompound(bool newDomain) {
    if (consume(LACC)) {
        if (newDomain) pushDomain();
        while (varDef() || stm());
        if (consume(RACC)) {
            if (newDomain) dropDomain();
            return true;
        }
    }
    return false;
}

bool expr(Ret* r) { return exprAssign(r); }[cite:26]

bool exprAssign(Ret* r) {
    Token* start = iTk;
    Ret rDst;
    if (exprUnary(&rDst)) {
        if (consume(ASSIGN)) {
            if (exprAssign(r)) {
                if (!rDst.lval) tkerr("the assign destination must be a left-value"); [cite:34]
                    if (rDst.ct) tkerr("the assign destination cannot be constant"); [cite:35]
                        if (!canBeScalar(&rDst)) tkerr("the assign destination must be scalar"); [cite:36]
                            if (!canBeScalar(r)) tkerr("the assign source must be scalar"); [cite:37]
                                if (!convTo(&r->type, &rDst.type)) tkerr("the assign source cannot be converted to destination"); [cite:38, 39]
                                    r->lval = false; [cite:40]
                                    r->ct = true; [cite:41]
                                    return true;
            }
        }
    }
    iTk = start;
    return exprOr(r);
}

bool exprOr(Ret* r) {
    if (exprAnd(r)) {
        if (exprOrPrim(r)) return true; [cite:238]
    }
    return false;
}

bool exprOrPrim(Ret* r) {
    if (consume(OR)) {
        Ret right;
        if (exprAnd(&right)) {
            [cite:254]
            Type tDst;
            if (!arithTypeTo(&r->type, &right.type, &tDst)) tkerr("invalid operand type for ||"); [cite:256, 257]
                * r = (Ret){ {TB_INT, NULL, -1}, false, true }; [cite:259]
                if (exprOrPrim(r)) return true; [cite:260]
        }
    }
    return true;
}

bool exprAnd(Ret* r) {
    if (exprEq(r)) {
        if (exprAndPrim(r)) return true;
    }
    return false;
}

bool exprAndPrim(Ret* r) {
    if (consume(AND)) {
        Ret right;
        if (exprEq(&right)) {
            Type tDst;
            if (!arithTypeTo(&r->type, &right.type, &tDst)) tkerr("invalid operand type for &&"); [cite:56]
                * r = (Ret){ {TB_INT, NULL, -1}, false, true }; [cite:56]
                if (exprAndPrim(r)) return true;
        }
    }
    return true;
}

bool exprEq(Ret* r) {
    if (exprRel(r)) {
        if (exprEqPrim(r)) return true;
    }
    return false;
}

bool exprEqPrim(Ret* r) {
    if (consume(EQUAL) || consume(NOTEQ)) {
        Ret right;
        if (exprRel(&right)) {
            Type tDst;
            if (!arithTypeTo(&r->type, &right.type, &tDst)) tkerr("invalid operand type for == or !="); [cite:64, 65]
                * r = (Ret){ {TB_INT, NULL, -1}, false, true }; [cite:66]
                if (exprEqPrim(r)) return true;
        }
    }
    return true;
}

bool exprRel(Ret* r) {
    if (exprAdd(r)) {
        if (exprRelPrim(r)) return true;
    }
    return false;
}

bool exprRelPrim(Ret* r) {
    if (consume(LESS) || consume(LESSEQ) || consume(GREATER) || consume(GREATEREQ)) {
        Ret right;
        if (exprAdd(&right)) {
            Type tDst;
            if (!arithTypeTo(&r->type, &right.type, &tDst)) tkerr("invalid operand type for relational operator"); [cite:75]
                * r = (Ret){ {TB_INT, NULL, -1}, false, true }; [cite:76]
                if (exprRelPrim(r)) return true;
        }
    }
    return true;
}

bool exprAdd(Ret* r) {
    if (exprMul(r)) {
        if (exprAddPrim(r)) return true;
    }
    return false;
}

bool exprAddPrim(Ret* r) {
    if (consume(ADD) || consume(SUB)) {
        Ret right;
        if (exprMul(&right)) {
            Type tDst;
            if (!arithTypeTo(&r->type, &right.type, &tDst)) tkerr("invalid operand type for + or -"); [cite:83]
                * r = (Ret){ tDst, false, true }; [cite:83]
                if (exprAddPrim(r)) return true;
        }
    }
    return true;
}

bool exprMul(Ret* r) {
    if (exprCast(r)) {
        if (exprMulPrim(r)) return true;
    }
    return false;
}

bool exprMulPrim(Ret* r) {
    if (consume(MUL) || consume(DIV)) {
        Ret right;
        if (exprCast(&right)) {
            Type tDst;
            if (!arithTypeTo(&r->type, &right.type, &tDst)) tkerr("invalid operand type for * or /"); [cite:90]
                * r = (Ret){ tDst, false, true }; [cite:91]
                if (exprMulPrim(r)) return true;
        }
    }
    return true;
}

bool exprCast(Ret* r) {
    Token* start = iTk;
    if (consume(LPAR)) {
        Type t;
        Ret op;
        if (typeBase(&t)) {
            arrayDecl(&t);
            if (consume(RPAR)) {
                if (exprCast(&op)) {
                    if (t.tb == TB_STRUCT) tkerr("cannot convert to a struct type"); [cite:100]
                        if (op.type.tb == TB_STRUCT) tkerr("cannot convert a struct"); [cite:101]
                            if (op.type.n >= 0 && t.n < 0) tkerr("an array can be converted only to another array"); [cite:102]
                                if (op.type.n < 0 && t.n >= 0) tkerr("a scalar can be converted only to another scalar"); [cite:103]
                                    * r = (Ret){ t, false, true }; [cite:103]
                                    return true;
                }
            }
        }
        iTk = start;
    }
    return exprUnary(r);
}

bool exprUnary(Ret* r) {
    if (consume(SUB) || consume(NOT)) {
        if (exprUnary(r)) {
            if (!canBeScalar(r)) tkerr("unary - or ! must have a scalar operand"); [cite:110]
                r->lval = false; [cite:111]
                r->ct = true; [cite:112]
                return true;
        }
    }
    return exprPostfix(r);
}

bool exprPostfix(Ret* r) {
    if (exprPrimary(r)) {
        if (exprPostfixPrim(r)) return true;
    }
    return false;
}

bool exprPostfixPrim(Ret* r) {
    if (consume(LBRACKET)) {
        Ret idx;
        if (expr(&idx)) {
            if (consume(RBRACKET)) {
                if (r->type.n < 0) tkerr("only an array can be indexed"); [cite:121]
                    Type tInt = { TB_INT, NULL, -1 }; [cite:122]
                    if (!convTo(&idx.type, &tInt)) tkerr("the index is not convertible to int"); [cite:123]
                        r->type.n = -1; [cite:124]
                        r->lval = true; [cite:125]
                        r->ct = false; [cite:126]
                        if (exprPostfixPrim(r)) return true;
            }
        }
    }
    if (consume(DOT)) {
        if (consume(ID)) {
            Token* tkName = consumedTk;
            if (r->type.tb != TB_STRUCT) tkerr("a field can only be selected from a struct"); [cite:130]
                Symbol* s = findSymbolInList(r->type.s->structMembers, tkName->text); [cite:131]
                if (!s) tkerr("the structure %s does not have a field %s", r->type.s->name, tkName->text); [cite:132, 133]
                    * r = (Ret){ s->type, true, s->type.n >= 0 }; [cite:134]
                    if (exprPostfixPrim(r)) return true;
        }
    }
    return true;
}

bool exprPrimary(Ret* r) {
    if (consume(ID)) {
        Token* tkName = consumedTk;
        Symbol* s = findSymbol(tkName->text); [cite:144]
            if (!s) tkerr("undefined id: %s", tkName->text); [cite:145]

                if (consume(LPAR)) {
                    if (s->kind != SK_FN) tkerr("only a function can be called"); [cite:149]
                        Ret rArg;
                    Symbol* param = s->fn.params; [cite:151]

                        if (expr(&rArg)) {
                            if (!param) tkerr("too many arguments in function call"); [cite:155]
                                if (!convTo(&rArg.type, &param->type)) tkerr("in call, cannot convert the argument type to the parameter type"); [cite:156, 157]
                                    param = param->next; [cite:158]

                                    while (consume(COMMA)) {
                                        if (!expr(&rArg)) tkerr("lipseste expresie dupa ,");
                                        if (!param) tkerr("too many arguments in function call"); [cite:162]
                                            if (!convTo(&rArg.type, &param->type)) tkerr("in call, cannot convert the argument type to the parameter type"); [cite:163, 164]
                                                param = param->next; [cite:166]
                                    }
                        }
                    if (consume(RPAR)) {
                        if (param) tkerr("too few arguments in function call"); [cite:169]
                            * r = (Ret){ s->type, false, true }; [cite:170]
                            return true;
                    }
                }
                else {
                    if (s->kind == SK_FN) tkerr("a function can only be called"); [cite:173]
                        * r = (Ret){ s->type, true, s->type.n >= 0 }; [cite:174]
                        return true;
                }
    }
    if (consume(INT)) { *r = (Ret){ {TB_INT, NULL, -1}, false, true }; return true; } [cite:176]
        if (consume(DOUBLE)) { *r = (Ret){ {TB_DOUBLE, NULL, -1}, false, true }; return true; } [cite:177]
            if (consume(CHAR)) { *r = (Ret){ {TB_CHAR, NULL, -1}, false, true }; return true; } [cite:178]
                if (consume(STRING)) { *r = (Ret){ {TB_CHAR, NULL, 0}, false, true }; return true; } [cite:179]

                    if (consume(LPAR)) {
                        if (expr(r)) {
                            if (consume(RPAR)) return true;
                        }
                    }
    return false;
}

bool unit() {
    for (;;) {
        if (structDef()) {}
        else if (fnDef()) {}
        else if (varDef()) {}
        else break;
    }
    if (consume(END)) return true;
    return false;
}

void parse(Token* tokens) {
    iTk = tokens;
    pushDomain();
    if (!unit()) tkerr("eroare de sintaxa");
    showDomain(symTable, "global");
    dropDomain();
}