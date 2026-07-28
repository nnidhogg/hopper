#ifndef HOPPER_LIBS_IDL_INCLUDE_HOPPER_IDL_AST_TYPE_HPP
#define HOPPER_LIBS_IDL_INCLUDE_HOPPER_IDL_AST_TYPE_HPP

namespace hopper::idl::ast
{
/**
 * @brief Primitive IDL types supported by the minimal parser.
 */
enum class Primitive_type
{
    Boolean,
    Int32,
    String,
};

/**
 * @brief Represents a type in the IDL.
 */
struct Type
{
    Primitive_type primitive;
};

} // namespace hopper::idl::ast

#endif // HOPPER_LIBS_IDL_INCLUDE_HOPPER_IDL_AST_TYPE_HPP
