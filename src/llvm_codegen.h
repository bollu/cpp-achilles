#pragma once
#define __STDC_LIMIT_MACROS
#define __STDC_CONSTANT_MACROS

#include "llvm/IR/Verifier.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"

#include <sstream>

#include "ast.h"
#include "type_system.h"
#include "tokenizer.h"
#include "pretty_print.h"


using namespace llvm;


struct LLVMCodeGenerator : public IASTVisitor {
    llvm::Module *module;
    IRBuilder<>Builder;

public:

    LLVMCodeGenerator(LLVMContext& context, Module *module) : Builder(context),
        module(module) {}

    virtual void inspect_root(ASTRoot& root) {
        for (auto statement: root.children) {
            this->get_value_for_ast(*statement);
        }
    }

    Value* get_value_for_ast(IAST& ast) {
        switch (ast.type) {
        case ASTType::Literal:
            return get_value_for_literal(dynamic_cast<ASTLiteral&>(ast));

        case ASTType::InfixExpr:
            return this->get_value_for_infix_expr(dynamic_cast<ASTInfixExpr&>(
                                                      ast));

        case ASTType::FunctionCall:
            return this->get_value_for_func_call(dynamic_cast<ASTFunctionCall&>(
                                                     ast));

        default:

            /*assert(false
               && "unknown AST type");*/
            std::stringstream error;
            error << "unknown ast type to codegen:\n";
            pretty_print_to_stream(ast, error);
            throw std::runtime_error(error.str());
        }
    }

    Value* get_value_for_infix_expr(ASTInfixExpr& expr) {
        Value *left = get_value_for_ast(*expr.left);
        Value *right = get_value_for_ast(*expr.right);

        switch (expr.op.type) {
        case TokenType::Plus:
            return Builder.CreateFAdd(left,  right, "addtmp");

        case TokenType::Minus:
            return Builder.CreateFSub(left,  right, "subtmp");

        case TokenType::Multiply:
            return Builder.CreateFMul(left,  right, "multmp");

        case TokenType::Divide:
            return Builder.CreateFDiv(left,  right, "divtmp");

        default:
            assert(false
                   && "unknown infix expression");
        }
    }

    Value* get_value_for_func_call(ASTFunctionCall& func_call) {
        if (func_call.name->type != ASTType::Literal) {
            std::stringstream error;
            error << "function call name is not a string";
            error << pretty_print(func_call);
            throw std::runtime_error(error.str());
        }

        std::string fn_name =
            *dynamic_cast<ASTLiteral&>(*func_call.name).token.value.ptr_s;


        Function *called_fn = module->getFunction(fn_name);

        if (!called_fn) {
            std::stringstream error;
            error << "undefined function: ";
            error << fn_name;
            error << func_call.position;
            throw std::runtime_error(error.str());


            if (func_call.params.size() != called_fn->arg_size()) {
                std::stringstream error;
                error << "different argument list sizes:\n";
                error << pretty_print(func_call);
                throw std::runtime_error(error.str());
            }
            std::vector<Value *>args;

            for (unsigned i = 0; i < func_call.params.size(); ++i) {
                args.push_back(this->get_value_for_ast(*func_call.params[i]));
            }

            return Builder.CreateCall(called_fn, args, "calltmp");
        }
        return nullptr;
    }

    Value* get_value_for_literal(ASTLiteral& literal) {
        switch (literal.token.type) {
        case TokenType::LiteralInt:
        {
            float val = *literal.token.value.ptr_i;
            return ConstantFP::get(getGlobalContext(), APFloat(val));
        }

        case TokenType::LiteralFloat:
        {
            float val = *literal.token.value.ptr_f;
            return ConstantFP::get(getGlobalContext(), APFloat(val));
        }

        case TokenType::Identifier:
        {
            std::string& name = *literal.token.value.ptr_s;
            assert(false
                   && "locals are unhandled for now");

            // literal.ts_data->scope->get_variable()
        }

        default:
            assert(false
                   && "cannot return llvm value");
            return nullptr;
        }
    }
};

llvm::Module* generate_llvm_code(IAST& root, TSContext& context) {
    LLVMContext& ctx = getGlobalContext();
    Module *module = new Module("main_module", ctx);
    LLVMCodeGenerator code_genner(ctx, module);

    code_genner.inspect_root(dynamic_cast<ASTRoot&>(root));

    std::cout << "\n-------\n\nmodule dump: \n";
    module->dump();
    return module;
}
