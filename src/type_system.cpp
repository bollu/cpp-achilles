#include "type_system.h"
#include "ast.h"
#include <sstream>
#include <iostream>

/*
   TSType *int_type = new TSType(TSType::Variant::Int, nullptr);
   TSType *float_type = new TSType(TSType::Variant::Float, nullptr);
   TSType *string_type = new TSType(TSType::Variant::String, nullptr);
 */

TSType::TSType(const TSFunctionTypeData func_data,
               PositionRange            *decl_pos) : decl_pos(decl_pos) {
    this->variant   = Variant::Function;
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

        for (auto arg: type.func_data->args) {
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

const TSType& get_type_from_ast(TSScope              *parent_scope,
                                std::shared_ptr<IAST>ast) {
    std::shared_ptr<ASTLiteral>type_ast = std::dynamic_pointer_cast<ASTLiteral>(
        ast);

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


    const TSType *typedecl = parent_scope->get_type(type_name);
    return *typedecl;
}

class ScopeGenerator : public IASTGenericVisitor {
private:

    TSScope    *scope;
    TSContext& context;

public:

    ScopeGenerator(TSContext& context, TSScope *scope) : context(context),
        scope(
            scope) {}

    void inspect_ast(IAST& ast) {
        ast.ts_data = std::make_shared<TSASTData>(scope);
        ast.traverse_inner(*this);
    }

    void inspect_fn_definition(ASTFunctionDefinition& fn_defn) {
        // return type should belong to outer scope
        this->inspect_ast(*fn_defn.return_type);

        // create a new scope for the function defn into which args are also
        // injected
        TSScope        *func_scope = context.create_child_scope(this->scope);

        ScopeGenerator fn_scope_generator(context, func_scope);

        for (auto arg: fn_defn.args) {
            std::shared_ptr<ASTLiteral>arg_name_ast  =
                std::dynamic_pointer_cast<ASTLiteral>(arg.first);
            std::shared_ptr<ASTLiteral>type_name_ast =
                std::dynamic_pointer_cast<ASTLiteral>(arg.second);

            fn_scope_generator.inspect_ast(*arg_name_ast);
            fn_scope_generator.inspect_ast(*type_name_ast);
        }

        fn_scope_generator.inspect_ast(*fn_defn.body);
    }

    void inspect_block(ASTBlock& block) {
        TSScope        *inner_scope          = context.create_child_scope(
            this->scope);
        ScopeGenerator inner_scope_generator = ScopeGenerator(context,
                                                              inner_scope);

        // make the inner scope setup the block
        inner_scope_generator.inspect_ast(block);
    }
};


const TSType& get_type_for_type_literal(ASTLiteral& type_ast) {
    assert(type_ast.token.type == TokenType::Identifier
           && " TokenType::Idenfitier required on ast->token.type");
    const std::string& name = *type_ast.token.value.ptr_s;

    TSScope            *scope = type_ast.ts_data->scope;
    assert(scope);

    if (!scope->has_type(name)) {
        std::stringstream error;
        error << "unknown type: " << name;
        error << type_ast.position;

        throw std::runtime_error(error.str());
    }

    const TSType *type = scope->get_type(name);
    return *type;
}

const TSVariable& get_var_for_literal(ASTLiteral& var_name_ast) {
    assert(var_name_ast.token.type == TokenType::Identifier
           && " TokenType::Idenfitier required on ast->token.type");
    const std::string& name = *var_name_ast.token.value.ptr_s;

    TSScope            *scope = var_name_ast.ts_data->scope;
    assert(scope);

    if (!scope->has_variable(name)) {
        std::stringstream error;
        error << "unknown identifier: " << name;
        error << var_name_ast.position;

        throw std::runtime_error(error.str());
    }

    const TSVariable *var = scope->get_variable(name);
    return *var;
}

void create_new_variable_for_variable_identifier(ASTLiteral  & var_name_ast,
                                                 const TSType& var_type) {
    assert(var_name_ast.token.type == TokenType::Identifier
           && " TokenType::Idenfitier required on ast->token.type");
    const std::string& name = *var_name_ast.token.value.ptr_s;

    TSScope            *scope = var_name_ast.ts_data->scope;

    if (scope->has_variable(name)) {
        std::stringstream error;
        error << "\nredefinition of variable: " << name;
        error << "\nfirst occurence: " << *scope->get_variable(name)->decl_pos;
        error << "\nredefinition: " << var_name_ast.position;

        throw std::runtime_error(error.str());
    }
    TSVariable *var = new TSVariable(name, &var_type, &var_name_ast.position);
    scope->add_variable(name, var);
    var_name_ast.ts_data->type = &var_type;
}

bool is_number(const TSType& type) {
    if ((type.variant == TSType::Variant::Int32)
        || (type.variant == TSType::Variant::Int)
        || (type.variant == TSType::Variant::Float)
        || (type.variant == TSType::Variant::Float32)) {
        return true;
    }
    return false;
}

const TSType* get_promoted_num_type(const TSType *t1, const TSType *t2) {
    if ((t1 == f32_type)
        || (t2 == f32_type)) {
        return f32_type;
    }

    if ((t1 == float_type)
        || (t2 == float_type)) {
        return float_type;
    }

    if ((t1 == i32_type)
        || (t2 == i32_type)) {
        return i32_type;
    }
    return int_type;
}

class Typer : public IASTVisitor {
    virtual void inspect_variable_definition(
        ASTVariableDefinition& variable_defn) {
        variable_defn.rhs_value->dispatch(*this);
        ASTLiteral& var_type_ast =
            dynamic_cast<ASTLiteral&>(*variable_defn.type);

        const TSType& type = get_type_for_type_literal(var_type_ast);
        var_type_ast.ts_data->type = &type;

        ASTLiteral& var_name_ast =
            dynamic_cast<ASTLiteral&>(*variable_defn.name);
        create_new_variable_for_variable_identifier(var_name_ast, type);
    }

    virtual void inspect_fn_call(ASTFunctionCall& fn_call) {
        const TSType& type =
            *get_var_for_literal(dynamic_cast<ASTLiteral&>(*fn_call.name)).type;

        fn_call.ts_data->type = &type;

        for (auto arg : fn_call.params) {
            arg->dispatch(*this);
        }
    }

    virtual void inspect_statement(ASTStatement& statement) {
        statement.ts_data->type = void_type;
    }

    virtual void inspect_literal(ASTLiteral& literal) {
        switch (literal.token.type) {
        case TokenType::Identifier:
        {
            TSScope      *scope = literal.ts_data->scope;

            const TSVariable& variable = get_var_for_literal(literal);
            literal.ts_data->type = variable.type;
            break;
        }

        case TokenType::LiteralString:
            literal.ts_data->type = string_type;
            break;

        case TokenType::LiteralInt:
            literal.ts_data->type = int_type;
            break;

        case TokenType::LiteralFloat:
            literal.ts_data->type = float_type;
            break;

        default:
            assert(false
                   && "unknown token type for literal");
        }
    }

    virtual void inspect_infix_expr(ASTInfixExpr& expr) {
        expr.left->dispatch(*this);
        expr.right->dispatch(*this);

        assert(expr.left->ts_data->type);
        assert(expr.right->ts_data->type);

        const TSType **t1 = &expr.left->ts_data->type;
        const TSType **t2 = &expr.right->ts_data->type;

        TokenType type = expr.op.type;

        if ((type == TokenType::Plus)
            || (type == TokenType::Minus)
            || (type == TokenType::Multiply)
            || (type == TokenType::Divide)) {
            if (!is_number(**t1)) {
                std::stringstream error;
                error << "expected number type. Found: " << **t1;
                error << expr.position;

                throw std::runtime_error(error.str());
            }

            if (!is_number(**t2)) {
                std::stringstream error;
                error << "expected number type. Found: " << **t2;
                error << expr.position;

                throw std::runtime_error(error.str());
            }

            const TSType *promoted_type = get_promoted_num_type(*t1, *t2);
            *t1 = promoted_type;
            *t2 = promoted_type;
            expr.ts_data->type = promoted_type;
            return;
        }

        std::stringstream error;
        error << "unknown infix expr to typecheck";
        error << expr.position;
        throw std::runtime_error(error.str());
    }
};

TSContext type_system_type_check(std::shared_ptr<IAST>root) {
    TSContext      ctx;

    ScopeGenerator scope_generator(ctx, ctx.get_root_scope());

    root->dispatch(scope_generator);

    Typer typer;
    root->dispatch(typer);


    return ctx;
}
