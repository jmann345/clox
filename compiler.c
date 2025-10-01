#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "chunk.h"
#include "common.h"
#include "compiler.h"
#include "object.h"
#include "tokenizer.h"
#include "value.h"

#ifdef DEBUG_PRINT_CODE
#include "debug.h"
#endif

typedef enum {
  PREC_NONE,
  PREC_ASSIGNMENT,  // "="
  PREC_TERNARY,     // "?"
  PREC_OR,          // "or"
  PREC_AND,         // "and"
  PREC_EQUALITY,    // "==" | "!="
  PREC_COMPARISON,  // "<" | ">" | "<=" | ">="
  PREC_TERM,        // "+" | "-"
  PREC_FACTOR,      // "*" | "/"
  PREC_UNARY,       // "not" | "-" | "--" | "++"
  PREC_POSTFIX,     // "--" | "++"
  PREC_CALL,        // "." | "()"
  PREC_PRIMARY
} Precedence;

typedef void (*ParseFn)(bool assignable);

typedef struct {
    ParseFn prefix;
    ParseFn infix;
    Precedence precedence;
} ParseRule;


typedef struct {
    Token current;
    Token previous;
    bool hadError;
    bool panicMode;
} Parser;

static Parser parser;
static Chunk* compilingChunk;

// NOTE: This will be changed later
static Chunk* currentChunk() {
    return compilingChunk;
}

static void errorAt(const Token* token, const char* message) {
    if (parser.panicMode) return;
    parser.panicMode = true;
    fprintf(stderr, "[line %d] Error", token->line);

    if (token->type == TOKEN_EOF) {
        fprintf(stderr, " at end");
    } else if (token->type == TOKEN_ERROR) {
        // Nothing.
    } else {
        fprintf(stderr, " at '%.*s'", token->length, token->start);
    }

    fprintf(stderr, ": %s\n", message);
    parser.hadError = true;
}

static void error(const char* const message) {
    errorAt(&parser.previous, message);
}

static void errorAtCurrent(const char* const message) {
    errorAt(&parser.current, message);
}

static void advance() {
    parser.previous = parser.current;
    for ever {
        parser.current = scanToken();
        if (parser.current.type != TOKEN_ERROR) break;

        errorAtCurrent(parser.current.start);
    }
}

static void consume(TokenType type, const char* message) {
    if (parser.current.type == type) {
        advance();
        return;
    }

    errorAtCurrent(message);
}

static bool tryConsume(TokenType type) {
    if (parser.current.type == type) {
        advance();
        return true;
    }

    return false;
}

static bool peekIsOneOf(int count, ...) {
    va_list types;
    va_start(types, count);

    TokenType peek = parser.current.type;
    bool match = false;
    for (int i = 0; i < count; i++) {
        int tok = va_arg(types, int);
        if (peek == (TokenType)tok) {
            match = true;
            break;
        }
    }

    va_end(types);
    return match;
}

static bool consumeOneOf(int count, ...) {
    va_list types;
    va_start(types, count);

    TokenType peek = parser.current.type;
    bool match = false;
    for (int i = 0; i < count; i++) {
        int tok = va_arg(types, int);
        if (peek == (TokenType)tok) {
            match = true;
            break;
        }
    }

    va_end(types);
    advance();

    return match;
}

static void emitByte(u8 byte) {
    appendChunk(currentChunk(), byte, parser.previous.line);
}

static void emitBytes(int count, ...) {
    va_list bytes;
    va_start(bytes, count);

    for (int i = 0; i < count; i++) {
        int byte = va_arg(bytes, int);
        assert(byte >= 0 && byte <= 255);
        emitByte((u8)byte);
    }

    va_end(bytes);
}

static void emitReturn() {
    emitByte(OP_RETURN);
}

static u8 makeConstant(Value value) {
    u32 constIndex = addConstant(currentChunk(), value);
    if (constIndex > U8_MAX) {
        error("Too many constants in one chunk.");
        return 0;
    }

    return (u8)constIndex;
}

