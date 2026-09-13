#ifndef HOPPER_LIBS_CLIKE_INCLUDE_HOPPER_CLIKE_AST_UNIT_HPP
#define HOPPER_LIBS_CLIKE_INCLUDE_HOPPER_CLIKE_AST_UNIT_HPP

#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <variant>
#include <vector>

#include "hopper/clike/ast/decl.hpp"
#include "hopper/clike/ast/stmt.hpp"
#include "hopper/parse/source_span.hpp"

namespace hopper::clike::ast
{
/**
 * @brief One function parameter, e.g. `const char* name` in a parameter list.
 *
 * The name is empty for an unnamed parameter. A default value is the `= expression` form; requiring defaulted
 * parameters to trail the others is left to a later semantic pass, like other non-syntactic rules.
 */
struct Parameter
{
    /**
     * @brief The parameter's qualified type.
     */
    Type type;

    /**
     * @brief The pointer depth: one star per level.
     */
    std::size_t pointers;

    /**
     * @brief Whether the parameter is a reference.
     */
    bool reference;

    /**
     * @brief The parameter's name, empty when unnamed.
     */
    std::string name;

    /**
     * @brief The default argument, when there is one.
     */
    std::optional<Expr> default_value;
};

/**
 * @brief A function definition or prototype.
 *
 * The return type carries the same pointer and reference shape as a declarator. `body` is null for a prototype
 * (`int f(int);`) and holds the compound statement of a definition.
 */
struct Function
{
    /**
     * @brief The qualified return type.
     */
    Type return_type;

    /**
     * @brief The return type's pointer depth.
     */
    std::size_t pointers;

    /**
     * @brief Whether the return type is a reference.
     */
    bool reference;

    /**
     * @brief The function's name.
     */
    std::string name;

    /**
     * @brief The parameters, in source order.
     */
    std::vector<Parameter> parameters;

    /**
     * @brief The body, or null for a prototype.
     */
    std::unique_ptr<Stmt> body;
};

/**
 * @brief A whole translation unit: functions and declarations in source order.
 */
struct Translation_unit
{
    /**
     * @brief One item with the source range it was parsed from.
     */
    struct Item
    {
        /**
         * @brief The kinds of node an item can be.
         */
        using Node_t = std::variant<Declaration, Function>;

        /**
         * @brief The node this item holds.
         */
        /**
         * @brief The item itself.
         */
        Node_t node;

        /**
         * @brief The source range this item was parsed from.
         */
        /**
         * @brief The source range the item was parsed from.
         */
        parse::Source_span span{};
    };

    /**
     * @brief The items in source order.
     */
    /**
     * @brief The items, in source order.
     */
    std::vector<Item> items;
};

} // namespace hopper::clike::ast

#endif // HOPPER_LIBS_CLIKE_INCLUDE_HOPPER_CLIKE_AST_UNIT_HPP
