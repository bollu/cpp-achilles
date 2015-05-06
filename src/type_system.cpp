#include "type_system.h"
#include "ast.h"
#include <sstream>
#include <iostream>
#include "pretty_print.h"
/*
   TSType *int_type = new TSType(TSType::Variant::Int, nullptr);
   TSType *float_type = new TSType(TSType::Variant::Float, nullptr);
   TSType *string_type = new TSType(TSType::Variant::String, nullptr);
   */

TSType::TSType(const TSFunctionTypeData func_data,
			   PositionRange            *decl_pos) : decl_pos(decl_pos) {
	this->variant = Variant::Function;
	this->func_data = std::make_shared<TSFunctionTypeData>(func_data);
}

std::ostream& operator<<(std::ostream& out, const TSType& type) {
	out << "t-";

	switch (type.variant) {
	case TSType::Variant::Void:
		out << "void";
		break;

	case TSType::Variant::Int32:
		out << "i32";
		break;

	case TSType::Variant::Float32:
		out << "f32";
		break;

	case TSType::Variant::Int:
		out << "i_";
		break;

	case TSType::Variant::Float:
		out << "f_";
		break;

	case TSType::Variant::String:
		out << "string";
		break;

	case TSType::Variant::Function:
		assert(type.func_data);

		out << "fn (";
		int num_commas = type.func_data->args.size() - 1;

		for (auto arg : type.func_data->args) {
			out << *arg;

			if (num_commas > 0) {
				out << ", ";
			}
		}
		out << ")";
		out << " -> " << *type.func_data->return_type;

		break;
	}
	return out;
}

struct TSDataCreator : public IASTVisitor {
	TSContext &ctx;
	TSScope  *scope;
	TSDataCreator(TSContext &ctx, TSScope *scope) : ctx(ctx), scope(scope) {};

	static void setup_variable_use(ASTLiteral &literal, TSScope *scope) {
		assert(literal.token.type == TokenType::Identifier);
		const std::string &name = *literal.token.value.ptr_s;

		if (!scope->has_variable(name)) {
			std::stringstream error;
			error << "undefined variable: " << name;
			error << literal.position;

			throw std::runtime_error(error.str());
		}

		const TSVariable *variable = scope->get_variable(name);
		literal.ts_data = std::make_shared<TSASTData>(scope, variable->type);
	};

	static void setup_variable_definition(ASTLiteral &literal, const TSType *type, TSScope *scope) {
		assert(literal.token.type == TokenType::Identifier);
		const std::string &name = *literal.token.value.ptr_s;

		if (scope->has_variable(name)) {
			const TSVariable *variable_definition = scope->get_variable(name);
			std::stringstream error;
			error << "multiple variable definition: " << name;
			error << *variable_definition->decl_pos << " originial definition";
			error << literal.position << " multiple definition";

			throw std::runtime_error(error.str());
		}

		TSVariable *new_var = new TSVariable(name, type, &literal.position);
		scope->add_variable(name, new_var);
		literal.ts_data = std::make_shared<TSASTData>(scope, type);
	}

	static const TSType* get_type_for_literal(ASTLiteral &literal, TSScope *scope) {
		assert(literal.token.type == TokenType::Identifier);
		const std::string &name = *literal.token.value.ptr_s;

		if (!scope->has_type(name)) {
			std::stringstream error;
			error << "unable to find type: " << name;
			error << literal.position;

			throw std::runtime_error(error.str());
		};

		return scope->get_type(name);
	};

	virtual void inspect_literal(ASTLiteral& literal) {

		switch (literal.token.type) {
		case TokenType::LiteralInt:
			literal.ts_data = std::make_shared<TSASTData>(scope, i32_type);
			break;

		case TokenType::LiteralFloat:
			literal.ts_data = std::make_shared<TSASTData>(scope, f32_type);
			break;

		case TokenType::LiteralString:
			literal.ts_data = std::make_shared<TSASTData>(scope, string_type);
			break;

		case TokenType::Identifier:
			TSDataCreator::setup_variable_use(literal, this->scope);
			break;

		default:
			assert(false && "unknown literal type");
		}
	};