static void emitConstant(Value value) {
    emitBytes(2, OP_CONSTANT, makeConstant(value));
}

static void haltCompiler() {
    emitReturn();
#ifdef DEBUG_PRINT_CODE
    if (!parser.hadError) {
        disassembleChunk(currentChunk(), "code");
    }
#endif
}

static void compileDeclaration();
static void compileVarDecl();
static void compileStatement();
static void compilePrintStmt();
static void compileExprStmt();
static void compileExpression();

static ParseRule* getRule(TokenType type);

static void synchronize() {
    parser.panicMode = false;

    for (;parser.current.type != TOKEN_EOF; advance()) {
        if (parser.previous.type == TOKEN_SEMICOLON) return;
        if (peekIsOneOf(8,
            TOKEN_CLASS, TOKEN_FUN, TOKEN_VAR,
            TOKEN_FOR, TOKEN_IF, TOKEN_WHILE,
            TOKEN_PRINT, TOKEN_RETURN)) {
            return;
        }
    }
}

static void parsePrecedence(Precedence precedence) {
    advance();
    ParseFn prefixRule = getRule(parser.previous.type)->prefix;
    if (prefixRule == NULL) {
        error("Expect expression.");
        return;
    }

    bool assignable = precedence <= PREC_ASSIGNMENT;
    prefixRule(assignable);

    while (precedence <= getRule(parser.current.type)->precedence) {
        advance();
        ParseFn infixRule = getRule(parser.previous.type)->infix;
        infixRule(assignable);
    }

    if (assignable && tryConsume(TOKEN_EQUAL)) {
        error("Invalid assignment target.");
    }
    // TODO: Add suffix rule for '++' and '--'
}

static u8 identifierConstant(Token* name) {
    return makeConstant(OBJ_VAL(copyString(name->start, name->length)));
}

static u8 parseVariable(const char* errorMessage) {
    consume(TOKEN_IDENTIFIER, errorMessage);
    return identifierConstant(&parser.previous);
}

static void defineVariable(u8 global) {
    emitBytes(2, OP_DEFINE_GLOBAL, global);
}

static void compileDeclaration() {
    if (tryConsume(TOKEN_VAR)) {
        compileVarDecl();
    } else {
        compileStatement();
    }

    if (parser.panicMode) synchronize();
}

static void compileVarDecl() {
    u8 global = parseVariable("Expect variable name.");

    if (tryConsume(TOKEN_EQUAL)) {
        compileExpression();
    } else {
        emitByte(OP_NIL);
    }

    consume(TOKEN_SEMICOLON, "Expect ';' after variable declaration.");

    defineVariable(global);
}

static void compileStatement() {
    if (tryConsume(TOKEN_PRINT)) {
        compilePrintStmt();
    } else {
        compileExprStmt();
    }
}

static void compilePrintStmt() {
    compileExpression();
    consume(TOKEN_SEMICOLON, "Expect ';' after expression.");
    emitByte(OP_PRINT);
}

static void compileExprStmt() {
    compileExpression();
    consume(TOKEN_SEMICOLON, "Expect ';' after expression.");
    emitByte(OP_POP);
}

static void compileExpression() {
    parsePrecedence(PREC_ASSIGNMENT);
}

static void compileBinary(bool _assignable) {
    TokenType op = parser.previous.type;
    ParseRule* rule = getRule(op);
    parsePrecedence((Precedence)rule->precedence + 1);

    switch (op) {
        case TOKEN_EQUAL_EQUAL:     emitByte(OP_EQUAL); break;
        case TOKEN_BANG_EQUAL:      emitBytes(2, OP_EQUAL, OP_NOT); break;
        case TOKEN_GREATER:         emitByte(OP_GREATER); break;
        case TOKEN_GREATER_EQUAL:   emitBytes(2, OP_LESS, OP_NOT); break;
        case TOKEN_LESS:            emitByte(OP_LESS); break;
        case TOKEN_LESS_EQUAL:      emitBytes(2, OP_GREATER, OP_NOT); break;
        case TOKEN_PLUS:            emitByte(OP_ADD); break;
        case TOKEN_MINUS:           emitByte(OP_SUBTRACT); break;
        case TOKEN_STAR:            emitByte(OP_MULTIPLY); break;
        case TOKEN_SLASH:           emitByte(OP_DIVIDE); break;
        default: return; // Unreachable.
    }
}

