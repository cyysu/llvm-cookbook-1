#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Transforms/Scalar.h"
#include <string>
#include <vector>

using namespace llvm;
using namespace llvm::legacy;

/*******************************************************
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

static Module *Module_Ob;
static IRBuilder<> Builder(getGlobalContext());
static std::map<std::string, Value *> Named_Values;

/*******************************************************************
 * 增加IR优化
 */
static FunctionPassManager *Global_FP;

static FILE *file;

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
                std::string NumStr;
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

/***********************************************************
 * 抽象语法树定义
 */

class BaseAST {
public:
virtual ~BaseAST();
virtual Value *Codegen() = 0;
};

class VariableAST : public BaseAST {
std::string Var_Name;

public:
VariableAST(std::string &name) : Var_Name(name) {
}
virtual Value *Codegen();
};

class NumericAST : public BaseAST {
int numeric_val;

public:
NumericAST(int val) : numeric_val(val) {
}
virtual Value *Codegen();
};

class BinaryAST : public BaseAST {
std::string Bin_Operator;
BaseAST *LHS, *RHS;

public:
BinaryAST(std::string op, BaseAST *lhs, BaseAST *rhs)
        : Bin_Operator(op), LHS(lhs), RHS(rhs) {
}
};

class FunctionDeclAST {
std::string Func_Name;
std::vector<std::string> Arguments;

public:
FunctionDeclAST(const std::string &name, const std::vector<std::string> &args)
        : Func_Name(name), Arguments(args) {
}
};

class FunctionDefnAST {
FunctionDeclAST *Func_Decl;
BaseAST *Body;

public:
FunctionDefnAST(FunctionDeclAST *proto, BaseAST *body)
        : Func_Decl(proto), Body(body) {
}
virtual Function *Codegen();
};

class FunctionCallAST : public BaseAST {
std::string Function_Callee;
std::vector<BaseAST *> Function_Arguments;

public:
FunctionCallAST(const std::string &callee, std::vector<BaseAST *> &args)
        : Function_Callee(callee), Function_Arguments(args) {
}
};

/*************************************************************
 * 语法分析器
 */

static BaseAST *identifier_parser();
static BaseAST *numeric_parser();
static BaseAST *paran_parser();

static int Current_token;

static void next_token() {
        Current_token = get_token();
}

static BaseAST *Base_Parser() {
        switch (Current_token) {
        default:
                return 0;
        case IDENTIFIER_TOKEN:
                return identifier_parser();
        case NUMERIC_TOKEN:
                return numeric_parser();
        case '(':
                return paran_parser();
        }
}

/*****************************************************************
 * 解析简单表达式
 */
static BaseAST *binary_op_parser(int Old_Prec, BaseAST *LHS);
static BaseAST *expression_parser();

static BaseAST *numeric_parser() {
        BaseAST *Result = new NumericAST(Numeric_Val);
        next_token();
        return Result;
}

static BaseAST *identifier_parser() {
        std::string IdName = Identifier_string;

        next_token();

        if (Current_token != '(')
                return new VariableAST(IdName);

        next_token();

        std::vector<BaseAST *> Args;
        if (Current_token != ')') {
                while (1) {
                        BaseAST *Arg = expression_parser();
                        if (!Arg)
                                return 0;
                        Args.push_back(Arg);

                        if (Current_token == ')')
                                break;

                        if (Current_token != ',')
                                return 0;
                        next_token();
                }
        }
        next_token();

        return new FunctionCallAST(IdName, Args);
}

static FunctionDeclAST *func_decl_parser() {
        if (Current_token != IDENTIFIER_TOKEN) {
                return 0;
        }

        std::string FnName = Identifier_string;
        next_token();

        if (Current_token != '(') {
                return 0;
        }

        std::vector<std::string> Function_Argument_Names;
        while (next_token() == IDENTIFIER_TOKEN) {
                Function_Argument_Names.push_back(Identifier_string);
        }
        if (Current_token != ')') {
                return 0;
        }
        next_token();

        return new FunctionDeclAST(FnName, Function_Argument_Names);
}

static FunctionDefnAST *func_defn_parser() {
        next_token();
        FunctionDeclAST *Decl = func_decl_parser();
        if (Decl == 0)
                return 0;

        if (BaseAST *Body = expression_parser())
                return new FunctionDefnAST(Decl, Body);
        return 0;
}

static BaseAST *expression_parser() {
        BaseAST *LHS = Base_Parser();
        if (!LHS)
                return 0;
        return binary_op_parser(0, LHS);
}

/*********************************************************************
 * 解析二元表达式
 */
static std::map<char, int> Operator_Precedence;

static void init_precedence() {
        Operator_Precedence['-'] = 1;
        Operator_Precedence['+'] = 2;
        Operator_Precedence['/'] = 3;
        Operator_Precedence['*'] = 4;
}

static int getBinOpPrecedence() {
        if (!isascii(Current_token))
                return -1;

        int TokPrec = Operator_Precedence[Current_token];
        if (TokPrec <= 0)
                return -1;
        return TokPrec;
}

