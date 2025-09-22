#ifndef clox_tokenizer_h
#define clox_tokenizer_h

typedef enum {
    // Single character tokens
	TOKEN_LEFT_PAREN, TOKEN_RIGHT_PAREN,
	TOKEN_LEFT_BRACKET, TOKEN_RIGHT_BRACKET,
	TOKEN_LEFT_BRACE, TOKEN_RIGHT_BRACE,

	TOKEN_SEMICOLON, TOKEN_COMMA, TOKEN_DOT,

	// Math operators
	TOKEN_MINUS, TOKEN_PLUS, TOKEN_SLASH, TOKEN_STAR,

	// Assignment
	TOKEN_EQUAL,

	// PREFIX/POSTFIX MATH OPS (ADDED)
	TOKEN_MINUS_MINUS, TOKEN_PLUS_PLUS,

	TOKEN_MINUS_EQUAL, TOKEN_PLUS_EQUAL, TOKEN_SLASH_EQUAL, TOKEN_STAR_EQUAL,

	// Comparison operators
	TOKEN_EQUAL_EQUAL, TOKEN_BANG_EQUAL,
	TOKEN_LESS, TOKEN_LESS_EQUAL,
	TOKEN_GREATER, TOKEN_GREATER_EQUAL,

    // Ternary operators
	TOKEN_COLON,
	TOKEN_QUESTION_MARK,

	// Boolean keywords
	// NOTE: Lox officially uses BANG instead of NOT
	TOKEN_NOT, TOKEN_AND, TOKEN_OR,
	// nil keyword
	TOKEN_NIL,
	// Boolean keywords
	TOKEN_TRUE, TOKEN_FALSE,
	// Control Flow keywords
	TOKEN_IF, TOKEN_ELSE,
	TOKEN_WHILE, TOKEN_FOR,
    TOKEN_FUN,

    // Flow breakers
	TOKEN_BREAK, TOKEN_CYCLE, TOKEN_RETURN,
	
	// OOP Keywords
	TOKEN_CLASS, TOKEN_SUPER, TOKEN_THIS,
	// Variable declaration keyword
	TOKEN_VAR,
	// Misc. keyword(s)
	TOKEN_PRINT,

	// Literals
	TOKEN_IDENTIFIER, TOKEN_NUMBER, TOKEN_STRING, TOKEN_LIST,

    // TOKEN_NAT == NOT_A_TOKEN (Yes, I know that's an oxymoron)
	TOKEN_ERROR, TOKEN_NAT, TOKEN_EOF
} TokenType;

typedef struct {
    TokenType type;
    const char* start;
    int length;
    int line;
} Token;

void initTokenizer(const char* source);
Token scanToken(void);

#endif