static void compileLiteral(bool _assignable) {
    switch (parser.previous.type) {
        case TOKEN_NIL:     emitByte(OP_NIL);   break;
        case TOKEN_TRUE:    emitByte(OP_TRUE);  break;
        case TOKEN_FALSE:   emitByte(OP_FALSE); break;
        default: return; // Unreachable.
    }
}

// Notice that this doesn't directly emit any bytecode
// That's by design! A grouping expression simply "upgrades" the precedance
// of an expression, so it only changes the expression's location on the AST
static void compileGrouping(bool _assignable) {
    compileExpression();
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after expression.");
}

static void compileNumber(bool _assignable) {
    f64 value = strtod(parser.previous.start, NULL);
    emitConstant(NUMBER_VAL(value));
}

static void compileString(bool _assignable) {
    emitConstant(OBJ_VAL(copyString(parser.previous.start+1,
                                    parser.previous.length-2)));
}

static void fetchNamedVariable(Token name, bool assignable) {
    u8 arg = identifierConstant(&name);
    if (assignable && tryConsume(TOKEN_EQUAL)) {
        compileExpression();
        emitBytes(2, OP_SET_GLOBAL, arg);
    } else {
        emitBytes(2, OP_GET_GLOBAL, arg);
    }
}

static void compileVariable(bool assignable) {
    fetchNamedVariable(parser.previous, assignable);
}

static void compileUnary(bool _assignable) {
    TokenType tok = parser.previous.type;
    parsePrecedence(PREC_UNARY); // compile the operand

    switch (tok) {
        case TOKEN_NOT: emitByte(OP_NOT); break;
        case TOKEN_MINUS: emitByte(OP_NEGATE); break;
        default: return; // unreachable
    }
}

static void compileTernary(bool _assignable) {
    parsePrecedence(PREC_TERNARY - 1); // parse rhs of ?
    consume(TOKEN_COLON, "Expect ':' in ternary expression.");
    parsePrecedence(PREC_TERNARY - 1); // parse rhs of :
}

