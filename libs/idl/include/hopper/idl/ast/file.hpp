#ifndef HOPPER_LIBS_IDL_INCLUDE_HOPPER_IDL_AST_FILE_HPP
#define HOPPER_LIBS_IDL_INCLUDE_HOPPER_IDL_AST_FILE_HPP

#include <vector>

#include "hopper/idl/ast/struct.hpp"

namespace hopper::idl::ast
{
/**
 * @brief Top-level compilation unit.
 */
struct File
{
    std::vector<Struct> structs;
};

} // namespace hopper::idl::ast

#endif // HOPPER_LIBS_IDL_INCLUDE_HOPPER_IDL_AST_FILE_HPP
