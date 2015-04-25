#include "type_system.h"
#include "ast.h"
#include <sstream>
#include <iostream>

TSType::TSType(const TSFunctionTypeData& func_data){
    this->variant   = Variant::Function;
    this->func_data = std::make_shared<TSFunctionTypeData>(func_data);
}


std::ostream& operator <<(std::ostream& out, const TSType& type){
    out << "t-";
    switch (type.variant) {
    case TSType::Variant::Void:
        out << "void";
        break;

    case TSType::Variant::Int32:
        out << "i32";
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
        for (auto arg: type.func_data->args) {
            out << arg;

            if (num_commas > 0) {
                out << ", ";
            }
        }
        out << ")";
        out << " -> " << type.func_data->return_type;

        break;
    }
    return out;
}


TSType& get_type_from_ast(TSScope *parent_scope, std::shared_ptr<IAST> ast){
    std::shared_ptr<ASTLiteral> type_ast = std::dynamic_pointer_cast<ASTLiteral>(ast);

    assert(type_ast);

    if (type_ast->token.type != TokenType::Identifier) {
        std::stringstream error;
        error << "expected identifier for type. found: " << type_ast->token;
        error << type_ast->position;
        throw std::runtime_error(error.str());
    }

    assert(type_ast->token.type == TokenType::Identifier);
    std::string type_name = *type_ast->token.value.ptr_s;

    if (!parent_scope->has_type(type_name)) {
        std::stringstream error;
        error << "unknown type: " << type_name;
        error << type_ast->position;
        throw std::runtime_error(error.str());
    }


    TSType& typedecl = parent_scope->get_type(type_name);
    return typedecl;
}


class ScopeGenerator : public IASTGenericVisitor {
private:
TSScope    *scope;
TSContext& context;
public:
ScopeGenerator(TSContext& context, TSScope *scope) : context(context), scope(scope){
}

void map_ast(IAST& ast){
    ast.ts_data = std::make_shared<TSASTData>(scope);
    ast.traverse_inner(*this);
}

void map_fn_definition(ASTFunctionDefinition& fn_defn){
    //return type should belong to outer scope
    this->map_ast(*fn_defn.return_type);

    //create a new scope for the function defn into which args are also injected
    TSScope        *func_scope = context.create_child_scope(this->scope);

    ScopeGenerator fn_scope_generator(context, func_scope);

    for (auto arg: fn_defn.args) {
        std::shared_ptr<ASTLiteral> arg_name_ast  = std::dynamic_pointer_cast<ASTLiteral>(arg.first);
        std::shared_ptr<ASTLiteral> type_name_ast = std::dynamic_pointer_cast<ASTLiteral>(arg.second);

        fn_scope_generator.map_ast(*arg_name_ast);
        fn_scope_generator.map_ast(*type_name_ast);
    }

    fn_scope_generator.map_ast(*fn_defn.body);
}

void map_block(ASTBlock& block){
    TSScope        *inner_scope          = context.create_child_scope(this->scope);
    ScopeGenerator inner_scope_generator = ScopeGenerator(context, inner_scope);

    //make the inner scope setup the block
    inner_scope_generator.map_ast(block);
}
};


TSType& get_type_for_type_literal(ASTLiteral& type_ast){
    assert(type_ast.token.type == TokenType::Identifier && " TokenType::Idenfitier required on ast->token.type");
    const std::string& name = *type_ast.token.value.ptr_s;

    TSScope            *scope = type_ast.ts_data->scope;
    assert(scope);
    if (!scope->has_type(name)) {
        std::stringstream error;
        error << "unknown type: " << name;
        error << type_ast.position;

        throw std::runtime_error(error.str());
    }

    TSType& type = scope->get_type(name);
    return type;
}


TSType& get_type_for_var_literal(ASTLiteral& var_name_ast){
    assert(var_name_ast.token.type == TokenType::Identifier && " TokenType::Idenfitier required on ast->token.type");
    const std::string& name = *var_name_ast.token.value.ptr_s;

    TSScope            *scope = var_name_ast.ts_data->scope;
    assert(scope);
    if (!scope->has_variable(name)) {
        std::stringstream error;
        error << "unknown identifier: " << name;
        error << var_name_ast.position;

        throw std::runtime_error(error.str());
    }

    TSType& type = scope->get_variable(name);
    return type;
}


void create_new_variable_for_variable_identifier(ASTLiteral& var_name_ast, TSType& var_type){
    assert(var_name_ast.token.type == TokenType::Identifier && " TokenType::Idenfitier required on ast->token.type");
    const std::string& name = *var_name_ast.token.value.ptr_s;

    TSScope            *scope = var_name_ast.ts_data->scope;
    if (scope->has_variable(name)) {
        std::stringstream error;
        error << "\nredefinition of variable: " << name;
        error << "\nfirst occurence: " << scope->get_decl(name);
        error << "\nredefinition: " << var_name_ast.position;

        throw std::runtime_error(error.str());
    }
    scope->add_variable(name, var_type);
    scope->add_decl(name, var_name_ast.position);
    var_name_ast.ts_data->type = &var_type;
}


class Typer : public IASTVisitor {
virtual void map_variable_definition(ASTVariableDefinition& variable_defn){
    variable_defn.rhs_value->map(*this);
    ASTLiteral& var_type_ast = dynamic_cast<ASTLiteral&>(*variable_defn.type);

    TSType&     type = get_type_for_type_literal(var_type_ast);
    var_type_ast.ts_data->type = &type;

    ASTLiteral& var_name_ast = dynamic_cast<ASTLiteral&>(*variable_defn.name);
    create_new_variable_for_variable_identifier(var_name_ast, type);
}

virtual void map_literal(ASTLiteral& literal){
    switch (literal.token.type) {
    case TokenType::Identifier:
    {
        TSScope      *scope = literal.ts_data->scope;
        std::string& name   = *literal.token.value.ptr_s;

        TSType&      type = get_type_for_var_literal(literal);
        literal.ts_data->type = &type;
        break;
    }

    case TokenType::LiteralString:
        literal.ts_data->type = &literal.ts_data->scope->get_type("string");
        break;

    case TokenType::LiteralInt:
        literal.ts_data->type = &literal.ts_data->scope->get_type("int");
        break;

    case TokenType::LiteralFloat:
        literal.ts_data->type = &literal.ts_data->scope->get_type("float");
        break;

    default:
        assert(false && "unknown token type for literal");
    }
}
};

TSContext type_system_type_check(std::shared_ptr<IAST> root){
    TSContext      ctx;

    ScopeGenerator scope_generator(ctx, ctx.get_root_scope());

    root->map(scope_generator);

    Typer typer;
    root->map(typer);


    return ctx;
}
