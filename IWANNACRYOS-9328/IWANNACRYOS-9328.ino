#include "ILI.h"

enum TOKTYPE {
  TOK_NULL,
  NUM,
  OP_ADD,
  OP_SUB,
  OP_MUL,
  OP_DIV,
  OP_EXP,
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
  NODE_NUM,  // numbers

  // Binary operators
  NODE_ADD,  // +
  NODE_SUB,  // -
  NODE_MUL,  // *
  NODE_DIV,  // /
  NODE_EXP,  // ^

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

char input[256] = "32767.36+25-25*25/25^(25)*sin()";  // input
char output[64];

#define FACTOR 256

typedef struct {
  TOKTYPE typ;
  int32_t val;
} Token;

Token tokens[64];

typedef struct{
    NodeType typ;
    int32_t val;
    uint8_t lnodeindex;
    uint8_t rnodeindex;
} Node;

Node nodes[32];

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
    val *= FACTOR;  // integer only
  }

  return val;
}

void Tokenizer(char* exp) {
  uint8_t TokIndex = 0;
  char* p = exp;

  while (*p && TokIndex < 63) {  // leave space for sentinel
    if (*p == ' ') p++;           // skip spaces

    else if (*p >= '0' && *p <= '9') {
      Token temp;
      temp.typ = NUM;
      temp.val = parseNumber(&p);
      tokens[TokIndex++] = temp;
    } else if (*p == '-') {
      Token temp;
      temp.typ = OP_SUB;
      temp.val = 0;
      tokens[TokIndex++] = temp;
      p++;
    } else if (*p == '+') {
      Token temp;
      temp.typ = OP_ADD;
      temp.val = 0;
      tokens[TokIndex++] = temp;
      p++;
    } else if (*p == '/') {
      Token temp;
      temp.typ = OP_DIV;
      temp.val = 0;
      tokens[TokIndex++] = temp;
      p++;
    } else if (*p == '*') {
      Token temp;
      temp.typ = OP_MUL;
      temp.val = 0;
      tokens[TokIndex++] = temp;
      p++;
    } else if (*p == '^') {
      Token temp;
      temp.typ = OP_EXP;
      temp.val = 0;
      tokens[TokIndex++] = temp;
      p++;
    }

    else if (*p == '(') {
      Token temp;
      temp.typ = L_PAREN;
      temp.val = 0;
      tokens[TokIndex++] = temp;
      p++;
    } else if (*p == ')') {
      Token temp;
      temp.typ = R_PAREN;
      temp.val = 0;
      tokens[TokIndex++] = temp;
      p++;
    } else if (*p >= 'a' && *p <= 'z') {
      char funco[6];
      int i = 0;

      while (*p >= 'a' && *p <= 'z' && i < 5) {
        funco[i++] = *p++;
      }
      funco[i] = '\0';

      if (strcmp(funco, "sin") == 0) tokens[TokIndex++] = { F_SIN, 0 };
      else if (strcmp(funco, "cos") == 0) tokens[TokIndex++] = { F_COS, 0 };
      else if (strcmp(funco, "tan") == 0) tokens[TokIndex++] = { F_TAN, 0 };
      else if (strcmp(funco, "floor") == 0) tokens[TokIndex++] = { F_FLO, 0 };
      else if (strcmp(funco, "ceil") == 0) tokens[TokIndex++] = { F_CEI, 0 };
      else if (strcmp(funco, "fract") == 0) tokens[TokIndex++] = { F_FRA, 0 };
    }

    else {
      p++;
    }
  }

  // Add sentinel token
  tokens[TokIndex].typ = TOK_NULL;
  tokens[TokIndex].val = 0;
}

void Parser() {
    uint8_t TokIndex = 0;
    uint8_t NodIndex;
    parseTerm(TokIndex, NodIndex);
}


void parseTerm(uint8_t TokIndex, uint8_t NodIndex) {

}

void parseFactor(uint8_t TokIndex, uint8_t NodIndex) {

}

void parseExp(uint8_t TokIndex, uint8_t NodIndex) {

}

void parseExpression(uint8_t TokIndex, uint8_t NodIndex) {

}


void setup() {
  tftInit();
  tftFill(BLACK);

  Tokenizer(input);
  uint32_t val = tokens[0].val;
  uint32_t integer_part = val / FACTOR;
  uint32_t fractional_part = (val % FACTOR * 100 + FACTOR / 2) / FACTOR;  // rounds to 0-99

  int pos = 0;

  // integer digits
  int temp = integer_part;
  if (temp == 0) output[pos++] = '0';
  else {
    int digits[20], dcount = 0;
    while (temp) {
      digits[dcount++] = temp % 10;
      temp /= 10;
    }
    for (int i = dcount - 1; i >= 0; i--) output[pos++] = '0' + digits[i];
  }

  // decimal point
  output[pos++] = '.';

  int temp2 = fractional_part;
  if (temp2 == 0) output[pos++] = '0';
  else {
    int digits[10], dcount = 0;
    while (temp2) {
      digits[dcount++] = temp2 % 10;
      temp2 /= 10;
    }
    for (int i = dcount - 1; i >= 0; i--) output[pos++] = '0' + digits[i];
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