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
struct TSFunctionTypeData;

struct TSType
{
public:
    enum class Variant
    {
        Void,
        String,
        Int,         //abstract int to be unified
        Float,       //abstract float to be unified
        Int32,
        Function,
    }
                                        variant;

    std::shared_ptr<TSFunctionTypeData> func_data;
    TSType(const TSFunctionTypeData& func_data);

private:
    friend class TSContext;
    TSType(Variant variant) : variant(variant){
    }
};

std::ostream& operator <<(std::ostream& out, const TSType& type);

struct TSFunctionTypeData
{
    TSType              return_type;
    std::vector<TSType> args;

    TSFunctionTypeData(std::vector<TSType> args, TSType return_type) :
        args(args), return_type(return_type){
    }
};


struct TSScope
{
public:
    TSType& get_type(const std::string& name){
        auto it = this->typename_to_type.find(name);

        if (it != this->typename_to_type.end()) {
            return *it->second.get();
        }else {
            if (this->parent != nullptr) {
                return this->parent->get_type(name);
            }else {
                assert(false && "do not have type");
            }
        }
    }

    TSType& get_variable(const std::string& name){
        auto it = this->varname_to_var.find(name);

        if (it != this->varname_to_var.end()) {
            return it->second;
        }else {
            if (this->parent != nullptr) {
                return this->parent->get_variable(name);
            }else {
                assert(false && "do not have variable");
            }
        }
    }

    PositionRange get_decl(const std::string& name){
        auto it = this->name_to_decl.find(name);

        if (it != this->name_to_decl.end()) {
            return it->second;
        }else {
            if (this->parent != nullptr) {
                return this->parent->get_decl(name);
            }else {
                assert(false && "do not have declaration");
            }
        }
    }

    bool has_type(const std::string& name){
        auto it = this->typename_to_type.find(name);

        if (it == this->typename_to_type.end()) {
            if (this->parent == nullptr) {
                return false;
            }else {
                return this->parent->has_type(name);
            }
        }else {
            return true;
        }
    }

    bool has_variable(const std::string& name){
        auto it = this->varname_to_var.find(name);

        if (it == this->varname_to_var.end()) {
            if (this->parent == nullptr) {
                return false;
            }else {
                return this->parent->has_variable(name);
            }
        }else {
            return true;
        }
    }

    void add_variable(const std::string& name, TSType& type){
        assert(!this->has_variable(name));
        this->varname_to_var.insert(std::pair<std::string, TSType&>(name, type));
        //this->varname_to_var[name] = type;
    }

    void add_type(const std::string& name, std::unique_ptr<TSType> decl){
        assert(!this->has_type(name));
        this->typename_to_type[name] = std::move(decl);
    }

    void add_decl(const std::string& name, const PositionRange& position){
        assert(!this->has_decl(name));
        this->name_to_decl.insert(std::pair<std::string, PositionRange>(name, position));
    }

private:
    TSScope *parent;
    TSScope(TSScope *parent) : parent(parent){
    }

    std::map<std::string, std::unique_ptr<TSType> > typename_to_type;
    std::map<std::string, TSType&>                  varname_to_var;

    std::map<std::string, PositionRange>            name_to_decl;

    friend class TSContext;

    bool has_decl(const std::string& name){
        if (this->name_to_decl.find(name) != this->name_to_decl.end()) {
            return true;
        }else {
            if (this->parent != nullptr) {
                return this->parent->has_decl(name);
            }else {
                return false;
            }
        }
    }
};

struct TSContext
{
private:
    //we need to store a vector of pointers since references to a frikkin
    //STL vector may get invalidated.
    std::vector<std::shared_ptr<TSScope> > scopes;
public:
    TSContext(){
        auto root_scope = std::shared_ptr<TSScope>(new TSScope(nullptr));

        root_scope->add_type("i32", std::unique_ptr<TSType>(new TSType(TSType(TSType::Variant::Int32))));
        root_scope->add_type("int", std::unique_ptr<TSType>(new TSType(TSType(TSType::Variant::Int))));
        root_scope->add_type("float", std::unique_ptr<TSType>(new TSType(TSType(TSType::Variant::Float))));
        root_scope->add_type("void", std::unique_ptr<TSType>(new TSType(TSType(TSType::Variant::Void))));
        root_scope->add_type("string", std::unique_ptr<TSType>(new TSType(TSType(TSType::Variant::String))));

        scopes.push_back(root_scope);
    }

    TSScope *get_root_scope(){
        return this->scopes.at(0).get();
    }

    TSScope *create_child_scope(TSScope *parent){
        scopes.push_back(std::shared_ptr<TSScope>(new TSScope(parent)));
        return this->scopes[this->scopes.size() - 1].get();
    }
};

struct TSASTData
{
    TSScope *scope;
    TSType  *type;

    TSASTData(TSScope *scope) : scope(scope), type(nullptr){
    }
};

TSContext type_system_type_check(std::shared_ptr<IAST> root);
