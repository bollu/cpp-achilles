#include "type_system.h"
#include "ast.h"
#include <assert.h>
int32_t TypeSystemType::static_uuid_count = 0;

bool TypeSystemType::operator == (const TypeSystemType &other) {
    assert(other.variant != Variant::Uninitialized && "using uninitialized type in comparison");
    assert(this->variant != Variant::Uninitialized && "using uninitialized type in comparison");

    //we have different variants. no way are these equal
    if(this->variant != other.variant) {
        return false;
    }

    switch(this->variant) {
        case Variant::Uninitialized:
            assert(false && "using unitialized type variable");
            return false;
            break;

        case Variant::TypeVariable:
            return other.uuid == this->uuid;
            break;

        case Variant::Bool:
        case Variant::String:
            break;

        case Variant::Int:
        case Variant::Float:
            return this->size == other.size;
    };

    assert(false && "control flow should not reach here");
    return false;
};


std::ostream& operator <<(std::ostream &out, const TypeSystemType& type) {
    switch(type.variant) {
        case TypeSystemType::Variant::TypeVariable:
            out<<"var-"<<type.uuid;
            break;

        case TypeSystemType::Variant::Uninitialized:
            out<<"<undef>";
            break;

        case TypeSystemType::Variant::Int:
            out<<"i"<<type.size;
            break;

        case TypeSystemType::Variant::Float:
            out<<"f"<<type.size;
            break;

        case TypeSystemType::Variant::Bool:
            out<<"bool"<<type.size;
            break;

        case TypeSystemType::Variant::String:
            out<<"string"<<type.size;
            break;
    };

    return out;
};

class ASTTypeVariableGenerator : public IASTVisitor {

};


void type_system_type_check(std::vector<std::shared_ptr<IAST>> &program) {
    for (auto line : program) {

    }
};
