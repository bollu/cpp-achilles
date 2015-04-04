#include "type_system.h"
#include "ast.h"
#include <sstream>
#include <iostream>

std::ostream &operator <<(std::ostream &out, const TSType &type) {
    out<<"t-";
    switch (type.variant) {
        case TSType::Variant::Void:
        out<<"void";
        break;
        case TSType::Variant::Int32:
        out<<"i32";
        break;
        case TSType::Variant::Int:
        out<<"i_";
        break;
        case TSType::Variant::Float:
        out<<"f_";
        break;
        case TSType::Variant::String:
        out<<"string";
        break;
    };
    return out;
}

class ScopeGenerator : public IASTGenericVisitor {
private:
    TSScope *scope;
    TSContext &context;
public:
    ScopeGenerator(TSContext &context, TSScope *scope) : context(context), scope(scope) {};

    void map_ast(IAST &ast) {
        ast.ts_data = std::make_shared<TSASTData>(scope);
        ast.traverse_inner(*this);
    };


    void map_block(ASTBlock &block) {
        TSScope *inner_scope = context.create_child_scope(this->scope);
        ScopeGenerator inner_scope_generator = ScopeGenerator(context, inner_scope);

            //make the inner scope setup the block
        inner_scope_generator.map_ast(block);
    };
};

class Typer : public IASTVisitor {

public:
    void map_variable_definition(ASTVariableDefinition &variable_defn) {
        TSScope *scope = variable_defn.ts_data->scope;
        assert(scope != nullptr);

        //eval rhs *first*
        //otherwise, things like let z = z can be written -_-
        if (variable_defn.rhs_value) {
            variable_defn.rhs_value->map(*this);
        }
        
        //next, find the type
        std::shared_ptr<ASTLiteral> type_ast = std::dynamic_pointer_cast<ASTLiteral>(variable_defn.type);
        
        if (type_ast->token.type != TokenType::Identifier) {
            std::stringstream error;
            error<<"expected identifier for type. found: "<<type_ast->token;
            error<<type_ast->position;
            throw std::runtime_error(error.str());
        };

        assert(type_ast->token.type == TokenType::Identifier);
        std::string type_name = *type_ast->token.value.ptr_s;
        
        if (!scope->has_type(type_name)) {
            std::stringstream error;
            error<<"unknown type: "<<type_name;
            error<<type_ast->position;
            throw std::runtime_error(error.str());
        }
        

        TSDeclaration &typedecl = scope->get_type_decl(type_name);
        
        //now look for the variable decl
        std::shared_ptr<ASTLiteral> name_ast = std::dynamic_pointer_cast<ASTLiteral>(variable_defn.name);
        
        const std::string &var_name = *name_ast->token.value.ptr_s.get();

        if (scope->has_variable(var_name)) {
            TSDeclaration &prev_var_decl = scope->get_variable_decl(var_name);
            std::stringstream error;
            error<<"\nmultiple definition of variable: "<<var_name;
            error<<"\n\noriginal definition:  "<<prev_var_decl.decl_ast->position;
            error<<"\n\nsecond definition: "<<variable_defn.position;

            throw std::runtime_error(error.str());
        }
        
        name_ast->ts_data->type = &typedecl.type;
        scope->add_variable_decl(var_name, std::unique_ptr<TSDeclaration>(new TSDeclaration(variable_defn, typedecl.type)));

    }

    void map_literal(ASTLiteral &literal) {
        TSScope *scope = literal.ts_data ->scope;
        if (literal.token.type == TokenType::LiteralString) {
            literal.ts_data->type = &scope->get_type_decl("string").type;
        }
        else if (literal.token.type == TokenType::LiteralInt) {
            literal.ts_data->type = &scope->get_type_decl("int").type;
        }
        else if (literal.token.type == TokenType::LiteralFloat) {
            literal.ts_data->type = &scope->get_type_decl("float").type;
        }

        else if (literal.token.type == TokenType::Identifier) {
            std::string &name = *literal.token.value.ptr_s.get();
            if (!scope->has_variable(name)) {
                std::stringstream error;
                error<<"\nunknown identifier: "<<name;
                error<<literal.position;
                throw std::runtime_error(error.str());
            } else {
                literal.ts_data->type = &scope->get_variable_decl(name).type;
            }
        }
        else {
            assert(false && "illegal literal token");
        }
    }

};



TSContext type_system_type_check(std::shared_ptr<IAST> root) {
    TSContext ctx;

    ScopeGenerator scope_generator(ctx, ctx.get_root_scope());
    root->map(scope_generator);

    Typer id_typer;
    root->map(id_typer);


    return ctx;
};
