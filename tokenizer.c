#include <stdio.h>
#include <string.h>

#include "common.h"
#include "tokenizer.h"

typedef struct {
    const char* start;  // the ptr is not constant, just the str it pts to
    const char* current;
    u32 line;
} Tokenizer;

static Tokenizer tokenizer;

void initTokenizer(const char *source) {
    tokenizer.start = source;
    tokenizer.current = source;
    tokenizer.line = 1;
}

static Token makeToken(TokenType type) {
    Token token = {
        .type = type,
        .start = tokenizer.start,
        .length = (u32)(tokenizer.current - tokenizer.start),
        .line = tokenizer.line
    };

    return token;
}

static Token errorToken(const char* message) {
    Token token = {
        .type = TOKEN_ERROR,
        .start = message,
        .length = (u32)strlen(message),
        .line = tokenizer.line
    };

    return token;
}

static Token notAToken() {
    Token token = {
        .type = TOKEN_NAT,
        .line = tokenizer.line
    };

    return token;
}

static inline char peek() {
    return *tokenizer.current;
}

static inline char peekNext() {
    if (peek() == '\0') return '\0';
    return tokenizer.current[1];
}

static inline char eat() {
    return *tokenizer.current++;
}

static inline char puke() {
    return *--tokenizer.current;
}

static inline bool isDigit(char c) {
    return c >= '0' && c <= '9';
}

static inline bool isAlpha(char c) {
    return (c >= 'a' && c <= 'z') ||
           (c >= 'A' && c <= 'Z') ||
            c == '_';
}

static inline bool isAlphaNumeric(char c) {
    return isAlpha(c) || isDigit(c);
}

static bool match(char expected) {
    if (peek() == '\0') return false;
    if (peek() != expected) return false;

    eat();
    return true;
}

static TokenType checkKeyword(
    u32 start, u32 length, const char* rest, TokenType type) {
    if (tokenizer.current - tokenizer.start == start + length &&
        memcmp(tokenizer.start + start, rest, length) == 0) {
        return type;
    }

    return TOKEN_IDENTIFIER;
}

static Token skipComment() {
	// Block comment #[ ... ]#
	if (peek() != '\0' && match('[')) {
		bool terminated = match(']') && match('#');
		while (peek() != '\0' && !terminated) {
            terminated = match(']') && match('#');
            if (!terminated) {
                if (peek() == '\n') tokenizer.line++;
                eat();
            }
		}
        if (!terminated)
            return errorToken("Unterminated #[ comment.");
	} else { // Single line comment # ...
		while (peek() != '\0' && peek() != '\n') {
            eat();
        }
	}

	return notAToken();
}

static Token skipWhitespace() {
    for ever {
        switch(eat()) {
            case ' ':
            case '\r':
            case '\t':
                break;
            case '\n':
                tokenizer.line++;
                break;
            case '#': {
                Token tok = skipComment();
                if (tok.type == TOKEN_ERROR)
                    return tok;
                break;
            }
            default:
                puke();
                return notAToken();
        }
    }
}

static Token scanString() {
    for (char c = peek(); c != '"' && c != '\0'; c = eat()) {
        if (c == '\n') tokenizer.line++;
    }

    if (peek() == '\0') 
        return errorToken("Unterminated string.");

    eat();
    return makeToken(TOKEN_STRING);
}

