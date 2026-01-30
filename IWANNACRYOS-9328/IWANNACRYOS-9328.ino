#include "ILI.h"
#include <avr/pgmspace.h>
#include "TouchScreen.h"

int XP = 7, YP = A2, XM = A1, YM = 6;

TouchScreen ts(XP, YP, XM, YM, 300); 
TSPoint p = ts.getPoint();

enum WINDOWS {
  WIN_MAIN,
  WIN_CALC,
  WIN_GRAP,
  WIN_PAIN,
  WIN_NOTE,
};

uint8_t keyset;

WINDOWS curwin = WIN_GRAP;
uint8_t selwin = 2;

bool static ranmain = false;
bool static rancalc = false;
bool static ranpaint = false;
bool static rangraph = false;
bool static rannote = false;

const int16_t sine_table_256[256] PROGMEM = {
    0, 6, 13, 19, 25, 31, 38, 44, 50, 56, 62, 68, 74, 80, 86, 92,
    98, 104, 109, 115, 121, 126, 132, 137, 142, 147, 152, 157, 162, 167, 172, 177,
    181, 185, 190, 194, 198, 202, 206, 209, 213, 216, 220, 223, 226, 229, 231, 234,
    237, 239, 241, 243, 245, 247, 248, 250, 251, 252, 253, 254, 255, 255, 256, 256,
    256, 256, 256, 255, 255, 254, 253, 252, 251, 250, 248, 247, 245, 243, 241, 239,
    237, 234, 231, 229, 226, 223, 220, 216, 213, 209, 206, 202, 198, 194, 190, 185,
    181, 177, 172, 167, 162, 157, 152, 147, 142, 137, 132, 126, 121, 115, 109, 104,
    98, 92, 86, 80, 74, 68, 62, 56, 50, 44, 38, 31, 25, 19, 13, 6,
    0, -6, -13, -19, -25, -31, -38, -44, -50, -56, -62, -68, -74, -80, -86, -92,
    -98, -104, -109, -115, -121, -126, -132, -137, -142, -147, -152, -157, -162, -167, -172, -177,
    -181, -185, -190, -194, -198, -202, -206, -209, -213, -216, -220, -223, -226, -229, -231, -234,
    -237, -239, -241, -243, -245, -247, -248, -250, -251, -252, -253, -254, -255, -255, -256, -256,
    -256, -256, -256, -255, -255, -254, -253, -252, -251, -250, -248, -247, -245, -243, -241, -239,
    -237, -234, -231, -229, -226, -223, -220, -216, -213, -209, -206, -202, -198, -194, -190, -185,
    -181, -177, -172, -167, -162, -157, -152, -147, -142, -137, -132, -126, -121, -115, -109, -104,
    -98, -92, -86, -80, -74, -68, -62, -56, -50, -44, -38, -31, -25, -19, -13, -6
};

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
  NODE_NUM,  // numbers
  NODE_UNA,

  // Binary operators
  NODE_ADD,  // +
  NODE_SUB,  // -
  NODE_MUL,  // *
  NODE_DIV,  // /
  NODE_EXP,  // ^

  // Functions
  NODE_SIN,
  NODE_COS,
  NODE_TAN,
  NODE_FRA,
  NODE_FLO,
  NODE_CEI
};
char input[256] = "32.25";
char output[64];

  uint8_t TokIndex = 0;
  uint8_t NodIndex;

#define FACTOR 256

typedef struct {
  TOKTYPE typ;
  int32_t val;
} Token;

Token tokens[64];

typedef struct {
    NodeType typ;
    int32_t val;
    int8_t lnode;  // Index of the left node in the array (-1 if none)
    int8_t rnode;  // Index of the right node in the array (-1 if none)
} Node;

Node nodes[32]; // Your "Pool" of 32 nodes
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
  TokIndex = 0;
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
      tokens[TokIndex].typ = TOK_NULL;
      tokens[TokIndex].val = 0;
      p++;
    }
  }

  // Add sentinel token
  tokens[TokIndex].typ = TOK_NULL;
  tokens[TokIndex].val = 0;
  TokIndex=0;
}

int a;

void Parser() {

    a = parseExpression();
    
}


int parseExpression() {
  uint32_t l = parseFactor();
  while (tokens[TokIndex].typ == OP_ADD || tokens[TokIndex].typ == OP_SUB) {
    NodeType a = (tokens[TokIndex].typ == OP_ADD) ? NODE_ADD : NODE_SUB;
    TokIndex++;
    uint8_t opIdidx = NodIndex++;
    nodes[opIdidx].lnode = l;
    nodes[opIdidx].rnode = parseFactor();
    nodes[opIdidx].typ = a;
    l = opIdidx;
  }
  return l;
  
}

