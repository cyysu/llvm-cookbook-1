#include <string>
#include <vector>

/**
 * 词法分析
 */

enum Token_type {
    EOF_TOKEN = 0,
    NUMERIC_TOKEN,
    IDENTIFIER_TOKEN,
    PARAN_TOKEN,
    DEF_TOKEN
};

static int Numeric_Val;

static std::string Identifier_string;

static int get_token() {
    static int LastChar = ' ';

    while (isspace(LastChar))
        LastChar = fgetc(file);

    if (isalpha(LastChar)) {
        Identifier_string = LastChar;
        while (isalnum((LastChar = fgetc(file))))
            Identifier_string += LastChar;

        if (Identifier_string == "def")
            return DEF_TOKEN;

        return IDENTIFIER_TOKEN;
    }

    if (isdigit(LastChar)) {
        stad::string NumStr;
        do {
            NumStr += LastChar;
            LastChar = fgetc(file);
        } while (isdigit(LastChar));

        Numeric_Val = strtod(NumStr.c_str(), 0);
        return NUMERIC_TOKEN;
    }

    if (LastChar == '#') {
        do
            LastChar = fgetc(file);
        while (LastChar != EOF && LastChar != '\n' && LastChar != '\r');

        if (LastChar != EOF)
            return get_token();
    }

    if (LastChar == EOF)
        return EOF_TOKEN;

    int ThisChar = LastChar;
    LastChar = fgetc(file);
    return ThisChar;
}

/**
 * 抽象语法树定义
 */

class BaseAST {
public:
    virtual ~BaseAST();
};

class VariableAST : public BaseAST {
    std::string Var_Name;
public:
    VariableAST (std::string &name) : Var_Name(name) {}
};

class NumericAST : public BaseAST {
    int numeric_val;
public:
    NumericAST (int val) : numeric_val(val) {}
};

class BinaryAST : public BaseAST {
    std::string Bin_Operator;
    BaseAST *LHS, *RHS;
public:
    BinaryAST (std::string op, BaseAST *lhs, BaseAST *rhs) :
    Bin_Operator(op), LHS(lhs), RHS(rhs) {}
};

class FunctionDeclAST {
    std::string Func_Name;
    std::vector<std::string> Arguments;
public:
    FunctionDeclAST (const std::string &name, const std::vector<std::string> &args) :
    Func_Name(name), Arguments(args) {}
};

class FunctionDefnAST {
    FunctionDeclAST *Func_Decl;
    BaseAST *Body;
public:
    FunctionDefnAST (FunctionDeclAST *proto, BaseAST *body) :
    Func_Decl(proto), Body(body) {}
};

class FunctionCallAST : public BaseAST {
    std::string Function_Callee;
    std::vector<BaseAST*> Function_Arguments;
public:
    FunctionCallAST (const std::string &callee, std::vector<BaseAST*> &args) :
    Function_Callee(callee), Function_Arguments(args) {}
};

/**
 * 语法分析器
 */
static int Current_token;

static void next_token() {
  Current_token = get_token();
}

static BaseAST* base_Parser() {
  switch (Current_token) {
    default: return 0;
    case IDENTIFIER_TOKEN: return identifier_parser();
    case NUMERIC_TOKEN: return numeric_parser();
    case '(': return paran_parser();
  }
}

/**
 * 解析简单表达式
 */
static BaseAST *numeric_parser() {
  BaseAST *Result = new NumericAST(Numeric_Val);
  next_token();
  return Result;
}

static BaseAST* identifier_parser() {
  std::string IdName = Identifier_string;

  next_token();

  if (Current_token != '(')
  return new VariableAST(IdName);

  next_token();

  std::vector<BaseAST*> Args;
  if (Current_token != ')') {
    while (1) {
      BaseAST* Arg = expression_parser();
      if (!Arg) return 0;
      Args.push_back(Arg);

      if (Current_token == ')') break;

      if (Current_token != ',')
      return 0;
      next_token();
    }
  }
  next_token();

  return new FunctionCallAST(IdName, Args);
}
