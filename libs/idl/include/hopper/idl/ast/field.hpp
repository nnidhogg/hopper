#ifndef HOPPER_LIBS_IDL_INCLUDE_HOPPER_IDL_AST_FIELD_HPP
#define HOPPER_LIBS_IDL_INCLUDE_HOPPER_IDL_AST_FIELD_HPP

#include <string>

#include "hopper/idl/ast/type.hpp"

namespace hopper::idl::ast
{
/**
 * @brief Represents a field inside a struct.
 */
struct Field
{
    Type type;
    std::string name;
};

} // namespace hopper::idl::ast

#endif // HOPPER_LIBS_IDL_INCLUDE_HOPPER_IDL_AST_FIELD_HPP
