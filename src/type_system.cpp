#include "type_system.h"
#include "ast.h"
#include <assert.h>
#include <sstream>


int32_t TSTypevar::static_uuid_count = 0;

TSType& TSScope::get_variable_type(const std::string &name) {
    auto it = this->sym_table.find(name);
    if (it == this->sym_table.end()) {
        if(this->parent == nullptr) {
            std::stringstream error;
            error<<"unable to find symbol: ";
            error<<"\""<<name<<"\"";

            throw std::runtime_error(error.str());
        } else {
            return this->parent->get_variable_type(name);
        }
    } else {
        TSType *type = this->sym_table[name];
        assert(type != nullptr);
        return *type; 
    }
}

/*
void TSScope::replace_variable_type(const std::string &name, TSType *new_type) {
    assert(this->has_type(name));

    delete(this->sym_table[name]);
    this->sym_table[name] = new_type;
};
*/

bool TSScope::has_type(const std::string &name) {
    if(this->sym_table.find(name) != this->sym_table.end()) {
        return true;
    } else {
        if (this->parent == nullptr) {
            return false;
        } else {
            return this->parent->has_type(name);
        }
    }
};

void TSScope::add_variable(const std::string &name, TSType *type) {
    assert(!has_type(name));
    assert(type);
    this->sym_table[name] = type;
};


std::ostream& operator <<(std::ostream &out, const TSType &type) {
    switch (type.variant) {
        case TSVariant::Void:
            out<<"void";
            break;

        case TSVariant::String:
            out<<"string";
            break;

        case TSVariant::AbstractInt:
            out<<"i_";
            break;

        case TSVariant::SizedInt:
            out<<"i"<<type.data.size;
            break;

        case TSVariant::AbstractFloat:
            out<<"f_";
            break;

        case TSVariant::SizedFloat:
            out<<"f"<<type.data.size;
            break;

        case TSVariant::Uninitialized:
            out<<"_";
            break;

        case TSVariant::TypeVariable:
            out<<"t"<<type.data.typevar->uuid;
            break;
    };
    return out;
};

/*
std::ostream& operator <<(std::ostream &out, const TSType& type) {
    switch(type.variant) {
        case TSType::Variant::TypeVariable:
            out<<"var-"<<type.typevar_ptr->uuid;
            break;

        case TSType::Variant::Uninitialized:
            out<<"bottom";
            break;

        case TSType::Variant::Void:
            out<<"void";
            break;

        case TSType::Variant::SizedInt:
            out<<"i"<<type.size;
            break;
        
        case TSType::Variant::AbstractInt:
            out<<"iabs";
            break;

        case TSType::Variant::SizedFloat:
            out<<"f"<<type.size;
            break;

        case TSType::Variant::AbstractFloat:
            out<<"fabs";
            break;

        case TSType::Variant::Bool:
            out<<"bool";
            break;

        case TSType::Variant::String:
            out<<"string";
            break;
    };

    return out;
};
*/



struct ASTScopeGenerator : public IASTVisitor {

    TSScope *scope;

    ASTScopeGenerator(TSScope *scope) : scope(scope) {};

    void map_root(ASTRoot &root) {
        recursive_setup_scope(root);
    };

    void map_literal(ASTLiteral &literal) {
        recursive_setup_scope(literal);
    };

    void map_block(ASTBlock &block) {
        //create a new scope that is a child of the current scope
        TSScope *inner_scope = new TSScope(this->scope);

        //create a new scope generator to recursively assign scopes
        ASTScopeGenerator inner_scope_generator(inner_scope);

        //make the inner scope setup on the block
        inner_scope_generator.recursive_setup_scope(block);
    };

    void map_infix_expr(ASTInfixExpr &infix) {
        recursive_setup_scope(infix);
    };

    void map_prefix_expr(ASTPrefixExpr &prefix) {
        recursive_setup_scope(prefix);
    };

