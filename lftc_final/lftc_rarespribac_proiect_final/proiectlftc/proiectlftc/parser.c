#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>
#include "parser.h"
#include "ad.h"
#include "at.h"
#include "vm.h"
#include "gc.h" // Includem generatorul de cod

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

bool typeBase(Type* t) {
    t->n = -1;
    if (consume(TYPE_INT)) { t->tb = TB_INT; return true; }
    if (consume(TYPE_DOUBLE)) { t->tb = TB_DOUBLE; return true; }
    if (consume(TYPE_CHAR)) { t->tb = TB_CHAR; return true; }
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
        else { t->n = 0; }
        if (consume(RBRACKET)) return true;
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
                s->type.tb = TB_STRUCT; s->type.s = s; s->type.n = -1;
                pushDomain();
                owner = s;

                while (varDef());

                if (consume(RACC)) {
                    if (consume(SEMICOLON)) {
                        owner = NULL; dropDomain(); return true;
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
            if (arrayDecl(&t)) { t.n = 0; }

            Symbol* param = findSymbolInDomain(symTable, tkName->text);
            if (param) tkerr("symbol redefinition: %s", tkName->text);

            param = newSymbol(tkName->text, SK_PARAM);
            param->type = t; param->owner = owner;
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
                fn->type = t; addSymbolToDomain(symTable, fn);
                owner = fn; pushDomain();

                if (fnParam()) {
                    while (consume(COMMA)) {
                        if (!fnParam()) tkerr("lipseste parametru dupa ,");
                    }
                }
                if (consume(RPAR)) {
                    Instr* co = addInstr(&fn->fn.instr, OP_ENTER); // Generare ENTER pentru cadrul functiei
                    int localsCountStart = symbolsLen(fn->fn.locals);

                    if (stmCompound(false, &fn->fn.instr)) {
                        co->arg.i = symbolsLen(fn->fn.locals) - localsCountStart; // Setam numarul final de variabile locale
                        if (t.tb == TB_VOID) {
                            addInstrWithInt(&fn->fn.instr, OP_RET_VOID, symbolsLen(fn->fn.params));
                        }
                        dropDomain(); owner = NULL; return true;
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

bool stm(Instr** code) {
    Ret rCond, rExpr;
    if (stmCompound(true, code)) return true;
    if (consume(IF)) {
        if (consume(LPAR)) {
            if (expr(&rCond, code)) {
                if (!canBeScalar(&rCond)) tkerr("the if condition must be a scalar value");
                addRVal(code, rCond.lval, &rCond.type);
                insertConvIfNeeded(lastInstr(*code), &rCond.type, &(Type){TB_INT, NULL, -1});

                if (consume(RPAR)) {
                    Instr* jf = addInstr(code, OP_JF);
                    if (stm(code)) {
                        if (consume(ELSE)) {
                            Instr* jmp = addInstr(code, OP_JMP);
                            jf->arg.instr = lastInstr(*code)->next; // Fixam saltul JF peste IF catre ELSE
                            if (!stm(code)) tkerr("lipseste instructiune dupa ELSE");
                            jmp->arg.instr = lastInstr(*code)->next; // Fixam saltul JMP peste ELSE
                        }
                        else {
                            jf->arg.instr = lastInstr(*code)->next;
                        }
                        return true;
                    }
                }
                else tkerr("lipseste ) dupa conditia IF");
            }
            else tkerr("expresie invalida in IF");
        }
    }
    if (consume(WHILE)) {
        if (consume(LPAR)) {
            Instr* whilePos = lastInstr(*code)->next;
            if (expr(&rCond, code)) {
                if (!canBeScalar(&rCond)) tkerr("the while condition must be a scalar value");
                addRVal(code, rCond.lval, &rCond.type);
                insertConvIfNeeded(lastInstr(*code), &rCond.type, &(Type){TB_INT, NULL, -1});

                if (consume(RPAR)) {
                    Instr* jf = addInstr(code, OP_JF);
                    if (stm(code)) {
                        addInstr(code, OP_JMP)->arg.instr = whilePos;
                        jf->arg.instr = lastInstr(*code)->next;
                        return true;
                    }
                }
            }
        }
    }
    if (consume(RETURN)) {
        if (expr(&rExpr, code)) {
            if (owner->type.tb == TB_VOID) tkerr("a void function cannot return a value");
            if (!canBeScalar(&rExpr)) tkerr("the return value must be a scalar value");
            addRVal(code, rExpr.lval, &rExpr.type);
            insertConvIfNeeded(lastInstr(*code), &rExpr.type, &owner->type);
            if (!convTo(&rExpr.type, &owner->type)) tkerr("cannot convert the return expression type to the function return type");
            addInstrWithInt(code, OP_RET, symbolsLen(owner->fn.params));
        }
        else {
            if (owner->type.tb != TB_VOID) tkerr("a non-void function must return a value");
            addInstrWithInt(code, OP_RET_VOID, symbolsLen(owner->fn.params));
        }
        if (consume(SEMICOLON)) return true;
    }
    if (expr(&rExpr, code)) {
        if (rExpr.type.tb != TB_VOID) {
            addInstr(code, OP_DROP); // Daca expresia returneaza ceva dar nu e asignata, curatam stiva
        }
        if (consume(SEMICOLON)) return true;
    }
    else if (consume(SEMICOLON)) return true;
    return false;
}

bool stmCompound(bool newDomain, Instr** code) {
    if (consume(LACC)) {
        if (newDomain) pushDomain();
        while (varDef() || stm(code));
        if (consume(RACC)) {
            if (newDomain) dropDomain(); return true;
        }
    }
    return false;
}

bool expr(Ret* r, Instr** code) { return exprAssign(r, code); }

bool exprAssign(Ret* r, Instr** code) {
    Token* start = iTk;
    Ret rDst;
    Instr* oldLast = lastInstr(*code);
    if (exprUnary(&rDst, code)) {
        if (consume(ASSIGN)) {
            if (exprAssign(r, code)) {
                if (!rDst.lval) tkerr("the assign destination must be a left-value");
                if (rDst.ct) tkerr("the assign destination cannot be constant");
                if (!canBeScalar(&rDst)) tkerr("the assign destination must be scalar");
                if (!canBeScalar(r)) tkerr("the assign source must be scalar");
                if (!convTo(&r->type, &rDst.type)) tkerr("the assign source cannot be converted to destination");

                addRVal(code, r->lval, &r->type);
                insertConvIfNeeded(lastInstr(*code), &r->type, &rDst.type);
                addInstr(code, OP_STORE_I); // Generam stocarea valorii la adresa determinata

                r->lval = false;
                r->ct = true;
                return true;
            }
        }
    }
    iTk = start;
    delInstrAfter(oldLast); // Curatam codul partial generat la incercarea esuata de exprUnary
    return exprOr(r, code);
}

bool exprOr(Ret* r, Instr** code) {
    if (exprAnd(r, code)) {
        if (exprOrPrim(r, code)) return true;
    }
    return false;
}

bool exprOrPrim(Ret* r, Instr** code) {
    if (consume(OR)) {
        Ret right;
        addRVal(code, r->lval, &r->type);
        if (exprAnd(&right, code)) {
            Type tDst;
            if (!arithTypeTo(&r->type, &right.type, &tDst)) tkerr("invalid operand type for ||");
            addRVal(code, right.lval, &right.type);
            // Pentru simplitate in AtomC se mapeaza operatiile logice pe cele aritmetice la nivel de VM de test
            addInstr(code, OP_ADD_I);
            *r = (Ret){ {TB_INT, NULL, -1}, false, true };
            if (exprOrPrim(r, code)) return true;
        }
    }
    return true;
}

bool exprAnd(Ret* r, Instr** code) {
    if (exprEq(r, code)) {
        if (exprAndPrim(r, code)) return true;
    }
    return false;
}

bool exprAndPrim(Ret* r, Instr** code) {
    if (consume(AND)) {
        Ret right;
        addRVal(code, r->lval, &r->type);
        if (exprEq(&right, code)) {
            Type tDst;
            if (!arithTypeTo(&r->type, &right.type, &tDst)) tkerr("invalid operand type for &&");
            addRVal(code, right.lval, &right.type);
            addInstr(code, OP_MUL_I);
            *r = (Ret){ {TB_INT, NULL, -1}, false, true };
            if (exprAndPrim(r, code)) return true;
        }
    }
    return true;
}

bool exprEq(Ret* r, Instr** code) {
    if (exprRel(r, code)) {
        if (exprEqPrim(r, code)) return true;
    }
    return false;
}

bool exprEqPrim(Ret* r, Instr** code) {
    if (consume(EQUAL) || consume(NOTEQ)) {
        Ret right;
        if (exprRel(&right, code)) {
            Type tDst;
            if (!arithTypeTo(&r->type, &right.type, &tDst)) tkerr("invalid operand type for == or !=");
            *r = (Ret){ {TB_INT, NULL, -1}, false, true };
            if (exprEqPrim(r, code)) return true;
        }
    }
    return true;
}

bool exprRel(Ret* r, Instr** code) {
    if (exprAdd(r, code)) {
        if (exprRelPrim(r, code)) return true;
    }
    return false;
}

bool exprRelPrim(Ret* r, Instr** code) {
    if (consume(LESS) || consume(LESSEQ) || consume(GREATER) || consume(GREATEREQ)) {
        int opCode = consumedTk->code;
        Ret right;
        addRVal(code, r->lval, &r->type);
        if (exprAdd(&right, code)) {
            Type tDst;
            if (!arithTypeTo(&r->type, &right.type, &tDst)) tkerr("invalid operand type for relational operator");
            addRVal(code, right.lval, &right.type);
            if (opCode == LESS) {
                addInstr(code, OP_LESS_I);
            }
            *r = (Ret){ {TB_INT, NULL, -1}, false, true };
            if (exprRelPrim(r, code)) return true;
        }
    }
    return true;
}

bool exprAdd(Ret* r, Instr** code) {
    if (exprMul(r, code)) {
        if (exprAddPrim(r, code)) return true;
    }
    return false;
}

bool exprAddPrim(Ret* r, Instr** code) {
    if (consume(ADD) || consume(SUB)) {
        int opCode = consumedTk->code;
        Ret right;
        addRVal(code, r->lval, &r->type);
        if (exprMul(&right, code)) {
            Type tDst;
            if (!arithTypeTo(&r->type, &right.type, &tDst)) tkerr("invalid operand type for + or -");
            addRVal(code, right.lval, &right.type);
            if (opCode == ADD) addInstr(code, OP_ADD_I);
            else addInstr(code, OP_SUB_I);
            *r = (Ret){ tDst, false, true };
            if (exprAddPrim(r, code)) return true;
        }
    }
    return true;
}

bool exprMul(Ret* r, Instr** code) {
    if (exprCast(r, code)) {
        if (exprMulPrim(r, code)) return true;
    }
    return false;
}

bool exprMulPrim(Ret* r, Instr** code) {
    if (consume(MUL) || consume(DIV)) {
        Ret right;
        addRVal(code, r->lval, &r->type);
        if (exprCast(&right, code)) {
            Type tDst;
            if (!arithTypeTo(&r->type, &right.type, &tDst)) tkerr("invalid operand type for * or /");
            addRVal(code, right.lval, &right.type);
            addInstr(code, OP_MUL_I);
            *r = (Ret){ tDst, false, true };
            if (exprMulPrim(r, code)) return true;
        }
    }
    return true;
}

bool exprCast(Ret* r, Instr** code) {
    Token* start = iTk;
    if (consume(LPAR)) {
        Type t;
        Ret op;
        if (typeBase(&t)) {
            arrayDecl(&t);
            if (consume(RPAR)) {
                if (exprCast(&op, code)) {
                    if (t.tb == TB_STRUCT) tkerr("cannot convert to a struct type");
                    if (op.type.tb == TB_STRUCT) tkerr("cannot convert a struct");
                    if (op.type.n >= 0 && t.n < 0) tkerr("an array can be converted only to another array");
                    if (op.type.n < 0 && t.n >= 0) tkerr("a scalar can be converted only to another scalar");

                    addRVal(code, op.lval, &op.type);
                    insertConvIfNeeded(lastInstr(*code), &op.type, &t);

                    *r = (Ret){ t, false, true };
                    return true;
                }
            }
        }
        iTk = start;
    }
    return exprUnary(r, code);
}

bool exprUnary(Ret* r, Instr** code) {
    if (consume(SUB) || consume(NOT)) {
        if (exprUnary(r, code)) {
            if (!canBeScalar(r)) tkerr("unary - or ! must have a scalar operand");
            r->lval = false;
            r->ct = true;
            return true;
        }
    }
    return exprPostfix(r, code);
}

bool exprPostfix(Ret* r, Instr** code) {
    if (exprPrimary(r, code)) {
        if (exprPostfixPrim(r, code)) return true;
    }
    return false;
}

bool exprPostfixPrim(Ret* r, Instr** code) {
    if (consume(LBRACKET)) {
        Ret idx;
        if (expr(&idx, code)) {
            if (consume(RBRACKET)) {
                if (r->type.n < 0) tkerr("only an array can be indexed");
                Type tInt = { TB_INT, NULL, -1 };
                if (!convTo(&idx.type, &tInt)) tkerr("the index is not convertible to int");
                r->type.n = -1;
                r->lval = true;
                r->ct = false;
                if (exprPostfixPrim(r, code)) return true;
            }
        }
    }
    if (consume(DOT)) {
        if (consume(ID)) {
            Token* tkName = consumedTk;
            if (r->type.tb != TB_STRUCT) tkerr("a field can only be selected from a struct");
            Symbol* s = findSymbolInList(r->type.s->structMembers, tkName->text);
            if (!s) tkerr("the structure %s does not have a field %s", r->type.s->name, tkName->text);
            *r = (Ret){ s->type, true, s->type.n >= 0 };
            if (exprPostfixPrim(r, code)) return true;
        }
    }
    return true;
}

bool exprPrimary(Ret* r, Instr** code) {
    if (consume(ID)) {
        Token* tkName = consumedTk;
        Symbol* s = findSymbol(tkName->text);
        if (!s) tkerr("undefined id: %s", tkName->text);

        if (consume(LPAR)) {
            if (s->kind != SK_FN) tkerr("only a function can be called");
            Ret rArg;
            Symbol* param = s->fn.params;

            if (expr(&rArg, code)) {
                if (!param) tkerr("too many arguments in function call");
                addRVal(code, rArg.lval, &rArg.type);
                insertConvIfNeeded(lastInstr(*code), &rArg.type, &param->type);
                if (!convTo(&rArg.type, &param->type)) tkerr("in call, cannot convert the argument type to the parameter type");
                param = param->next;

                while (consume(COMMA)) {
                    if (!expr(&rArg, code)) tkerr("lipseste expresie dupa ,");
                    if (!param) tkerr("too many arguments in function call");
                    addRVal(code, rArg.lval, &rArg.type);
                    insertConvIfNeeded(lastInstr(*code), &rArg.type, &param->type);
                    if (!convTo(&rArg.type, &param->type)) tkerr("in call, cannot convert the argument type to the parameter type");
                    param = param->next;
                }
            }
            if (consume(RPAR)) {
                if (param) tkerr("too few arguments in function call");
                if (s->fn.extFnPtr) {
                    addInstr(code, OP_CALL_EXT)->arg.extFnPtr = s->fn.extFnPtr;
                }
                else {
                    addInstr(code, OP_CALL)->arg.instr = s->fn.instr;
                }
                *r = (Ret){ s->type, false, true };
                return true;
            }
        }
        else {
            if (s->kind == SK_FN) tkerr("a function can only be called");
            if (s->kind == SK_VAR) {
                if (s->owner == NULL) { // Variabila globala
                    addInstr(code, OP_ADDR)->arg.p = s->varMem;
                }
                else { // Variabila locala sau parametru
                    addInstrWithInt(code, OP_FPADDR_I, s->varIdx);
                }
            }
            else if (s->kind == SK_PARAM) {
                addInstrWithInt(code, OP_FPADDR_I, -2 - symbolsLen(s->owner->fn.params) + s->paramIdx);
            }
            *r = (Ret){ s->type, true, s->type.n >= 0 };
            return true;
        }
    }
    if (consume(INT)) {
        *r = (Ret){ {TB_INT, NULL, -1}, false, true };
        addInstrWithInt(code, OP_PUSH_I, consumedTk->i);
        return true;
    }
    if (consume(DOUBLE)) {
        *r = (Ret){ {TB_DOUBLE, NULL, -1}, false, true };
        addInstrWithDouble(code, OP_PUSH_F, consumedTk->d);
        return true;
    }
    if (consume(CHAR)) { *r = (Ret){ {TB_CHAR, NULL, -1}, false, true }; return true; }
    if (consume(STRING)) { *r = (Ret){ {TB_CHAR, NULL, 0}, false, true }; return true; }

    if (consume(LPAR)) {
        if (expr(r, code)) {
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
    vmInit(); // Initializam masina virtuala si functiile native inaintea parsarii
    if (!unit()) tkerr("eroare de sintaxa");

    // Cautam functia main din AtomC pentru a sti de unde sa incepem executia
    Symbol* m = findSymbol("main");
    if (!m || m->kind != SK_FN) tkerr("lipseste functia main");

    printf("\n--- EXECUTIE COD GENERAT DIN TESTGC.C ---\n");
    run(m->fn.instr); // Pornim executia masinii virtuale direct din codul generat!

    dropDomain();
}