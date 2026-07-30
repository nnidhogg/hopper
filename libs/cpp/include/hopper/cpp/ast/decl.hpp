#ifndef HOPPER_LIBS_CPP_INCLUDE_HOPPER_CPP_AST_DECL_HPP
#define HOPPER_LIBS_CPP_INCLUDE_HOPPER_CPP_AST_DECL_HPP

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

#include "hopper/cpp/ast/expr.hpp"
#include "hopper/cpp/ast/type.hpp"

namespace hopper::cpp::ast
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
    std::size_t pointers;
    bool reference;
    std::string name;
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
    Type type;
    std::vector<Declarator> declarators;
};

} // namespace hopper::cpp::ast

#endif // HOPPER_LIBS_CPP_INCLUDE_HOPPER_CPP_AST_DECL_HPP
