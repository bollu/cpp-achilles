#pragma once
#include "ast.h"
#include <sstream>
#include <iostream>
#include <ostream>

class ASTPrettyPrinter : public IASTVisitor {
private:

    int depth;
    std::ostream& out;

    void print_indent();
	void print_type(IAST &ast);

public:

    ASTPrettyPrinter(std::ostream& out);
    void inspect_literal(ASTLiteral& literal);
    void inspect_block(ASTBlock& block);
    void inspect_infix_expr(ASTInfixExpr& infix);
    void inspect_prefix_expr(ASTPrefixExpr& prefix);
    void inspect_statement(ASTStatement& statement);
    void inspect_fn_definition(ASTFunctionDefinition& fn_defn);
    void inspect_fn_call(ASTFunctionCall& fn_call);
    void inspect_variable_definition(ASTVariableDefinition& variable_defn);
};

void pretty_print_to_stream(IAST& ast, std::ostream& out);
std::string pretty_print(IAST& ast);