	static void setup_block(ASTBlock &block, TSDataCreator &inner_creator, TSDataCreator &outer_creator) {
		//run statements against the new block
		for (auto stmt : block.statements) {
			stmt->dispatch(inner_creator);
		}

		if (block.return_expr) {
			//generate a return type using the *inner* block
			block.return_expr->dispatch(inner_creator);
			//copy the type
			//the *entire* block belongs to the outer scope
			const TSType *return_type = block.return_expr->ts_data->type;
			block.ts_data = std::make_shared<TSASTData>(outer_creator.scope, return_type);
		}
		else {
			//the *entire* block belongs to the outer scope
			block.ts_data = std::make_shared<TSASTData>(outer_creator.scope, void_type);
		}
	};

	virtual void inspect_block(ASTBlock& block) {
		TSScope *block_scope = ctx.create_child_scope(this->scope);
		TSDataCreator block_tsdata_creator(ctx, block_scope);

		TSDataCreator::setup_block(block, block_tsdata_creator, *this);
	};

	virtual void inspect_statement(ASTStatement& statement) {
		statement.ts_data = std::make_shared<TSASTData>(this->scope, void_type);
		statement.inner->dispatch(*this);
	};

	//we don't need to do anything for these. just let the
	//default instance of ASTVisitor which "does the right thing"
	virtual void inspect_infix_expr(ASTInfixExpr& infix){
		infix.left->dispatch(*this);
		infix.right->dispatch(*this);
        
        //arith
		if (infix.op.type == TokenType::Plus ||
			infix.op.type == TokenType::Minus ||
			infix.op.type == TokenType::Multiply ||
			infix.op.type == TokenType::Divide) {

			//HACK!
			const TSType *unified_type = f32_type;
			infix.ts_data = std::make_shared<TSASTData>(scope, unified_type);
		};
	};

	//virtual void inspect_prefix_expr(ASTPrefixExpr& prefix){};

	virtual void inspect_fn_definition(ASTFunctionDefinition& fn_defn) {
		fn_defn.ts_data = std::make_shared<TSASTData>(scope, void_type);

		TSScope *fn_scope = ctx.create_child_scope(this->scope);
		TSDataCreator fn_defn_data_creator(this->ctx, fn_scope);


		std::vector<const TSType*> arg_types;
		//fill in the args
		for (auto arg : fn_defn.args) {
			ASTLiteral &arg_name = *reinterpret_cast<ASTLiteral*>(arg.first.get());
			ASTLiteral &type_name = *reinterpret_cast<ASTLiteral*>(arg.second.get());

			const TSType *arg_type = TSDataCreator::get_type_for_literal(type_name, fn_scope);
			arg_types.push_back(arg_type);
			TSDataCreator::setup_variable_definition(arg_name, arg_type, fn_scope);
		}

		//it need not have a body, could be a prototype
		if (fn_defn.body) {
			//now type the block
			ASTBlock &fn_body = *reinterpret_cast<ASTBlock*>(fn_defn.body.get());
			TSDataCreator::setup_block(fn_body, fn_defn_data_creator, *this);
		}
		//type the return type
		ASTLiteral &return_name = *reinterpret_cast<ASTLiteral*>(fn_defn.return_type.get());
		const TSType *return_type = TSDataCreator::get_type_for_literal(return_name, this->scope);
		fn_defn.return_type->ts_data = std::make_shared<TSASTData>(this->scope, return_type);

		//construct fn type
		const TSType *fn_type = new TSType(TSFunctionTypeData(arg_types, return_type), &fn_defn.position);
		fn_defn.ts_data = std::make_shared<TSASTData>(this->scope, fn_type);
	};

	virtual void inspect_fn_call(ASTFunctionCall& fn_call) {
		std::string fn_name = *reinterpret_cast<ASTLiteral*>(fn_call.name.get())->token.value.ptr_s;

		if (!this->scope->has_variable(fn_name)) {
			std::stringstream error;
			error << fn_call.position;
			error << "unknown function: " << fn_name;

			throw std::runtime_error(error.str());
		}

		const TSVariable *fn = this->scope->get_variable(fn_name);
		fn_call.ts_data = std::make_shared<TSASTData>(this->scope, fn->type);


		for (auto param : fn_call.params) {
			param->dispatch(*this);
		}
	};

