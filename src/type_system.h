#pragma once
#include <memory>
#include <ostream>
#include <vector>

struct TypeSystemType {
    enum class Variant {
        Int,
        Float,
        Bool,
        String,
        TypeVariable,
        Uninitialized,
    } variant;
    
    //size for int or float
    int8_t size;
    //uuid for a type variable
    int32_t uuid;

    //total uuid's ever generated
    static int32_t static_uuid_count;

    bool operator == (const TypeSystemType &other);

    
    TypeSystemType Int(int8_t size) { 
        TypeSystemType type(Variant::Int);
        type.size = size;

        return type;
    }

    TypeSystemType TypeVariable() {
        TypeSystemType type(Variant::TypeVariable);
        type.uuid = TypeSystemType::static_uuid_count;
        TypeSystemType::static_uuid_count++;
        return type;
    }
    
    TypeSystemType() : variant(Variant::Uninitialized), size(UNDEFINED), uuid(UNDEFINED) {}

    protected:
    static const int UNDEFINED = -42;
    TypeSystemType(Variant variant) : variant(variant), size(UNDEFINED), uuid(UNDEFINED) {};
};

std::ostream& operator <<(std::ostream &out, const TypeSystemType& type);

class IAST;
void type_system_type_check(std::vector<std::shared_ptr<IAST>> &program);