    void map_statement(ASTStatement &statement) {
        recursive_setup_scope(statement);
    };

    void map_fn_definition(ASTFunctionDefinition &fn_defn) {
        recursive_setup_scope(fn_defn);
    };

    void map_variable_definition(ASTVariableDefinition &variable_defn) {
        recursive_setup_scope(variable_defn);
    };

    void recursive_setup_scope(IAST &ast) {
        ast.ts_scope = this->scope;
        ast.traverse_inner(*this);
    };
};

#include <iostream>

struct ASTTypeVariableGenerator : public IASTVisitor {

    void map_variable_definition(ASTVariableDefinition &variable_defn) {
        const Token &name_token = variable_defn.name;
        assert(name_token.value.ptr_s && "identifier token does not have proper string");
        std::string name = *name_token.value.ptr_s;

        TSScope *scope = variable_defn.ts_scope;;

        //the variable already exists
        if (scope->has_type(name)) {
            std::stringstream error;
            error<<"multiple definition of variable;";
            error<<"\nname: "<<name;
            error<<"\nprevious definition at: "<<scope->get_variable_type(name).data.typevar->definition_pos;
            

            throw std::runtime_error(error.str());
        };    

        //add in the variable
        TSTypevar *typevar = new TSTypevar(variable_defn.position);
        TSType *type = new TSType(typevar);
        scope->add_variable(name, type);
    };

};

struct ASTLiteralTypeApplier : public IASTVisitor {
    void map_literal(ASTLiteral &literal) {
            
        TSScope *scope = literal.ts_scope;

        if (literal.token.type == TokenType::Int) {
            literal.ts_type = new TSType(TSVariant::AbstractInt);
            literal.traverse_inner(*this); 
        }
        else if (literal.token.type == TokenType::Float) {
            literal.ts_type = new TSType(TSVariant::AbstractFloat);
            literal.traverse_inner(*this); 
        }
        else if (literal.token.type == TokenType::String) {
            literal.ts_type = new TSType(TSVariant::String);
            literal.traverse_inner(*this); 
        }
        else if (literal.token.type == TokenType::Identifier) {        
            assert(literal.token.type == TokenType::Identifier);
            //all identifiers
            assert(literal.token.value.ptr_s && "literal AST name undefined");
            std::string name = *literal.token.value.ptr_s;

            try {
                //set the type up
                literal.ts_type = &scope->get_variable_type(name);
                literal.traverse_inner(*this); 
            }
            catch(std::runtime_error no_type_variable) {
                std::stringstream err;
                err<<no_type_variable.what();
                err<<literal.position;
                throw std::runtime_error(err.str());

            };
        } else {
            assert(false && "should not reach here");
        }
    }
};


class ASTTypeSubstituter : public IASTVisitor {
    void map_variable_definition(ASTVariableDefinition &variable_defn) {
        TSScope *scope = variable_defn.ts_scope;
        std::string &var_name = *variable_defn.name.value.ptr_s;
    
    };
};

//--------------------------------
//TYPE INFERENCE ALGORITHM


void populate_default_identifiers(TSScope *root) {
    root->add_variable("void",new TSType(TSVariant::Void));
    root->add_variable("i64", new TSType(TSVariant::SizedInt, 64));
    root->add_variable("i32", new TSType(TSVariant::SizedInt, 32));
    root->add_variable("i16", new TSType(TSVariant::SizedInt, 16));
}


void type_system_type_check(std::shared_ptr<IAST> &program) {
    TSScope *root_scope = new TSScope(nullptr);
    populate_default_identifiers(root_scope);

    ASTScopeGenerator scope_generator(root_scope);
    program->map(scope_generator);

    ASTTypeVariableGenerator typevar_generator;
    program->map(typevar_generator);

    //creates typevaras for identifiers
    //creates abstract / concrete types for literals
    ASTLiteralTypeApplier type_applier;
    program->map(type_applier);

    ASTTypeSubstituter type_substituter;
};
