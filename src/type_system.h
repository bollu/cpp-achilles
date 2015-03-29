#pragma once
#include <memory>
#include <ostream>
#include <vector>
#include <map>
#include <assert.h>

#include "file_handling.h"

class IAST;
enum class TSVariant;
union TSTypeData;
struct TSType;
struct TSScope;

enum class TSVariant {
    AbstractInt,
    SizedInt,
    AbstractFloat,
    SizedFloat,
    TypeVariable,
    Void,
    Uninitialized,
    String,
};


struct TSTypevar {
    //uuid for a type variable
    int32_t uuid;
    //total uuid's ever generated
    static int32_t static_uuid_count;
    //position where this was defined
    PositionRange definition_pos;

    TSTypevar(PositionRange definition_pos) : definition_pos(definition_pos) {
        this->uuid = static_uuid_count;
        static_uuid_count++;
    }

    private:
    TSTypevar(const TSTypevar &other) = delete;
    TSTypevar& operator = (const TSTypevar &other) = delete;
};

union TSTypeData {
    int size;
    //not owning
    TSTypevar* typevar;


    TSTypeData() : size(-42) {};
    TSTypeData(TSTypevar *typevar) : typevar(typevar) {};
    TSTypeData(int size) : size(size) {};

    ~TSTypeData() {
        if(typevar) {
            delete(typevar);
            typevar = (TSTypevar*) 0xDEADBEEF;
        }
    }

    private:
    TSTypeData(const TSTypeData &other) = delete;
    TSTypeData& operator = (const TSTypeData &other) = delete;
};

struct TSType {
    TSVariant variant;
    TSTypeData data;
    //TSScope *scope;

    TSType(TSVariant variant) : variant(variant){};
    TSType(TSVariant variant, int size) : variant(variant), data(size) {};
    TSType(TSTypevar *typevar) : variant(TSVariant::TypeVariable), data(typevar) {};

    private:
    TSType(const TSType &other) = delete;
    TSType& operator = (const TSType &other) = delete;
};

std::ostream& operator <<(std::ostream &out, const TSType &type);
struct TSScope {
    private:
        std::map<std::string, TSType*> sym_table;
        
    public:
        TSScope *parent;

        TSScope(TSScope *parent) : parent(parent) {}

        ~TSScope() {
            for (auto it : sym_table) {
                delete(it.second);
            }
        }

        bool has_type(const std::string &name);
        TSType& get_variable_type(const std::string &name);
        //void replace_variable_type(const std::string &name, TSType *new_type);
        void add_variable(const std::string &name, TSType *type);
};

void type_system_type_check(std::shared_ptr<IAST> &program);
