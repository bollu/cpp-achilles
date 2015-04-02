#include "type_system.h"
#include "ast.h"
#include <assert.h>
#include <sstream>
#include <iostream>
#include <iostream>



int32_t TSTypevar::static_uuid_count = 0;
std::vector<TSType> TSScope::types;

TSType TSScope::get_symbol_type(const std::string &name) {
    auto it = this->name_to_index_map.find(name);
    if (it == this->name_to_index_map.end()) {
        if(this->parent == nullptr) {
            std::stringstream error;
            error<<"unable to find symbol: ";
            error<<"\""<<name<<"\"";

            throw std::runtime_error(error.str());
        } else {
            return this->parent->get_symbol_type(name);
        }
    } else {
        TypevarUUID index = it->second;
        return this->get_symbol_type(index); 
    }

}

TSType TSScope::get_symbol_type(const TypevarUUID &id ){
    assert(id >= 0);
    return this->types.at(id);
};

void TSScope::replace_type(const TypevarUUID &id, TSType new_type) {
    assert(id >= 0 && id < this->types.size());
    std::cout<<"\nreplacing: "<<this->types[id]<<" with "<<new_type;
    this->types[id] = new_type;
}
TypevarUUID TSScope::get_symbol_uuid(const std::string &name) {
    auto it = this->name_to_index_map.find(name);

    if (it == this->name_to_index_map.end()) {
        if (this->parent == nullptr) {
            std::stringstream error;
            error<<"unable to find symbol: "<<name;
            throw std::runtime_error(error.str());

        } else {
            return this->parent->get_symbol_uuid(name);
        }
    }

    return it->second;
};

bool TSScope::has_type(const std::string &name) {
    if(this->name_to_index_map.find(name) != this->name_to_index_map.end()) {
        return true;
    } else {
        if (this->parent == nullptr) {
            return false;
        } else {
            return this->parent->has_type(name);
        }
    }
};


