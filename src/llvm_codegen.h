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


llvm::Type* llvm_achilles_to_llvm_type(const TSType &type, LLVMContext &ctx) {
	switch (type.variant) {
		case TSType::Variant::Float32:
			return Type::getDoubleTy(ctx);
			//return Type::getFloatTy(ctx);
			break;
	case TSType::Variant::Int32:
		return Type::getInt32Ty(ctx);
		break;
	case TSType::Variant::Void:
		return Type::getVoidTy(ctx);
		break;

	case TSType::Variant::Function: {
		std::vector<Type*> llvm_arg_types;
		for (auto arg : type.func_data->args) {
			llvm_arg_types.push_back(llvm_achilles_to_llvm_type(*arg, ctx));
		};

		Type* llvm_return_type = llvm_achilles_to_llvm_type(*type.func_data->return_type, ctx);
		const bool is_var_arg = false;
		FunctionType *func_type = FunctionType::get(llvm_return_type, llvm_arg_types, is_var_arg);

		return func_type;
	}
		
	default:
		assert("unable to convert achilles type to llvm type");
	}
	return nullptr;
}

llvm::Function* llvm_create_extern_linkage(ASTFunctionDefinition &fn_defn, llvm::LLVMContext &ctx, llvm::Module *module) {

	const std::string &name = *fn_defn.fn_name.value.ptr_s;
	llvm::FunctionType* type = reinterpret_cast<llvm::FunctionType*>(llvm_achilles_to_llvm_type(*fn_defn.ts_data->type, ctx));
    return Function::Create(type, Function::ExternalLinkage, name, module);

};

struct LLVMASTData {
	llvm::Value* val;
	llvm::Function *fn;

	LLVMASTData(llvm::Value &val) : val(&val), fn(nullptr) {};
};

struct LLVMCodeGenerator : public IASTVisitor {
	llvm::Module *module;
	llvm::LLVMContext &ctx;
	IRBuilder<>Builder;
    
    //map variables to values
	std::map<const TSVariable *, llvm::Value *> var_to_value_map;


public:

	LLVMCodeGenerator(LLVMContext& context, Module *module) : ctx(context), Builder(context),
		module(module) {}

	virtual void inspect_root(ASTRoot& root) {
		for (auto statement : root.children) {
			this->get_value_for_ast(*statement);
		}
	}

	llvm::Value* get_value_for_ast(IAST& ast) {
		switch (ast.type) {
		case ASTType::Literal:
			return get_value_for_literal(dynamic_cast<ASTLiteral&>(ast));

		case ASTType::InfixExpr:
			return this->get_value_for_infix_expr(dynamic_cast<ASTInfixExpr&>(ast));

		case ASTType::FunctionCall:
			return this->get_value_for_func_call(dynamic_cast<ASTFunctionCall&>(ast));

		case ASTType::Statement: {
			ASTStatement &stmt = dynamic_cast<ASTStatement&>(ast);
			return this->get_value_for_ast(*stmt.inner);
		}

		case ASTType::Block: {
			ASTBlock &block = dynamic_cast<ASTBlock&>(ast);
			for (auto stmt : block.statements) {
				//HACK: this will only work for the _fist_ call.
				return this->get_value_for_ast(*stmt);
			}
			//return this->get_value_for_ast(*stmt.inner);
			break;
		}
		case ASTType::FunctionDefinition: {
			return this->get_value_for_function_defn(dynamic_cast<ASTFunctionDefinition&>(ast));
		}
		case ASTType::PrefixExpr: {
			return this->get_value_for_prefix_expr(dynamic_cast<ASTPrefixExpr&>(ast));
		}

		default:
			std::stringstream error;
			error << "unknown ast type to codegen:\n";
			pretty_print_to_stream(ast, error);
			throw std::runtime_error(error.str());
		}

		assert(false && "should not have gotten here");
	}

	llvm::Function *get_value_for_function_defn(ASTFunctionDefinition &fn_defn){
		if (fn_defn.body) {
			Function *f = llvm_create_extern_linkage(fn_defn, this->ctx, this->module);
            BasicBlock *BB = BasicBlock::Create(getGlobalContext(), "entry", f);
            Builder.SetInsertPoint(BB);
            
			unsigned index = 0;
			for (Function::arg_iterator arg_val_iter = f->arg_begin(); index != fn_defn.args.size();
				 ++arg_val_iter, ++index) {

				ASTLiteral &arg_ast = dynamic_cast<ASTLiteral&>(*fn_defn.args[index].first);
				std::string arg_name = *arg_ast.token.value.ptr_s;
				arg_val_iter->setName(arg_name);

                //this typecast is mysterious
				Value *arg_val = arg_val_iter;

				const TSVariable *indexing_var = arg_ast.ts_data->scope->get_variable(arg_name);
				assert(this->var_to_value_map.find(indexing_var) == this->var_to_value_map.end());
				this->var_to_value_map[indexing_var] = arg_val;

			}

			Value *return_value = this->get_value_for_ast(*fn_defn.body);
            Builder.CreateRet(return_value);
            
			verifyFunction(*f);
			return f;

		}
        //this is a forward decl.
        //HACK: for now, assume *all* forward decls to be externs -_-
		else {
			Function *f = llvm_create_extern_linkage(fn_defn, this->ctx, this->module);
			return f;
		};
	
	}

	Value* get_value_for_infix_expr(ASTInfixExpr& expr) {
		Value *left = get_value_for_ast(*expr.left);
		Value *right = get_value_for_ast(*expr.right);

		switch (expr.op.type) {
		case TokenType::Plus:
			return Builder.CreateFAdd(left, right, "addtmp");

		case TokenType::Minus:
			return Builder.CreateFSub(left, right, "subtmp");

		case TokenType::Multiply:
			return Builder.CreateFMul(left, right, "multmp");

		case TokenType::Divide:
			return Builder.CreateFDiv(left, right, "divtmp");

		default:
			assert(false && "unknown infix expression");
		}
		return nullptr;
	}


	Value *get_value_for_prefix_expr(ASTPrefixExpr& prefix_expr) {
		switch (prefix_expr.op.type) {
		case TokenType::Minus:
			return nullptr;
		default:
			assert(false && "unknown prefx expr");
			return nullptr;
		}
	}


	Value* get_value_for_func_call(ASTFunctionCall& func_call) {
		if (func_call.name->type != ASTType::Literal) {
			std::stringstream error;
			error << "function call name is not a string";
			error << pretty_print(func_call);
			throw std::runtime_error(error.str());
		}

		std::string fn_name = *dynamic_cast<ASTLiteral&>(*func_call.name).token.value.ptr_s;

		Function *called_fn = module->getFunction(fn_name);

		if (!called_fn) {
			std::stringstream error;
			error << "undefined function: ";
			error << fn_name;
			error << func_call.position;
			throw std::runtime_error(error.str());
		}

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
			const TSVariable *id_var = literal.ts_data->scope->get_variable(name);
			auto it = this->var_to_value_map.find(id_var);

			assert(it != this->var_to_value_map.end());
            
			Value *val = it->second;
			return val;
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
	Module *module = new Module("marg_val_itern_module", ctx);
	LLVMCodeGenerator code_genner(ctx, module);

	code_genner.inspect_root(dynamic_cast<ASTRoot&>(root));

	std::cout << "\n-------\n\nmodule dump: \n";
	module->dump();
	return module;
}