	virtual void inspect_variable_definition(ASTVariableDefinition& variable_defn) {

		//find the type of the "type" part of type definition. and give it over to the AST.
		ASTLiteral &type_name = *reinterpret_cast<ASTLiteral*>(variable_defn.type.get());
		const TSType *type = TSDataCreator::get_type_for_literal(type_name, this->scope);

		variable_defn.type->ts_data = std::make_shared<TSASTData>(this->scope, type);
		variable_defn.name->ts_data = std::make_shared<TSASTData>(this->scope, type);

		//create a new variable, bring it into scope <3
		ASTLiteral &variable_name = *reinterpret_cast<ASTLiteral*>(variable_defn.name.get());
		TSDataCreator::setup_variable_definition(variable_name, type, this->scope);

		variable_defn.ts_data = std::make_shared<TSASTData>(this->scope, type);
	};

};

struct TSArithTypeChecker : public IASTVisitor {
	static bool is_number(const TSType *type) {
		return type == i32_type ||
			type == f32_type;
	}

	static TSType* coerce_safely_to(const TSType *to, const TSType *from) {
		assert(is_number(to) && is_number(from));

		return nullptr;
	};

	virtual void inspect_infix_expr(ASTInfixExpr& infix) {
		infix.left->dispatch(*this);
		infix.right->dispatch(*this);

		if (infix.op.type == TokenType::Plus ||
			infix.op.type == TokenType::Minus ||
			infix.op.type == TokenType::Multiply ||
			infix.op.type == TokenType::Divide) {

			if (!infix.left->ts_data) {
				std::cout << "\n" << infix.position << "\nleft: " << infix.left->position << "\nright: " << infix.right->position;
				std::cout << "\n" <<pretty_print(*infix.left);
				infix.left->dispatch(*this);
			}
			assert(infix.left->ts_data);

			if (!is_number(infix.left->ts_data->type)) {
				std::stringstream error;
				error << "expected a number as a left operand to " << infix.op.type;
				error << infix.left->position << "expected number";
				error << "\nreceived: " << *infix.left->ts_data->type;

				throw std::runtime_error(error.str());
			}

			if (!is_number(infix.right->ts_data->type)) {
				std::stringstream error;
				error << "expected a number as a right operand to " << infix.op.type;
				error << infix.right->position << "expected number";
				error << "\nreceived: " << *infix.right->ts_data->type;

				throw std::runtime_error(error.str());
			}


		};
	};

	//TO IMPLEMENT
	virtual void inspect_prefix_expr(ASTPrefixExpr& prefix) {
		if (prefix.op.type == TokenType::Minus) {
			if (!is_number(prefix.expr->ts_data->type)) {
				std::stringstream error;
				error << "expected number for unary -";
				error << prefix.position;

				throw std::runtime_error(error.str());
			}
		}
	};
};

struct TSEqualityTypeChecker : public IASTVisitor {
	virtual void inspect_infix_expr(ASTInfixExpr& infix){
		if (infix.op.type == TokenType::Equals) {

			if (infix.left->ts_data->type != infix.right->ts_data->type) {
				std::stringstream error;
				error << "types do not match on \"=\" |";
				error << "left: " << *infix.left->ts_data->type;
				error << " | right: " << *infix.right->ts_data->type;

				error << infix.left->position << " type: " << *infix.left->ts_data->type;
				error << infix.right->position << " type: " << *infix.right->ts_data->type;

				throw std::runtime_error(error.str());
			}

		}
	};
};

TSContext type_system_type_check(std::shared_ptr<IAST>root) {
	TSContext ctx;
	TSDataCreator ts_data_creator(ctx, ctx.get_root_scope());
	root->dispatch(ts_data_creator);

	TSArithTypeChecker ts_arith_checker;
	root->dispatch(ts_arith_checker);

	TSEqualityTypeChecker equality_checker;
	root->dispatch(equality_checker);

	return ctx;
}
