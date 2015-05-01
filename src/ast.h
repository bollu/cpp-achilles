#pragma once
#include "tokenizer.h"
#include "file_handling.h"
#include "type_system.h"
#include "assert.h"

class IAST;
class IASTVisitor;
class ASTRoot;
class ASTLiteral;
class ASTBlock;
class ASTInfixExpr;
class ASTPrefixExpr;
class ASTStatement;
class ASTFunctionDefinition;
class ASTFunctionCall;
class ASTVariableDefinition;

enum class ASTType {
	PrefixExpr,
	InfixExpr,
	Literal,
	Statement,
	Block,
	FunctionDefinition,
	ExternDeclaration,
	VariableDefinition,
	FunctionCall,
	Attribute,
	Root,
};

class IAST {
protected:

	IAST(ASTType ast_type, PositionRange position) : type(ast_type), position(
		position),
		ts_data(nullptr) {}

public:

	PositionRange position;
	std::shared_ptr<TSASTData>ts_data;
	ASTType type;

	// calls the correct version of the visit on the visitor
	virtual void dispatch(IASTVisitor& visitor) = 0;

	// recurses the traversal on sub elements
	virtual void traverse_inner(IASTVisitor& visitor) = 0;
};

// visitor
class IASTVisitor {
public:

	virtual void inspect_root(ASTRoot& root);
	virtual void inspect_literal(ASTLiteral& literal);
	virtual void inspect_block(ASTBlock& block);
	virtual void inspect_infix_expr(ASTInfixExpr& infix);
	virtual void inspect_prefix_expr(ASTPrefixExpr& prefix);
	virtual void inspect_statement(ASTStatement& statement);
	virtual void inspect_fn_definition(ASTFunctionDefinition& fn_defn);
	virtual void inspect_fn_call(ASTFunctionCall& fn_call);
	virtual void inspect_variable_definition(
		ASTVariableDefinition& variable_defn);
};

// provides a inspect_ast that lets you map over any generic ast type;
////NOTE - all of the inspect_* internally call map_ast for this effect
class IASTGenericVisitor : public IASTVisitor {
public:

	virtual void inspect_ast(IAST& ast) = 0;

	virtual void inspect_root(ASTRoot& root);
	virtual void inspect_literal(ASTLiteral& literal);
	virtual void inspect_block(ASTBlock& block);
	virtual void inspect_infix_expr(ASTInfixExpr& infix);
	virtual void inspect_prefix_expr(ASTPrefixExpr& prefix);
	virtual void inspect_statement(ASTStatement& statement);
	virtual void inspect_fn_definition(ASTFunctionDefinition& fn_defn);
	virtual void inspect_fn_call(ASTFunctionCall& fn_call);
	virtual void inspect_variable_definition(
		ASTVariableDefinition& variable_defn);
};

class ASTPrefixExpr : public IAST {
public:

	const Token& op;
	std::shared_ptr<IAST>expr;

	ASTPrefixExpr(const Token & op,
				  std::shared_ptr<IAST>expr,
				  PositionRange position) : op(op), expr(expr), IAST(ASTType::PrefixExpr, position) {}

	virtual void dispatch(IASTVisitor& visitor) {
		visitor.inspect_prefix_expr(*this);
	}

	virtual void traverse_inner(IASTVisitor& visitor) {
		expr->dispatch(visitor);
	}
};

class ASTInfixExpr : public IAST {
public:

	const Token&          op;
	std::shared_ptr<IAST>left;
	std::shared_ptr<IAST>right;

	ASTInfixExpr(std::shared_ptr<IAST>left,
				 const Token          & op,
				 std::shared_ptr<IAST>right,
				 PositionRange        position) :
				 left(left), op(op), right(right), IAST(ASTType::InfixExpr, position) {}

	virtual void dispatch(IASTVisitor& visitor) {
		visitor.inspect_infix_expr(*this);
	}

	virtual void traverse_inner(IASTVisitor& visitor) {
		left->dispatch(visitor);
		right->dispatch(visitor);
	}
};

class ASTStatement : public IAST {
public:

	std::shared_ptr<IAST>inner;

	ASTStatement(std::shared_ptr<IAST>inner, PositionRange position) :
		inner(inner), IAST(ASTType::Statement, position) {}

	virtual void dispatch(IASTVisitor& visitor) {
		visitor.inspect_statement(*this);
	}

