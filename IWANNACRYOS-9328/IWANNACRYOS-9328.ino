#include "ILI.h"
#include <avr/pgmspace.h>

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

char input[256] = "floor(1.29)";  // input
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

      if (r==256) {
      return base;
      }
      if (r<=255) {
      return 0;
      }

      int64_t res = base;
      for(int i=1;i<=(r>>8);i++) {
        res *= base;
        res = res >> 8; 

      }
      if (res > INT32_MAX) return INT32_MAX;
      if (res < INT32_MIN) return INT32_MIN;
      return (int32_t)res;
    }
    case NODE_SIN: {
      return ((int16_t)(pgm_read_word(&sine_table_256[((evaluate(n.rnode)*256 ) /(360 << 8)) & 0xFF ])) );
    }
    case NODE_COS: {
      return ((int16_t)(pgm_read_word(&sine_table_256[((((evaluate(n.rnode)*256 ) /(360 << 8)) & 0xFF)+64)& 0xFF])) );
    }
    case NODE_TAN: {
      int16_t val = ((int16_t)(pgm_read_word(&sine_table_256[((evaluate(n.rnode) * 256) / (360 << 8)) & 0xFF]) << 8)) / (((int16_t)(pgm_read_word(&sine_table_256[((((evaluate(n.rnode) * 256) / (360 << 8)) & 0xFF) + 64) & 0xFF])) == 0) ? 1 : (int16_t)(pgm_read_word(&sine_table_256[((((evaluate(n.rnode) * 256) / (360 << 8)) & 0xFF) + 64) & 0xFF])));
      if (val > INT16_MAX) {
        return INT16_MAX;
      }
      return val;
    }
    case NODE_FRA: {
      return ((evaluate(n.rnode) %256));
    }
    case NODE_CEI: {
      int funval = evaluate(n.rnode);
      return funval - (funval % 256) + 256;
    }
    case NODE_FLO: {
      int funval = evaluate(n.rnode);
      return funval - (funval % 256);
    }

    
  }
  return 0;
}

// Helper to get node type name
const char* getNodeTypeName(NodeType t) {
  switch(t) {
    case NODE_NUM: return "NUM";
    case NODE_UNA: return "UNA";
    case NODE_ADD: return "ADD";
    case NODE_SUB: return "SUB";
    case NODE_MUL: return "MUL";
    case NODE_DIV: return "DIV";
    case NODE_EXP: return "EXP";
    case NODE_SIN: return "SIN";
    case NODE_COS: return "COS";
    case NODE_TAN: return "TAN";
    case NODE_FRA: return "FRA";
    case NODE_FLO: return "FLO";
    case NODE_CEI: return "CEI";
    default: return "???";
  }
}

// Print fixed-point value
void printFixed(int32_t val) {
  char buf[32];
  int32_t intPart = val / FACTOR;
  int32_t fracPart = val % FACTOR;
  
  if (val < 0) {
    intPart = -intPart;
    fracPart = -fracPart;
  }
  
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

// Print AST as table/list
void printAST() {
  tftFill(BLACK);
  setTextColor(WHITE, BLACK);
  setTextSize(1);
  
  setCursor(5, 5);
  tftPrint("Input: ");
  tftPrint(input);
  
  setCursor(5, 20);
  tftPrint("Root node: ");
  char buf[8];
  itoa(a, buf, 10);
  tftPrint(buf);
  
  setCursor(5, 35);
  tftPrint("Total nodes: ");
  itoa(NodIndex, buf, 10);
  tftPrint(buf);
  
  // Print node table
  setCursor(5, 55);
  tftPrint("ID TYPE    VAL      L  R");
  
  int yPos = 70;
  for(int i = 0; i < NodIndex; i++) {
    setCursor(5, yPos);
    
    // Node index
    itoa(i, buf, 10);
    tftPrint(buf);
    if(i < 10) tftPrint(" ");
    
    setCursor(25, yPos);
    // Node type
    tftPrint(getNodeTypeName(nodes[i].typ));
    
    setCursor(70, yPos);
    // Value (if number)
    if(nodes[i].typ == NODE_NUM) {
      printFixed(nodes[i].val);
    } else {
      tftPrint("-");
    }
    
    setCursor(140, yPos);
    // Left child
    if(nodes[i].lnode != -1) {
      itoa(nodes[i].lnode, buf, 10);
      tftPrint(buf);
    } else {
      tftPrint("-");
    }
    
    setCursor(165, yPos);
    // Right child
    if(nodes[i].rnode != -1) {
      itoa(nodes[i].rnode, buf, 10);
      tftPrint(buf);
    } else {
      tftPrint("-");
    }
    
    yPos += 15;
    if(yPos > 220) break; // Don't overflow screen
  }
}

void setup() {
  NodIndex=0;
  TokIndex=0;


  tftInit();
  setTextColor(WHITE, BLACK);
  setTextSize(1);
  delay(100);
  
  Tokenizer(input);
  Parser();
  printAST();
  printFixed(evaluate(a));
}

void loop() {
}