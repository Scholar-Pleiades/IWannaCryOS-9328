#include "ILI.h"

enum TOKTYPE {
  TOK_NULL,
  NUM,
  OP_ADD,
  OP_SUB,
  OP_MUL,
  OP_DIV,
  OP_MOD,
  L_PAREN,
  R_PAREN,
  F_SIN,
  F_COS,
  F_TAN,
  F_FRA,
  F_FLO,
  F_CEI
};

enum NodeType : uint8_t {
    // Literals
    NODE_NUM,       // numbers

    // Binary operators
    NODE_ADD,       // +
    NODE_SUB,       // -
    NODE_MUL,       // *
    NODE_DIV,       // /
    NODE_MOD,       // %

    // Parentheses aren’t nodes themselves—they just affect parsing
    // NODE_LPAREN, NODE_RPAREN  <-- unnecessary in AST

    // Functions
    NODE_SIN,
    NODE_COS,
    NODE_TAN,
    NODE_FRA,
    NODE_FLO,
    NODE_CEI
};

char input[256] = "591.364"; // input 
char output[64];

#define FACTOR 256

typedef struct {
  TOKTYPE typ;
  int32_t val;
} Token;

Token tokens[128];


uint32_t parseNumber(char** p) {
    uint32_t val = 0;

    // Integer part
    while (**p >= '0' && **p <= '9') {
        val = val * 10 + (**p - '0');
        (*p)++;
    }

    // Fractional part
    if (**p == '.') {
        (*p)++;
        uint32_t frac = 0;
        uint32_t div = 10;

        while (**p >= '0' && **p <= '9') {
            frac += ((**p - '0') * FACTOR) / div;
            div *= 10;
            (*p)++;
        }
        val = val * FACTOR + frac;
    } else {
        val *= FACTOR; // integer only
    }

    return val;
}

void Tokenizer(char* exp) {
    uint8_t TokIndex = 0;
    char* p = exp;

    while (*p && TokIndex < 127) { // leave space for sentinel
        while (*p == ' ') p++; // skip spaces

        if (*p >= '0' && *p <= '9') {
            Token temp;
            temp.typ = NUM;
            temp.val = parseNumber(&p);
            tokens[TokIndex++] = temp;
        } else {
            p++; // skip unknown characters for now
        }
    }

    // Add sentinel token
    tokens[TokIndex].typ = TOK_NULL;
    tokens[TokIndex].val = 0;
}




void setup () {
  tftInit();
  tftFill(BLACK);

  Tokenizer(input);
uint32_t val = tokens[0].val;
uint32_t integer_part = val / FACTOR;
uint32_t fractional_part = (val % FACTOR * 100 + FACTOR/2) / FACTOR; // rounds to 0-99

int pos = 0;

// integer digits
int temp = integer_part;
if (temp == 0) output[pos++] = '0';
else {
    int digits[20], dcount = 0;
    while (temp) { digits[dcount++] = temp % 10; temp /= 10; }
    for (int i = dcount-1; i >= 0; i--) output[pos++] = '0' + digits[i];
}

// decimal point
output[pos++] = '.';

int temp2 = fractional_part;
if (temp2 == 0) output[pos++] = '0';
else {
    int digits[10], dcount = 0;
    while (temp2) { digits[dcount++] = temp2 % 10; temp2 /= 10; }
    for (int i = dcount-1; i >= 0; i--) output[pos++] = '0' + digits[i];
}

output[pos] = '\0';

  tftPrintln(output);
  tftPrintln(input);
  int i = 0;
char buf[128];
while (tokens[i].typ != TOK_NULL) {
    sprintf(buf, "%d : %ld", tokens[i].typ, tokens[i].val);
    tftPrintln(buf);
    i++;
}

}

void loop() {
  
}