	virtual void traverse_inner(IASTVisitor& visitor) {
		inner->dispatch(visitor);
	}
};

class ASTBlock : public IAST {
public:

	std::vector<std::shared_ptr<IAST> >statements;
	std::shared_ptr<IAST>return_expr;

	ASTBlock(std::vector<std::shared_ptr<IAST> >statements,
			 std::shared_ptr<IAST>              return_expr,
			 PositionRange                      position) :
			 statements(statements), return_expr(return_expr), IAST(ASTType::Block,
			 position) {}

	virtual void dispatch(IASTVisitor& visitor) {
		visitor.inspect_block(*this);
	}

	virtual void traverse_inner(IASTVisitor& visitor) {
		for (auto statement : this->statements) {
			statement->dispatch(visitor);
		}
	}
};

class ASTLiteral : public IAST {
public:

	const Token& token;

	ASTLiteral(const Token& token, PositionRange position) :
		token(token), IAST(ASTType::Literal, position) {}

	virtual void dispatch(IASTVisitor& visitor) {
		visitor.inspect_literal(*this);
	}

	virtual void traverse_inner(IASTVisitor& visitor) {}
};

class ASTFunctionDefinition : public IAST {
public:

	typedef std::pair<std::shared_ptr<IAST>, std::shared_ptr<IAST> >Argument;
	const Token&          fn_name;
	std::vector<Argument>args;

	std::shared_ptr<IAST>body;
	std::shared_ptr<IAST>return_type;

	ASTFunctionDefinition(const Token &fn_name,
						  std::vector<Argument> args,
						  std::shared_ptr<IAST> return_type,
						  std::shared_ptr<IAST> body,
						  PositionRange position) :
						  fn_name(fn_name),
						  args(args),
						  return_type(return_type),
						  body(body),
						  IAST(ASTType::FunctionDefinition, position) {}

	virtual void dispatch(IASTVisitor& visitor) {
		visitor.inspect_fn_definition(*this);
	}

	virtual void traverse_inner(IASTVisitor& visitor) {
		for (auto arg : args) {
			arg.first->dispatch(visitor);
			arg.second->dispatch(visitor);
		}
		if (this->body) {
			this->body->dispatch(visitor);
		}

		this->return_type->dispatch(visitor);
	}
};


class ASTFunctionCall : public IAST {
public:

	std::shared_ptr<IAST>name;
	std::vector<std::shared_ptr<IAST> >params;


	ASTFunctionCall(std::shared_ptr<IAST> name,
					std::vector<std::shared_ptr<IAST>>params,
					PositionRange position) :
					name(name), params(params), IAST(ASTType::FunctionCall, position) {}

	virtual void dispatch(IASTVisitor& visitor) {
		visitor.inspect_fn_call(*this);
	}

	virtual void traverse_inner(IASTVisitor& visitor) {
		this->name->dispatch(visitor);

		for (auto param : params) {
			param->dispatch(visitor);
		}
	}
};

class ASTVariableDefinition : public IAST {
public:

	std::shared_ptr<IAST>name;
	std::shared_ptr<IAST>type;

	ASTVariableDefinition(std::shared_ptr<IAST>name,
						  std::shared_ptr<IAST>type,
						  PositionRange position) :
						  name(name), type(type), IAST(
						  ASTType::VariableDefinition,
						  position) {}

	virtual void dispatch(IASTVisitor& visitor) {
		visitor.inspect_variable_definition(*this);
	}

	virtual void traverse_inner(IASTVisitor& visitor) {
		assert(this->name);
		this->name->dispatch(visitor);

		if (this->type) {
			this->type->dispatch(visitor);
		}
	}
};

class ASTRoot : public IAST {
public:

	std::vector<std::shared_ptr<IAST> >children;

	ASTRoot(std::vector<std::shared_ptr<IAST> >children,
			PositionRange position) :
			IAST(ASTType::Root, position), children(children) {}

	virtual void dispatch(IASTVisitor& visitor) {
		visitor.inspect_root(*this);
	}

	virtual void traverse_inner(IASTVisitor& visitor) {
		for (auto child : this->children) {
			child->dispatch(visitor);
		}
	}
};

std::shared_ptr<IAST>parse(std::vector<Token> &tokens,
						   std::string &file_data);
