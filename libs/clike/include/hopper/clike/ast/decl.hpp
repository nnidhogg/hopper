#ifndef HOPPER_LIBS_CLIKE_INCLUDE_HOPPER_CLIKE_AST_DECL_HPP
#define HOPPER_LIBS_CLIKE_INCLUDE_HOPPER_CLIKE_AST_DECL_HPP

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

#include "hopper/clike/ast/expr.hpp"
#include "hopper/clike/ast/type.hpp"

namespace hopper::clike::ast
{
/**
 * @brief One declared entity, e.g. `**&name = value` within a declaration.
 *
 * Pointers apply before the reference, so `pointers = 1, reference = true` reads `*&`, a reference to pointer; a
 * pointer to reference is not expressible, exactly as in C++. Rejecting an uninitialized reference is left to a
 * later semantic pass, like other non-syntactic rules.
 */
struct Declarator
{
    /**
     * @brief The pointer depth and reference bit before the name.
     */
    Indirection indirection;

    /**
     * @brief The declared name.
     */
    std::string name;

    /**
     * @brief The initializer after `=`, when there is one.
     */
    std::optional<Expr> initializer;
};

/**
 * @brief A declaration statement, e.g. `const int first = 1, second;`.
 *
 * One type specifier shared by one or more declarators, each with its own pointer and reference shape and its own
 * optional initializer.
 */
struct Declaration
{
    /**
     * @brief The type every declarator shares.
     */
    Type type;

    /**
     * @brief The declarators, in source order.
     */
    std::vector<Declarator> declarators;
};

} // namespace hopper::clike::ast

#endif // HOPPER_LIBS_CLIKE_INCLUDE_HOPPER_CLIKE_AST_DECL_HPP