int parseFactor() {
  uint32_t l = parseUnary();
  while (tokens[TokIndex].typ == OP_MUL || tokens[TokIndex].typ == OP_DIV) {
    NodeType a = (tokens[TokIndex].typ == OP_MUL) ? NODE_MUL : NODE_DIV;
    TokIndex++;
    uint8_t opIdidx = NodIndex++;
    nodes[opIdidx].lnode = l;
    nodes[opIdidx].rnode = parseUnary();
    nodes[opIdidx].typ = a;
    l = opIdidx;
  }
  return l;
}

int parsePower() {
  uint32_t l = parsePrimary();
  if (tokens[TokIndex].typ == OP_EXP) {
    TokIndex++;
    uint8_t opIdidx = NodIndex++;
    nodes[opIdidx].typ = NODE_EXP;
    nodes[opIdidx].lnode = l;
    nodes[opIdidx].rnode = parsePower();
    return opIdidx;
  }
  return l;
}

int parseUnary() {
  
  if (tokens[TokIndex].typ == OP_SUB) {
    int8_t opIdx = NodIndex++;
    TokIndex++; 
    
    nodes[opIdx].typ = NODE_UNA;
    nodes[opIdx].lnode = -1; 
    nodes[opIdx].rnode = parseUnary(); 
    return opIdx;
  }
  
  return parsePower(); 
}

int parseFunction() {
    TOKTYPE tok = tokens[TokIndex].typ;
    if (tok >= F_SIN && tok <= F_CEI) {
        int idx = NodIndex++;
        nodes[idx].lnode = -1;

        switch(tok) {
            case F_SIN: nodes[idx].typ = NODE_SIN; break;
            case F_COS: nodes[idx].typ = NODE_COS; break;
            case F_TAN: nodes[idx].typ = NODE_TAN; break;
            case F_FRA: nodes[idx].typ = NODE_FRA; break;
            case F_FLO: nodes[idx].typ = NODE_FLO; break;
            case F_CEI: nodes[idx].typ = NODE_CEI; break;
        }

        TokIndex++;
        if (tokens[TokIndex].typ == L_PAREN) {
            TokIndex++;
            nodes[idx].rnode = parseExpression();
          if (nodes[idx].rnode == -1) nodes[idx].rnode = parsePrimary();

          if (tokens[TokIndex].typ == R_PAREN) TokIndex++;
        } 
        else {
            nodes[idx].rnode = parseFactor();
        }
        return idx;
    }
    return -1;
}


int parsePrimary() {
  if (tokens[TokIndex].typ == L_PAREN) {
    TokIndex++;
    int val = parseExpression();
    TokIndex++;
    return val;
  }
  if (tokens[TokIndex].typ == NUM) {
    nodes[NodIndex].typ = NODE_NUM;
    nodes[NodIndex].val = tokens[TokIndex++].val ;
    nodes[NodIndex].lnode = -1;
    nodes[NodIndex].rnode = -1;
    return NodIndex++;
  }
  int val = parseFunction();
  return (val==-1) ? -1 : val;
  
}


int32_t evaluate(int idfa) {
  if (idfa==-1) {
    return 0;
  }
  Node& n = nodes[idfa];
  switch (n.typ) {
    case NODE_NUM: return n.val;
    case NODE_ADD: return evaluate(n.lnode) + evaluate(n.rnode);
    case NODE_SUB: return evaluate(n.lnode) - evaluate(n.rnode);
    case NODE_UNA: return -evaluate(n.rnode);
    case NODE_MUL: {
      int64_t res;
      res = (int64_t)evaluate(n.lnode) * evaluate(n.rnode);
      if (res > INT32_MAX) return INT32_MAX;
      if (res < INT32_MIN) return INT32_MIN;
      return (int32_t)res >> 8;
    }
    case NODE_DIV: {
      int64_t res;
      if (evaluate(n.rnode) == 0) {
        return 0;
      }
      res = (int64_t)(evaluate(n.lnode) << 8) / evaluate(n.rnode);
      if (res > INT32_MAX) return INT32_MAX;
      if (res < INT32_MIN) return INT32_MIN;
      return (int32_t)res;
    }
    case NODE_EXP: {
      int32_t base= evaluate(n.lnode);
      int32_t r=evaluate(n.rnode);

      r = r>>8;

      if (r==0) {
      return 256;
      }

      int64_t res = 256;
      for(int i=1;i<=r;i++) {
        res *= base;
        res = res >> 8; 

      }
      if (res > INT32_MAX) return INT32_MAX;
      if (res < INT32_MIN) return INT32_MIN;
      return (int32_t)res;
    }
    case NODE_SIN: {
      uint8_t indx = ((((((evaluate(n.rnode) >> 8)%360) +360) % 360) * 32/45) & 0xFF);
      return (int32_t)(int16_t)pgm_read_word(&sine_table_256[indx]);
    }
    case NODE_COS: {
      uint8_t indx = (((((((evaluate(n.rnode) >> 8)%360) +360) % 360) * 32/45)+64) & 0xFF);
      return (int32_t)(int16_t)pgm_read_word(&sine_table_256[indx]);
    }
    case NODE_TAN: {
      uint8_t input = ((((((evaluate(n.rnode) >> 8)%360) +360) % 360) * 32/45) & 0xFF); 
      uint8_t indx = (((((((evaluate(n.rnode) >> 8)%360) +360) % 360) * 32/45)+64) & 0xFF);
      int64_t sin = (int64_t)(int16_t)pgm_read_word(&sine_table_256[input]);
      int64_t cos = (int64_t)(int16_t)pgm_read_word(&sine_table_256[indx]);
      return (cos==0) ? (2147483647L >> 8) : (int32_t)(int16_t)((sin << 8 )/cos);
    }
    case NODE_FRA: {
      return ((evaluate(n.rnode) & 0xFF));
    }
    case NODE_CEI: {
      int funval = evaluate(n.rnode);
      if ((funval % 256)==0) {
       return funval;
      }
      return funval - (funval % 256) + 256;
    }
    case NODE_FLO: {
      int funval = evaluate(n.rnode);
      return funval - (funval % 256);
    }

    
  }
  return 0;
}