ParseRule rules[] = {
  [TOKEN_LEFT_PAREN]    = {compileGrouping, NULL,           PREC_NONE},
  [TOKEN_RIGHT_PAREN]   = {NULL,            NULL,           PREC_NONE},
  [TOKEN_LEFT_BRACKET]  = {NULL,            NULL,           PREC_NONE}, 
  [TOKEN_RIGHT_BRACKET] = {NULL,            NULL,           PREC_NONE},
  [TOKEN_LEFT_BRACE]    = {NULL,            NULL,           PREC_NONE}, 
  [TOKEN_RIGHT_BRACE]   = {NULL,            NULL,           PREC_NONE},
  [TOKEN_SEMICOLON]     = {NULL,            NULL,           PREC_NONE},
  [TOKEN_COMMA]         = {NULL,            NULL,           PREC_NONE},
  [TOKEN_DOT]           = {NULL,            NULL,           PREC_NONE},
  [TOKEN_MINUS]         = {compileUnary,    compileBinary,  PREC_TERM},
  [TOKEN_PLUS]          = {NULL,            compileBinary,  PREC_TERM},
  [TOKEN_SLASH]         = {NULL,            compileBinary,  PREC_FACTOR},
  [TOKEN_STAR]          = {NULL,            compileBinary,  PREC_FACTOR},
  [TOKEN_EQUAL]         = {NULL,            NULL,           PREC_NONE},
  [TOKEN_MINUS_MINUS]   = {NULL,            NULL,           PREC_NONE},
  [TOKEN_PLUS_PLUS]     = {NULL,            NULL,           PREC_NONE},
  [TOKEN_MINUS_EQUAL]   = {NULL,            NULL,           PREC_NONE},
  [TOKEN_PLUS_EQUAL]    = {NULL,            NULL,           PREC_NONE},
  [TOKEN_SLASH_EQUAL]   = {NULL,            NULL,           PREC_NONE},
  [TOKEN_STAR_EQUAL]    = {NULL,            NULL,           PREC_NONE},
  [TOKEN_EQUAL_EQUAL]   = {NULL,            compileBinary,  PREC_EQUALITY},
  [TOKEN_BANG_EQUAL]    = {NULL,            compileBinary,  PREC_EQUALITY},
  [TOKEN_LESS]          = {NULL,            compileBinary,  PREC_COMPARISON},
  [TOKEN_LESS_EQUAL]    = {NULL,            compileBinary,  PREC_COMPARISON},
  [TOKEN_GREATER]       = {NULL,            compileBinary,  PREC_COMPARISON},
  [TOKEN_GREATER_EQUAL] = {NULL,            compileBinary,  PREC_COMPARISON},
  [TOKEN_COLON]         = {NULL,            NULL,           PREC_NONE},
  [TOKEN_QUESTION_MARK] = {NULL,            compileTernary, PREC_TERNARY},
  [TOKEN_NOT]           = {compileUnary,    NULL,           PREC_NONE},
  [TOKEN_AND]           = {NULL,            NULL,           PREC_NONE},
  [TOKEN_OR]            = {NULL,            NULL,           PREC_NONE},
  [TOKEN_NIL]           = {compileLiteral,  NULL,           PREC_NONE},
  [TOKEN_TRUE]          = {compileLiteral,  NULL,           PREC_NONE},
  [TOKEN_FALSE]         = {compileLiteral,  NULL,           PREC_NONE},
  [TOKEN_IF]            = {NULL,            NULL,           PREC_NONE},
  [TOKEN_ELSE]          = {NULL,            NULL,           PREC_NONE},
  [TOKEN_WHILE]         = {NULL,            NULL,           PREC_NONE},
  [TOKEN_FOR]           = {NULL,            NULL,           PREC_NONE},
  [TOKEN_FUN]           = {NULL,            NULL,           PREC_NONE},
  [TOKEN_BREAK]         = {NULL,            NULL,           PREC_NONE},
  [TOKEN_CYCLE]         = {NULL,            NULL,           PREC_NONE},
  [TOKEN_RETURN]        = {NULL,            NULL,           PREC_NONE},
  [TOKEN_CLASS]         = {NULL,            NULL,           PREC_NONE},
  [TOKEN_SUPER]         = {NULL,            NULL,           PREC_NONE},
  [TOKEN_THIS]          = {NULL,            NULL,           PREC_NONE},
  [TOKEN_VAR]           = {NULL,            NULL,           PREC_NONE},
  [TOKEN_PRINT]         = {NULL,            NULL,           PREC_NONE},
  [TOKEN_IDENTIFIER]    = {compileVariable, NULL,           PREC_NONE},
  [TOKEN_NUMBER]        = {compileNumber,   NULL,           PREC_NONE},
  [TOKEN_STRING]        = {compileString,   NULL,           PREC_NONE},
  [TOKEN_LIST]          = {NULL,            NULL,           PREC_NONE},
  [TOKEN_ERROR]         = {NULL,            NULL,           PREC_NONE},
  [TOKEN_NAT]           = {NULL,            NULL,           PREC_NONE},
  [TOKEN_EOF]           = {NULL,            NULL,           PREC_NONE},
};

static ParseRule* getRule(TokenType type) {
    return &rules[type];
}

bool compile(const char* source, Chunk* chunk) {
    initTokenizer(source);
    compilingChunk = chunk;

    parser.hadError = false;
    parser.panicMode = false;

    advance();
    while (!tryConsume(TOKEN_EOF)) {
        compileDeclaration();
    }
    haltCompiler();

    return !parser.hadError;
}