TypevarUUID TSScope::add_symbol(std::string name, TSType type) {
    assert(!this->has_type(name));
    this->types.push_back(type);
    TypevarUUID index = this->types.size() - 1;

    this->name_to_index_map[name] = index;

    return index;
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




//AST VISITORS----------------------------------------
struct ASTScopeGenerator : public IASTGenericVisitor {

    TSScope *scope;

    ASTScopeGenerator(TSScope *scope) : scope(scope) {};

    void map_block(ASTBlock &block) {
        //create a new scope that is a child of the current scope
        TSScope *inner_scope = new TSScope(this->scope);

        //create a new scope generator to recursively assign scopes
        ASTScopeGenerator inner_scope_generator(inner_scope);

        //make the inner scope setup on the block
        inner_scope_generator.map_ast(block);
    };

    void map_ast(IAST &ast) {
        ast.ts_scope = this->scope;
        ast.traverse_inner(*this);
    };
};


struct ASTTypeVariableCreator : public IASTVisitor {

    void map_variable_definition(ASTVariableDefinition &variable_defn) {
        const Token &name_token = variable_defn.name;
        assert(name_token.value.ptr_s && "identifier token does not have proper string");
        std::string var_name = *name_token.value.ptr_s;

        TSScope *scope = variable_defn.ts_scope;;

        //the variable already exists
        if (scope->has_type(var_name)) {
            std::stringstream error;
            error<<"multiple definition of variable;";
            error<<"\nname: "<<var_name;
            error<<"\ncurrent definition at: "<<variable_defn.position;
            error<<"\nprevious definition at: "<<scope->get_symbol_type(var_name).data.typevar->definition_pos;


            throw std::runtime_error(error.str());
        };    

        //construct the new type variable
        scope->add_symbol(var_name, TSType::Typevar(variable_defn.position));
    };

};

struct ASTTypeVariableSetter : public IASTVisitor {
    void map_literal(ASTLiteral &literal) {

        TSScope *scope = literal.ts_scope;

        if (literal.token.type == TokenType::Int) {
            literal.ts_typevar_uuid = scope->get_symbol_uuid("int");
            literal.traverse_inner(*this); 
        }
        else if (literal.token.type == TokenType::Float) {
            literal.ts_typevar_uuid = scope->get_symbol_uuid("float");
            literal.traverse_inner(*this); 
        }
        else if (literal.token.type == TokenType::String) {
            literal.ts_typevar_uuid = scope->get_symbol_uuid("string");
            literal.traverse_inner(*this); 
        }
        else if (literal.token.type == TokenType::Identifier) {        
            //all identifiers
            assert(literal.token.value.ptr_s && "literal AST name undefined");
            std::string name = *literal.token.value.ptr_s;

            try {
                //set the type up
                assert(literal.ts_typevar_uuid < 0);
                //&scope->get_symbol_type(name);
                //std::cout<<"\n name: "<<name<<" | "<<scope->get_symbol_type(name); 
                TypevarUUID uuid = scope->get_symbol_uuid(name);
                literal.ts_typevar_uuid = uuid;
            }
            catch (std::runtime_error no_type_variable) {
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


class ASTSubsGenerator : public IASTVisitor {
    void map_variable_definition(ASTVariableDefinition &variable_defn) {
        TSScope *scope = variable_defn.ts_scope;
        std::string &var_name = *variable_defn.name.value.ptr_s;

        TSType lhs_type = scope->get_symbol_type(var_name);

        //substitute LHS with the RHS
        if(variable_defn.rhs_value != nullptr) {
            lhs_type.data.typevar->substitution_uuid = variable_defn.rhs_value->ts_typevar_uuid;

        }
    }
};


class ASTSubsApplier : public IASTGenericVisitor {

    void map_ast(IAST &ast) {
        TSScope *scope = ast.ts_scope;
        TypevarUUID non_conrete_uuid = ast.ts_typevar_uuid;
        
        TypevarUUID concrete_uuid = this->substitute_for_concrete_type(scope, non_conrete_uuid); 
    
        std::cout<<"\nmapping:";
        scope->replace_type(non_conrete_uuid, scope->get_symbol_type(concrete_uuid));
        
        ast.ts_typevar_uuid = concrete_uuid;
        ast.traverse_inner(*this);
    }

    TypevarUUID substitute_for_concrete_type(TSScope *scope, const TypevarUUID &base_uuid) {

        assert(base_uuid >= 0);
        TypevarUUID seeker_uuid = base_uuid;
        while(scope->get_symbol_type(seeker_uuid).variant == TSVariant::TypeVariable) {
            assert(seeker_uuid >= 0);
            seeker_uuid = scope->get_symbol_type(seeker_uuid).data.typevar->substitution_uuid;
        }

        assert(scope->get_symbol_type(seeker_uuid).variant != TSVariant::TypeVariable);
        return seeker_uuid;
    }

};


//--------------------------------------------------
//CONSTAINTS
class ASTConstraintVariableDefinition : public IASTVisitor {

    public:

        void map_variable_definition(ASTVariableDefinition &variable_defn) {
            TSScope *scope = variable_defn.ts_scope;

            if (variable_defn.type) {
                std::string &name = *variable_defn.name.value.ptr_s;
                TypevarUUID lhs_uuid = scope->get_symbol_uuid(name);

                TypevarUUID expected_uuid = variable_defn.type->ts_typevar_uuid;

                if (lhs_uuid != expected_uuid) {
                    std::stringstream error;
                    error<<"types do not match for variable declaration";
                    error<<"\nexpected type: "<<scope->get_symbol_type(expected_uuid);
                    error<<"\nfound type: "<<scope->get_symbol_type(lhs_uuid);
                    error<<variable_defn.position;

                    throw std::runtime_error(error.str());
                }

            }
        };
};


//---------------------------------------------------
//CORE ALGORITHM
void populate_default_identifiers(TSScope *root) {
    root->add_symbol("void", TSType::Void());
    root->add_symbol("string", TSType::String());

    root->add_symbol("int", TSType::AbstractInt());
    root->add_symbol("i64", TSType::SizedInt(64));
    root->add_symbol("i32", TSType::SizedInt(32u));
    root->add_symbol("i16", TSType::SizedInt(16));

    root->add_symbol("float", TSType::AbstractFloat());
    root->add_symbol("f64", TSType::SizedFloat(64));
    root->add_symbol("f32", TSType::SizedFloat(32));
    root->add_symbol("f16", TSType::SizedInt(16));
}


void type_system_type_check(std::shared_ptr<IAST> &program) {
    TSScope *root_scope = new TSScope(nullptr);
    populate_default_identifiers(root_scope);

    ASTScopeGenerator scope_generator(root_scope);
    program->map(scope_generator);


    //generate typevars 
    ASTTypeVariableCreator typevar_generator;
    program->map(typevar_generator);


    //creates typevaras for identifiers
    //creates abstract / concrete types for literals
    ASTTypeVariableSetter type_applier;
    program->map(type_applier);




    //generate substitutions for entire program
    ASTSubsGenerator subs_generator;
    program->map(subs_generator);

    //apply substitutions
    ASTSubsApplier subs_applier;
    program->map(subs_applier);


    return;
    //constraints variable declaration
    ASTConstraintVariableDefinition constraint_var_defn;
    program->map(constraint_var_defn);
};
