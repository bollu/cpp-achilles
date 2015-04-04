#pragma once
#include <map>
#include <vector>
#include <memory>
#include "file_handling.h"

class IAST;

struct TSType;
struct TSScope;
struct TSContext;
struct TSASTData;

struct TSType {
    public:
        enum class Variant {
            Void,
            String,
            Int, //abstract int to be unified
            Float, //abstract float to be unified
            Int32,
        } variant;

    private:
        friend class TSScope;
        friend class TSContext;

        TSType(Variant variant) : variant(variant){};
};

std::ostream &operator <<(std::ostream &out, const TSType &type);

struct TSDeclaration {
    std::unique_ptr<IAST> decl_ast;
    TSType type;
    
    TSDeclaration(IAST &decl_ast, TSType type) : decl_ast(&decl_ast), type(type) {};
    TSDeclaration(TSType type) : decl_ast(nullptr), type(type) {};
};

struct TSScope {
    public:
        TSDeclaration& get_type_decl(const std::string &name) {
            auto it = this->typename_to_type.find(name);
            if (it != this->typename_to_type.end()) {
                return *it->second.get();
            } else {
                if (this->parent != nullptr) {
                    return this->parent->get_type_decl(name);
                } else {
                    assert(false && "do not have type");
                }
            }
        };

        TSDeclaration& get_variable_decl(const std::string &name) {
            auto it = this->varname_to_var.find(name);
            if (it != this->varname_to_var.end()) {
                return *it->second.get();
            } else {
                if (this->parent != nullptr) {
                    return this->parent->get_variable_decl(name);
                } else {
                    assert(false && "do not have type");
                }
            }
        };

        bool has_type(const std::string &name) {
            auto it = this->typename_to_type.find(name);
            if (it == this->typename_to_type.end()) {
                if (this->parent == nullptr) {
                    return false;
                } else {
                    return this->parent->has_type(name);
                }
            } else {
                return true;
            }
        };

        bool has_variable(const std::string &name) {
            auto it = this->varname_to_var.find(name);
            if (it == this->varname_to_var.end()) {
                if (this->parent == nullptr) {
                    return false;
                } else {
                    return this->parent->has_variable(name);
                }
            } else {
                return true;
            }
        };

        void add_variable_decl(const std::string &name, std::unique_ptr<TSDeclaration> decl){
            assert(!this->has_variable(name));
            this->varname_to_var[name] = std::move(decl);
        };

        void add_type_decl(const std::string &name, std::unique_ptr<TSDeclaration> decl) {
            assert(!this->has_type(name));
            this->typename_to_type[name] = std::move(decl);
        };
    private:
        TSScope* parent;
        TSScope(TSScope *parent) : parent(parent) {};
    
        std::map<std::string, std::unique_ptr<TSDeclaration>> typename_to_type;
        std::map<std::string, std::unique_ptr<TSDeclaration>> varname_to_var;

        friend class TSContext;
};


struct TSContext {
    private:
        //we need to store a vector of pointers since references to a frikkin
        //STL vector may get invalidated.
        std::vector<std::shared_ptr<TSScope>> scopes;
    public:
        TSContext() {
            auto root_scope = std::shared_ptr<TSScope>(new TSScope(nullptr));

            root_scope->add_type_decl("i32", std::unique_ptr<TSDeclaration>(new TSDeclaration(TSType(TSType::Variant::Int32))));
            root_scope->add_type_decl("int", std::unique_ptr<TSDeclaration>(new TSDeclaration(TSType(TSType::Variant::Int))));
            root_scope->add_type_decl("float", std::unique_ptr<TSDeclaration>(new TSDeclaration(TSType(TSType::Variant::Float))));
            root_scope->add_type_decl("void", std::unique_ptr<TSDeclaration>(new TSDeclaration(TSType(TSType::Variant::Void))));
            root_scope->add_type_decl("string", std::unique_ptr<TSDeclaration>(new TSDeclaration(TSType(TSType::Variant::String))));

            scopes.push_back(root_scope);
        };

        TSScope* get_root_scope() {
            return this->scopes.at(0).get();
        }; 

        TSScope *create_child_scope(TSScope *parent) {
            scopes.push_back(std::shared_ptr<TSScope>(new TSScope(parent)));
            return this->scopes[this->scopes.size() - 1].get();
        }
};

struct TSASTData {
    TSScope *scope;
    TSType *type;

    TSASTData(TSScope *scope) : scope(scope), type(nullptr) {};
};

TSContext type_system_type_check(std::shared_ptr<IAST> root);