static BaseAST *binary_op_parser(int Old_Prec, BaseAST *LHS) {
        while (1) {
                int Operator_Prec = getBinOpPrecedence();
                if (Operator_Prec < Old_Prec)
                        return LHS;

                int BinOp = Current_token;
                next_token();

                BaseAST *RHS = Base_Parser();
                if (!RHS)
                        return 0;

                int Next_Prec = getBinOpPrecedence();
                if (Operator_Prec < Next_Prec) {
                        RHS = binary_op_parser(Operator_Prec + 1, RHS);
                        if (RHS == 0)
                                return 0;
                }

                LHS = new BinaryAST(std::to_string(BinOp), LHS, RHS);
        }
}

static BaseAST *paran_parser() {
        next_token();
        BaseAST *V = expression_parser();
        if (!V)
                return 0;

        if (Current_token != ')')
                return 0;
        return V;
}

static void HandleDefn() {
        if (FunctionDefnAST *F = func_defn_parser()) {
                if (Function *LF = F->Codegen()) {
                        /* code */
                }
        } else {
                next_token();
        }
}

static void HandleTopExpression() {
        if (FunctionDefnAST *F = top_level_parser()) {
                if (Function *LF = F->Codegen()) {
                        /* code */
                }
        } else {
                next_token();
        }
}

/**************************************************************************
 * 解析驱动
 */
static void Driver() {
        while (1) {
                switch (Current_token) {
                case EOF_TOKEN:
                        return;
                case ';':
                        next_token();
                        break;
                case DEF_TOKEN:
                        HandleDefn();
                        break;
                default:
                        HandleTopExpression();
                        break;
                }
        }
}

int main(int argc, char const *argv[]) {
        LLVMContext &Context = getGlobalContext();
        init_precedence();
        file = fopen(argv[1], "r");
        if (file == 0) {
                printf("Could not open file\n");
        }
        next_token();
        Module_Ob = new Module("my compiler", Context);

        // 优化器
        FunctionPassManager My_FP(Module_Ob);
        My_FP.add(createBasicAliasAnalysisPass());
        My_FP.add(createInstructionCombiningPass());
        My_FP.add(createReassociatePass());
        My_FP.add(createGVNPass());
        My_FP.doInitialization();

        Global_FP = &My_FP;
        Driver();
        Module_Ob->dump();
        return 0;
}

/**********************************************************************
 * 为表达式生成IR
 * @return [description]
 */
Value *NumericAST::Codegen() {
        return ConstantInt::get(Type::getInt32Ty(getGlobalContext()), numeric_val);
}

Value *VariableAST::Codegen() {
        Value *V = Named_Values[Var_Name];
        return V ? V : 0;
}

Value *BinaryAST::Codegen() {
        Value *L = LHS->Codegen();
        Value *R = RHS->Codegen();
        if (L == 0 || R == 0)
                return 0;

        switch (atoi(Bin_Operator.c_str())) {
        case '+':
                return Builder.CreateAdd(L, R, "addtmp");
        case '-':
                return Builder.CreateSub(L, R, "subtmp");
        case '*':
                return Builder.CreateMul(L, R, "multmp");
        case '/':
                return Builder.CreateUDiv(L, R, "divtmp");
        default:
                return 0;
        }
}

/*****************************************************************8
 * 为函数生成IR
 */
Value *FunctionCallAST::Codegen() {
        Function *CalleeF = Module_Ob->getFunction(Function_Callee);
        std::vector<Value *> ArgsV;
        for (unsigned i = 0, e = Function_Arguments.size(); i != e; i++) {
                ArgsV.push_back(Function_Arguments[i]->Codegen());
                if (ArgsV.back() == 0)
                        return 0;
        }
        return Builder.CreateCall(CalleeF, ArgsV, "calltmp");
}

Function *FunctionDeclAST::Codegen() {
        std::vector<Type *> Integers(Arguments.size(),
                                     Type::getInt32Ty(getGlobalContext()));
        FunctionType *FT =
                FunctionType::get(Type::getInt32Ty(getGlobalContext()), Integers, false);
        Function *F =
                Function::Create(FT, Function::ExternalLinkage, Func_Name, Module_Ob);

        if (F->getName() != Func_Name) {
                F->eraseFromParent();

                if (!F->empty())
                        return 0;
                if (F->arg_size() != Arguments.size())
                        return 0;
        }

        unsigned Idx = 0;
        for (Function::arg_iterator Arg_It = F->arg_begin(); Idx != Arguments.size();
             ++Arg_It, ++Idx) {
                Arg_It->setName(Arguments[Idx]);
                Named_Values[Arguments[Idx]] = Arg_It;
        }

        return F;
}

Function *FunctionDefnAST::Codegen() {
        Named_Values.clear();

        Function *TheFunction = Func_Decl->Codegen();
        if (TheFunction == 0)
                return 0;

        BasicBlock *BB = BasicBlock::Create(getGlobalContext(), "entry", TheFunction);
        Builder.SetInsertPoint(BB);

        if (Value *RetVal = Body->Codegen()) {
                Builder.CreateRet(RetVal);
                verifyFunction(*TheFunction);
                Global_FP->run(*TheFunction); // 优化器
                return TheFunction;
        }
        TheFunction->eraseFromParent();
        return 0;
}
