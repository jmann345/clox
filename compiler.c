#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "chunk.h"
#include "common.h"
#include "compiler.h"
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

typedef void (*ParseFn)();

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

static void compileExpression();
static ParseRule* getRule(TokenType type);
static void parsePrecedence(Precedence precedence) {
    advance();
    ParseFn prefixRule = getRule(parser.previous.type)->prefix;
    if (prefixRule == NULL) {
        error("Expect expression.");
        return;
    }

    prefixRule();

    while (precedence <= getRule(parser.current.type)->precedence) {
        advance();
        ParseFn infixRule = getRule(parser.previous.type)->infix;
        infixRule();
    }
    // TODO: Add suffix rule for '++' and '--'
}

static void compileExpression() {
    parsePrecedence(PREC_ASSIGNMENT);
}

static void compileBinary() {
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

static void compileLiteral() {
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
static void compileGrouping() {
    compileExpression();
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after expression.");
}

static void compileNumber() {
    f64 value = strtod(parser.previous.start, NULL);
    emitConstant(NUMBER_VAL(value));
}

static void compileString() {
    emitConstant(OBJ_VAL(copyString(parser.previous.start+1,
                                    parser.previous.length-2)));
}

static void compileUnary() {
    TokenType tok = parser.previous.type;
    parsePrecedence(PREC_UNARY); // compile the operand

    switch (tok) {
        case TOKEN_NOT: emitByte(OP_NOT); break;
        case TOKEN_MINUS: emitByte(OP_NEGATE); break;
        default: return; // unreachable
    }
}

static void compileTernary() {
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
  [TOKEN_IDENTIFIER]    = {NULL,            NULL,           PREC_NONE},
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
    compileExpression();
    consume(TOKEN_EOF, "Expect end of expression.");
    haltCompiler();

    return !parser.hadError;
}
