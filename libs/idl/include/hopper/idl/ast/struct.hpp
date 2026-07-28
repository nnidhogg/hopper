#ifndef HOPPER_LIBS_IDL_INCLUDE_HOPPER_IDL_AST_STRUCT_HPP
#define HOPPER_LIBS_IDL_INCLUDE_HOPPER_IDL_AST_STRUCT_HPP

#include <string>
#include <vector>

#include "hopper/idl/ast/field.hpp"

namespace hopper::idl::ast
{
/**
 * @brief Represents a struct declaration.
 */
struct Struct
{
    std::string name;
    std::vector<Field> fields;
};

} // namespace hopper::idl::ast

#endif // HOPPER_LIBS_IDL_INCLUDE_HOPPER_IDL_AST_STRUCT_HPP
