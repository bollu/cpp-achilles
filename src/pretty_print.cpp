#pragma once
#include "pretty_print.h"
#include <sstream>
#include <iostream>
#include <ostream>

void pretty_print_to_stream(IAST& ast, std::ostream& out) {
    ASTPrettyPrinter printer(out);

    ast.dispatch(printer);
}

std::string pretty_print(IAST& ast) {
    std::stringstream sstream;

    pretty_print_to_stream(ast, sstream);
    return sstream.str();
}

ASTPrettyPrinter::ASTPrettyPrinter(std::ostream& out) : out(out), depth(0) {}

void ASTPrettyPrinter::print_indent() {
    for (int i = 0; i < this->depth; ++i) {
        out << "|";
        out << "  ";
    }
}


void ASTPrettyPrinter::inspect_literal(ASTLiteral& literal) {
    this->out << literal.token;

    if (literal.ts_data
        && literal.ts_data->type) {
        this->out << "@" << *literal.ts_data->type;
    }
}

void ASTPrettyPrinter::inspect_block(ASTBlock& block) {
    out << "\n";
    this->print_indent();
    out << "{\n";
    this->depth++;
    this->print_indent();

    for (auto statement: block.statements) {
        statement->dispatch(*this);
    }

    this->depth--;
    out << "\n";
    this->print_indent();
    out << "}";
}

void ASTPrettyPrinter::inspect_infix_expr(ASTInfixExpr& infix) {
    out << "(";
    infix.left->dispatch(*this);
    out << " " << infix.op << " ";
    infix.right->dispatch(*this);
    out << ")";
}

void ASTPrettyPrinter::inspect_prefix_expr(ASTPrefixExpr& prefix) {
    out << prefix.op;
    prefix.expr->dispatch(*this);
}

void ASTPrettyPrinter::inspect_statement(ASTStatement& statement) {
    out << "\n";
    this->print_indent();
	out << "<";
    statement.inner->dispatch(*this);
    out << ">;";
}

void ASTPrettyPrinter::inspect_fn_definition(ASTFunctionDefinition& fn_defn) {
    out << "fn ";
    out << fn_defn.fn_name;
    out << "(";

    int commas_count = fn_defn.args.size() - 1;

    for (auto arg: fn_defn.args) {
        arg.first->dispatch(*this);
        out << " ";
        arg.second->dispatch(*this);

        if (commas_count > 0) {
            out << ", ";
            commas_count--;
        }
    }


    out << ")" << " -> ";
    fn_defn.return_type->dispatch(*this);
	if (fn_defn.body) {
		fn_defn.body->dispatch(*this);
	};
}

void ASTPrettyPrinter::inspect_variable_definition(ASTVariableDefinition& defn)
{
    out << "let ";
    defn.name->dispatch(*this);

    if (defn.type) {
        out << " : ";
        defn.type->dispatch(*this);
    }
}

void ASTPrettyPrinter::inspect_fn_call(ASTFunctionCall& fn_call) {
	fn_call.name->dispatch(*this);
	out << "(";
	int num_commas = fn_call.params.size() - 1;

	for (auto param : fn_call.params) {
		param->dispatch(*this);

		if (num_commas > 0) {
			out << ", ";
			num_commas--;
		}
	}
	out << ")";
	if (fn_call.ts_data) {
		out << "@" << *fn_call.ts_data->type << "";
	}
}
