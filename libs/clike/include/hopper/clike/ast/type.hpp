#ifndef HOPPER_LIBS_CLIKE_INCLUDE_HOPPER_CLIKE_AST_TYPE_HPP
#define HOPPER_LIBS_CLIKE_INCLUDE_HOPPER_CLIKE_AST_TYPE_HPP

#include <cstddef>
#include <cstdint>

namespace hopper::clike::ast
{
/**
 * @brief A fundamental type name.
 */
enum class Type_kind : std::uint8_t
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
    /**
     * @brief Whether the type is const-qualified.
     */
    bool is_const;

    /**
     * @brief The fundamental type.
     */
    Type_kind kind;
};

/**
 * @brief The pointer stars and the optional reference that follow a type or open a declarator, read the same way
 *        wherever they occur.
 */
struct Indirection
{
    /**
     * @brief The pointer depth: one star per level.
     */
    std::size_t pointers;

    /**
     * @brief Whether an ampersand follows the stars.
     */
    bool reference;

    /**
     * @brief Two indirections are equal when their depth and reference bit are.
     * @return True when equal.
     */
    [[nodiscard]] bool operator==(const Indirection&) const = default;
};

/**
 * @brief A complete type as a cast target, e.g. `const char**&`: a specifier with its pointer and reference shape.
 *
 * Pointers apply before the reference, exactly as in Declarator.
 */
struct Type_id
{
    /**
     * @brief The qualified fundamental type.
     */
    Type type;

    /**
     * @brief The pointer depth and reference bit after the type.
     */
    Indirection indirection;
};

} // namespace hopper::clike::ast

#endif // HOPPER_LIBS_CLIKE_INCLUDE_HOPPER_CLIKE_AST_TYPE_HPP
