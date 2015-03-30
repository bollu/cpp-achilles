#pragma once
#include "ast.h"
#include <sstream>
#include <iostream>
#include <ostream>

class ASTPrettyPrinter : public IASTVisitor {
    private:
        int depth;
        std::ostream &out;

        void print_indent();
    public:
        ASTPrettyPrinter(std::ostream &out);
        void map_literal(ASTLiteral &literal);
        void map_block(ASTBlock &block);
        void map_infix_expr(ASTInfixExpr &infix);
        void map_prefix_expr(ASTPrefixExpr &prefix);
        void map_statement(ASTStatement &statement);
        void map_fn_definition(ASTFunctionDefinition &fn_defn);
        void map_fn_call(ASTFunctionCall &fn_call);
        void map_variable_definition(ASTVariableDefinition &variable_defn);
};

void pretty_print_to_stream(IAST &ast, std::ostream &out) {
    ASTPrettyPrinter printer(out);
    ast.map(printer);
}

std::string pretty_print(IAST &ast) {
    std::stringstream sstream;
    pretty_print_to_stream(ast, sstream);
    return sstream.str();
}

ASTPrettyPrinter::ASTPrettyPrinter(std::ostream &out) : out(out), depth(0) {};

void ASTPrettyPrinter::print_indent() {
    for(int i = 0; i < this->depth; ++i) {
        out<<"|";
        out<<"  ";
    }
};

void ASTPrettyPrinter::map_literal(ASTLiteral &literal) {
    this->out<<literal.token;
    //this->out<<"<-";
    this->out<<":"; 
    if(literal.ts_typevar_uuid >= 0) {
        this->out<<literal.ts_scope->get_symbol_type(literal.ts_typevar_uuid);
    }

};

void ASTPrettyPrinter::map_block(ASTBlock &block){
    out<<"\n";
    this->print_indent();
    out<<"{\n";
    this->depth++;
    this->print_indent();

    for(auto statement: block.statements) {
        statement->map(*this);
    }

    this->depth--;
    out<<"\n";
    this->print_indent();
    out<<"}";
};

void ASTPrettyPrinter::map_infix_expr(ASTInfixExpr &infix){
    out<<"(";
    infix.left->map(*this);
    out<<" "<<infix.op<<" ";
    infix.right->map(*this);
    out<<")";
};

void ASTPrettyPrinter::map_prefix_expr(ASTPrefixExpr &prefix){
    out<<prefix.op;
    prefix.expr->map(*this);
};

void ASTPrettyPrinter::map_statement(ASTStatement &statement){
    out<<"\n";
    this->print_indent();
    statement.inner->map(*this);
    out<<";";
};

void ASTPrettyPrinter::map_fn_definition(ASTFunctionDefinition &fn_defn){
    out<<"\n";
    this->print_indent();
    out<<"fn ";
    out<<fn_defn.fn_name;
    out<<"(";

    int commas_count = fn_defn.args.size() - 1;

    for (auto arg: fn_defn.args) {
        out<<arg.first;
        out<<" ";
        arg.second->map(*this);

        if(commas_count > 0) {
            out<<", ";
            commas_count--;
        }
    }


    out<<")"<<" -> ";
    fn_defn.return_type->map(*this);
    fn_defn.body->map(*this);

};

void ASTPrettyPrinter::map_variable_definition(ASTVariableDefinition &defn) {
    out<<"let ";
    out<<defn.name;

    if(defn.type) {
        out<<" : ";
        defn.type->map(*this);
    };

    if(defn.rhs_value) {
        out<<" = ";
        defn.rhs_value->map(*this);
    }
}

void ASTPrettyPrinter::map_fn_call(ASTFunctionCall &fn_call) {
    fn_call.name->map(*this);
    out<<"(";
    int num_commas = fn_call.params.size() - 1;

    for(auto param : fn_call.params) {
        param->map(*this);

        if(num_commas > 0) {
            out<<", ";
            num_commas--;
        }
    }
    out<<")";
};
