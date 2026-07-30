#ifndef HOPPER_LIBS_CPP_INCLUDE_HOPPER_CPP_AST_TYPE_HPP
#define HOPPER_LIBS_CPP_INCLUDE_HOPPER_CPP_AST_TYPE_HPP

#include <cstddef>

namespace hopper::cpp::ast
{
/**
 * @brief A fundamental type name.
 */
enum class Type_kind
{
    Bool,
    Char,
    Int,
    Float,
    Double,
    Void,
};

/**
 * @brief A type specifier: a fundamental type with an optional const qualifier.
 *
 * The qualifier's spelling position (`const int` or `int const`) is not preserved; both name the same type.
 */
struct Type
{
    bool is_const;
    Type_kind kind;
};

/**
 * @brief A complete type as a cast target, e.g. `const char**&`: a specifier with its pointer and reference shape.
 *
 * Pointers apply before the reference, exactly as in Declarator.
 */
struct Type_id
{
    Type type;
    std::size_t pointers;
    bool reference;
};

} // namespace hopper::cpp::ast

#endif // HOPPER_LIBS_CPP_INCLUDE_HOPPER_CPP_AST_TYPE_HPP