static TokenType identifierType() {
    switch (tokenizer.start[0]) {
        case 'a': return checkKeyword(1, 2, "nd", TOKEN_AND);
        case 'b': return checkKeyword(1, 4, "reak", TOKEN_BREAK);
        case 'c': 
            if (tokenizer.current - tokenizer.start >= 2) {
                switch (tokenizer.start[1]) {
                    case 'l': return checkKeyword(2, 3, "ass", TOKEN_CLASS);
                    case 'y': return checkKeyword(2, 3, "cle", TOKEN_CYCLE);
                }
            }
            break;
        case 'e': return checkKeyword(1, 3, "lse", TOKEN_ELSE);
        case 'f':
            if (tokenizer.current - tokenizer.start >= 2) {
                switch (tokenizer.start[1]) {
                    case 'a': return checkKeyword(2, 3, "lse", TOKEN_FALSE);
                    case 'o': return checkKeyword(2, 1, "r", TOKEN_FOR);
                    case 'u': return checkKeyword(2, 1, "n", TOKEN_FUN);
                }
            }
            break;
        case 'i': return checkKeyword(1, 1, "f", TOKEN_IF);
        case 'n': 
            if (tokenizer.current - tokenizer.start >= 2) {
                switch (tokenizer.start[1]) {
                    case 'i': return checkKeyword(2, 1, "l", TOKEN_NIL);
                    case 'o': return checkKeyword(2, 1, "t", TOKEN_NOT);
                }
            }
            break;
        case 'o': return checkKeyword(1, 1, "r", TOKEN_OR);
        case 'p': return checkKeyword(1, 4, "rint", TOKEN_PRINT);
        case 'r': return checkKeyword(1, 5, "eturn", TOKEN_RETURN);
        case 's': return checkKeyword(1, 4, "uper", TOKEN_SUPER);
        case 't':
            if (tokenizer.current - tokenizer.start >= 2) {
                switch (tokenizer.start[1]) {
                    case 'h': return checkKeyword(2, 2, "is", TOKEN_THIS);
                    case 'r': return checkKeyword(2, 2, "ue", TOKEN_TRUE);
                }
            }
            break;
        case 'v': return checkKeyword(1, 2, "ar", TOKEN_VAR);
        case 'w': return checkKeyword(1, 4, "hile", TOKEN_WHILE);
    }

    return TOKEN_IDENTIFIER;
}

static Token scanIdentifier() {
    while (isAlpha(peek()) || isDigit(peek())) eat();
    return makeToken(identifierType());
}

static Token scanNumber() {
    while (isDigit(peek())) eat();

    if (peek() == '.' && isDigit(peekNext())) {
        eat(); // eat the '.'

        while (isDigit(peek())) eat();
    }

    return makeToken(TOKEN_NUMBER);
}

Token scanToken() {
    Token tok = skipWhitespace();
    tokenizer.start = tokenizer.current;
    if (tok.type == TOKEN_ERROR)
        return tok;


    if (peek() == '\0')
        return makeToken(TOKEN_EOF);

    char c = eat();
    switch (c) {
        case '(': return makeToken(TOKEN_LEFT_PAREN);
        case ')': return makeToken(TOKEN_RIGHT_PAREN);
        case '{': return makeToken(TOKEN_LEFT_BRACE);
        case '}': return makeToken(TOKEN_RIGHT_BRACE);
        case ';': return makeToken(TOKEN_SEMICOLON);
        case '.': return makeToken(TOKEN_DOT);
        case ',': return makeToken(TOKEN_COMMA);
        case ':': return makeToken(TOKEN_COLON);
        case '?': return makeToken(TOKEN_QUESTION_MARK);
        case '-': 
            return makeToken(match('-') ? TOKEN_MINUS_MINUS
                           : match('=') ? TOKEN_MINUS_EQUAL
                                        : TOKEN_MINUS);
        case '+':
            return makeToken(match('+') ? TOKEN_PLUS_PLUS
                           : match('=') ? TOKEN_PLUS_EQUAL
                                        : TOKEN_PLUS);
        case '/':
            return makeToken(match('=') ? TOKEN_SLASH_EQUAL : TOKEN_SLASH);
        case '*':
            return makeToken(match('=') ? TOKEN_STAR_EQUAL : TOKEN_STAR);
        case '!': // I use 'not' instead of '!' for logical negation
            return match('=') ? makeToken(TOKEN_BANG_EQUAL)
                              : errorToken("Unexpected character.");
        case '=':
            return makeToken(match('=') ? TOKEN_EQUAL_EQUAL : TOKEN_EQUAL);
        case '<':
            return makeToken(match('=') ? TOKEN_LESS_EQUAL : TOKEN_LESS);
        case '>':
            return makeToken(match('=') ? TOKEN_GREATER_EQUAL : TOKEN_GREATER);
        case '"':
            return scanString();
    }

    if (isAlpha(c)) return scanIdentifier();
    if (isDigit(c)) return scanNumber();

    return errorToken("Unexpected character.");
}
