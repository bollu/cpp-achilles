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
        Int,     // abstract int to be unified
        Float,   // abstract float to be unified
        Float32,
        Int32,
        Function,
    } variant;

    std::shared_ptr<TSFunctionTypeData>func_data;
    PositionRange *decl_pos;

    TSType(Variant variant, PositionRange *decl_pos) : variant(variant) {}

    TSType(const TSFunctionTypeData func_data,
           PositionRange            *decl_pos);
};
static const TSType *int_type = new TSType(TSType::Variant::Int, nullptr);
static const TSType *float_type = new TSType(TSType::Variant::Float, nullptr);
static const TSType *string_type = new TSType(TSType::Variant::String, nullptr);
static const TSType *void_type = new TSType(TSType::Variant::Void, nullptr);

static const TSType *i32_type = new TSType(TSType::Variant::Int32, nullptr);
static const TSType *f32_type = new TSType(TSType::Variant::Float32, nullptr);

std::ostream& operator<<(std::ostream& out,
                         const TSType& type);

struct TSFunctionTypeData
{
    const TSType *return_type;
    std::vector<const TSType *>args;

    TSFunctionTypeData(std::vector<const TSType *>args,
                       const TSType               *return_type) :
        args(args), return_type(return_type) {}
};

struct TSVariable {
    std::string name;
    const TSType *type;

    PositionRange *decl_pos;

    TSVariable(std::string name, const TSType *type,
               PositionRange *decl_pos) : name(
            name), type(type), decl_pos(decl_pos) {}
};

struct TSScope
{
public:

    const TSType* get_type(const std::string& name) {
        auto it = this->typename_to_type.find(name);

        if (it != this->typename_to_type.end()) {
            return it->second;
        } else {
            if (this->parent != nullptr) {
                return this->parent->get_type(name);
            } else {
                assert(false && "do not have type");
            }
		}
		return nullptr;
    }

    const TSVariable* get_variable(const std::string& name) {
        auto it = this->varname_to_var.find(name);

        if (it != this->varname_to_var.end()) {
            return it->second;
        } else {
            if (this->parent != nullptr) {
                return this->parent->get_variable(name);
            } else {
                assert(false && "do not have variable");
            }
        }
		return nullptr;

	}

    bool has_type(const std::string& name) {
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
    }

    bool has_variable(const std::string& name) {
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
    }

    void add_variable(const std::string& name, TSVariable *type) {
        assert(!this->has_variable(name));
        this->varname_to_var.insert(std::make_pair(name, type));

        // this->varname_to_var[name] = type;
    }

    void add_type(const std::string& name, const TSType *type) {
        assert(!this->has_type(name));
        this->typename_to_type[name] = type;
    }

private:

    TSScope *parent;
    TSScope(TSScope *parent) : parent(parent) {}

    std::map<std::string, const TSType *>typename_to_type;
    std::map<std::string, const TSVariable *>varname_to_var;


    friend struct TSContext;
};

struct TSContext
{
private:

    // we need to store a vector of pointers since references to a frikkin
    // STL vector may get invalidated.
    std::vector<std::shared_ptr<TSScope> >scopes;

public:

    TSContext() {
        auto root_scope = std::shared_ptr<TSScope>(new TSScope(nullptr));


        TSType *sin_type =
            new TSType(TSFunctionTypeData({ f32_type }, f32_type),
                       nullptr);

        root_scope->add_variable("sin",
                                 new TSVariable("sin", sin_type, nullptr));
        root_scope->add_type("i32", i32_type);
        root_scope->add_type("f32", f32_type);
        root_scope->add_type("void", void_type);
        root_scope->add_type("string", string_type);


        scopes.push_back(root_scope);
    }

    TSScope* get_root_scope() {
        return this->scopes.at(0).get();
    }

    TSScope* create_child_scope(TSScope *parent) {
        scopes.push_back(std::shared_ptr<TSScope>(new TSScope(parent)));
        return this->scopes[this->scopes.size() - 1].get();
    }
};

struct TSASTData
{
    TSScope *scope;
    const TSType  *type;

    TSASTData(TSScope *scope, const TSType *type) : scope(scope), type(type) {}
};

TSContext type_system_type_check(std::shared_ptr<IAST>root);
