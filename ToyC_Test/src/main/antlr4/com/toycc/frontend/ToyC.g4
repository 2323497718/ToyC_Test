grammar ToyC;

// ===== Lexer Rules =====

// Keywords
CONST: 'const';
INT: 'int';
VOID: 'void';
IF: 'if';
ELSE: 'else';
WHILE: 'while';
BREAK: 'break';
CONTINUE: 'continue';
RETURN: 'return';

// Operators
LE: '<=';
GE: '>=';
EQ: '==';
NE: '!=';
LAND: '&&';
LOR: '||';

// Identifiers
ID: [_A-Za-z][_A-Za-z0-9]*;

// Numbers (supports negative numbers as unary expression)
NUMBER: '-'? ('0' | [1-9][0-9]*);

// Comments
COMMENT: '//' ~[\r\n]* -> skip;
MULTILINE_COMMENT: '/*' .*? '*/' -> skip;

// Whitespace
WS: [ \t\r\n]+ -> skip;

// ===== Parser Rules =====

compUnit
    : (decl | funcDef)+
    ;

decl
    : constDecl
    | varDecl
    ;

constDecl
    : CONST INT ID '=' expr ';'
    ;

varDecl
    : INT ID '=' expr ';'
    ;

funcDef
    : (INT | VOID) ID '(' paramListOpt ')' block
    ;

paramListOpt
    : paramList?
    ;

paramList
    : INT ID (',' INT ID)*
    ;

block
    : '{' stmt* '}'
    ;

stmt
    : block
    | ';'
    | expr ';'
    | ID '=' expr ';'
    | decl
    | IF '(' expr ')' stmt (ELSE stmt)?
    | WHILE '(' expr ')' stmt
    | BREAK ';'
    | CONTINUE ';'
    | RETURN ';'
    | RETURN expr ';'
    ;

expr
    : lorExpr
    ;

lorExpr
    : landExpr (LOR landExpr)*
    ;

landExpr
    : relExpr (LAND relExpr)*
    ;

relExpr
    : addExpr ((LT | GT | LE | GE | EQ | NE) addExpr)*
    ;

addExpr
    : mulExpr ((PLUS | MINUS) mulExpr)*
    ;

mulExpr
    : unaryExpr ((MUL | DIV | MOD) unaryExpr)*
    ;

unaryExpr
    : (PLUS | MINUS | NOT) unaryExpr
    | primaryExpr
    ;

primaryExpr
    : ID
    | NUMBER
    | '(' expr ')'
    | ID '(' argListOpt ')'
    ;

argListOpt
    : argList?
    ;

argList
    : expr (',' expr)*
    ;

// Fragment rules for lexer
fragment LT: '<';
fragment GT: '>';
fragment PLUS: '+';
fragment MINUS: '-';
fragment MUL: '*';
fragment DIV: '/';
fragment MOD: '%';
fragment NOT: '!';