void printFixed(int32_t val) {
  char buf[32];
  int32_t intPart = val / FACTOR;
  int32_t fracPart = val % FACTOR;
  
  if (fracPart < 0) fracPart = -fracPart;  // Make fraction positive
  
  itoa(intPart, buf, 10);
  tftPrint(buf);
  
  if (fracPart > 0) {
    tftPrint(".");
    int decimal = (fracPart * 100) / FACTOR;
    if (decimal < 10) tftPrint("0");
    itoa(decimal, buf, 10);
    tftPrint(buf);
  }
}

void setup() {
  NodIndex=0;
  TokIndex=0;


  tftInit();
  tftFill(BLACK);
  setTextColor(GREEN, BLACK);
  setTextSize(2);
  setCursor(0, 15);
  delay(100);
  
  Tokenizer(input);
  Parser();
}

void loop() {
  handleEvents();
  update();
  render();
}

void handleEvents() {

}

void update() {

}

void render() {
  if (curwin == WIN_MAIN) {
    if(ranmain) return;

    tftFill(BLACK);

    fillRect(2, 2, 316, 2, WHITE);
    fillRect(2, 2, 2, 236, WHITE);
    fillRect(2, 236, 316, 2, WHITE);
    fillRect(316, 2, 2, 236, WHITE);

    setCursor(getCursorX()+10, getCursorY());
    tftPrintChar( (selwin == WIN_CALC) ? (char)(0x3E) : (char)(0x20) );
    tftPrintln("Calculator");
    setCursor(getCursorX()+10, getCursorY());
    tftPrintChar( (selwin == WIN_GRAP) ? (char)(0x3E) : (char)(0x20) );
    tftPrintln("Grapher");
    setCursor(getCursorX()+10, getCursorY());
    tftPrintChar( (selwin == WIN_PAIN) ? (char)(0x3E) : (char)(0x20) );
    tftPrintln("Paint");
    setCursor(getCursorX()+10, getCursorY());
    tftPrintChar( (selwin == WIN_NOTE) ? (char)(0x3E) : (char)(0x20) );
    tftPrintln("Notepad"); 
    ranmain = true;
  }
  if (curwin == WIN_CALC) {
    if(rancalc) return;

    tftFill(BLACK);
    
    fillRect(2, 2, 316, 2, WHITE);
    fillRect(2, 2, 2, 236, WHITE);
    fillRect(2, 236, 316, 2, WHITE);
    fillRect(316, 2, 2, 236, WHITE);

    fillRect(2, 214, 316, 2, WHITE);

    setCursor(6, 6);
    tftPrint("Input:");
    tftPrint(input);

    setCursor(6, 218);
    tftPrint("Output:");

    Tokenizer(input);
    Parser();
    printFixed(evaluate(a));    

    rancalc = true;
  }
  if (curwin == WIN_GRAP) {
    if(rangraph) return;

    tftFill(BLACK);
    
    fillRect(2, 2, 316, 2, WHITE);
    fillRect(2, 2, 2, 236, WHITE);
    fillRect(2, 236, 316, 2, WHITE);
    fillRect(316, 2, 2, 236, WHITE);

    fillRect(2, 26, 316, 2, WHITE);

    setCursor(6, 6);
    tftPrint("Input:");
    tftPrint(input);

    rangraph = true;
  }
}










