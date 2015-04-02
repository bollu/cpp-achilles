#pragma once
#include <memory>
#include <ostream>
#include <vector>
#include <map>
#include <assert.h>

#include "file_handling.h"

class IAST;
enum class TSVariant;
struct  TSTypeData;
struct TSType;
struct TSScope;

typedef long long TypevarUUID;

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

    TypevarUUID substitution_uuid;

    TSTypevar(PositionRange definition_pos) : definition_pos(definition_pos), substitution_uuid(-42) {
        this->uuid = static_uuid_count;
        static_uuid_count++;
    }

    private:
    //TSTypevar(const TSTypevar &other) = delete;
    //TSTypevar& operator = (const TSTypevar &other) = delete;
};

struct TSTypeData {
    int size;
    //not owning
    TSTypevar* typevar;


    TSTypeData() : size(-42), typevar(nullptr) {};
    TSTypeData(TSTypevar *typevar) : typevar(typevar), size(-42) {};
    TSTypeData(int size) : size(size), typevar(nullptr) {};
};

struct TSType {
    TSVariant variant;
    TSTypeData data;
    //TSScope *scope;

    bool operator == (const TSType &other) {
        if(this->variant != other.variant) {
            return false;
        }

        if(this->variant == TSVariant::SizedInt || this->variant == TSVariant::SizedFloat) {
            return this->data.size == other.data.size;
        }

        if(this->variant == TSVariant::TypeVariable) {
            return this->data.typevar->uuid == other.data.typevar->uuid;
        }
        return true;
    };

    bool operator != (const TSType &other) {
        return !(*this == other);
    };

    public:
    static TSType AbstractInt() {
        return TSType(TSVariant::AbstractInt);
    }
    static TSType AbstractFloat() {
        return TSType(TSVariant::AbstractFloat);
    }
    static TSType Void() {
        return TSType(TSVariant::Void);
    }
    static TSType String() {
        return TSType(TSVariant::String);
    }
    static TSType SizedInt(int size) {
        return TSType(TSVariant::SizedInt, size);
    }
    static TSType SizedFloat(int size) {
        return TSType(TSVariant::SizedFloat, size);
    }
    static TSType Typevar(const PositionRange &definition_pos) {
        TSTypevar *typevar = new TSTypevar(definition_pos);
        return TSType(typevar); 
    }
    private:
    TSType(TSVariant variant) : variant(variant){};
    TSType(TSVariant variant, int size) : variant(variant), data(size) {};
    TSType(TSTypevar *typevar) : variant(TSVariant::TypeVariable), data(typevar) {};

};

std::ostream& operator <<(std::ostream &out, const TSType &type);



struct TSScope {
    private:
        // the TSType vector _has_ to be static since the UUID's
        // must be sequential.
        // otheriwse, scope 1 may have 1 element in it's types
        // and there could be a type that belongs to (Scope 2) with of (UUID 100)
        // so when you index types on (Scope 1), it fails.
        // if it is static, then the (UUID 100) type will go and
        // index at the static vector
        static std::vector<TSType> types;
        //maps names to locations in the vector
        std::map<std::string, TypevarUUID> name_to_index_map;

    public:
        TSScope *parent;

        TSScope(TSScope *parent) : parent(parent) {}
        TypevarUUID add_symbol(std::string name, TSType type);
        bool has_type(const std::string &name);
        TSType get_symbol_type(const std::string &name);
        TSType get_symbol_type(const TypevarUUID &id);
        void replace_type(const TypevarUUID &id, TSType new_type);

        TypevarUUID get_symbol_uuid(const std::string &name);

};


void type_system_type_check(std::shared_ptr<IAST> &program);
