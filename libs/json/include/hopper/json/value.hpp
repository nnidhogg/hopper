#ifndef HOPPER_LIBS_JSON_INCLUDE_HOPPER_JSON_VALUE_HPP
#define HOPPER_LIBS_JSON_INCLUDE_HOPPER_JSON_VALUE_HPP

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

#include "hopper/parse/source_span.hpp"

namespace hopper::json
{
struct Member;
struct Value;

/**
 * @brief The JSON null.
 */
struct Null
{
    /**
     * @brief Two nulls are equal.
     * @return True.
     */
    [[nodiscard]] constexpr bool operator==(const Null&) const noexcept { return true; }
};

/**
 * @brief A JSON number, kept as the text the document spelled it with.
 *
 * RFC 8259 sets no precision, so the text is the value; a caller who wants a double asks for one and accepts that
 * conversion's rounding.
 */
struct Number
{
    /**
     * @brief The number as spelled, already known to match the RFC grammar.
     */
    std::string text;

    /**
     * @brief The nearest double to the spelled value.
     * @return The converted value; a magnitude past the double range gives an infinity, one under it a zero.
     */
    [[nodiscard]] double to_double() const;

    /**
     * @brief Two numbers are equal when spelled the same.
     * @return True when the texts match.
     */
    [[nodiscard]] bool operator==(const Number&) const = default;
};

/**
 * @brief A JSON array: its elements in document order.
 */
struct Array
{
    /**
     * @brief The elements, in the order the document lists them.
     */
    std::vector<Value> elements;

    /**
     * @brief Two arrays are equal when their elements are, in order.
     * @return True when equal.
     */
    [[nodiscard]] bool operator==(const Array&) const;
};

/**
 * @brief A JSON object: its members in document order, duplicates kept.
 *
 * RFC 8259 asks for unique names without requiring them, so the tree keeps what the document says and lets the
 * caller decide; the lookups answer as most processors do, with the last member of that name.
 */
struct Object
{
    /**
     * @brief The members, in the order the document lists them, every duplicate included.
     */
    std::vector<Member> members;

    /**
     * @brief Finds the last member carrying a name.
     * @param name The member name, as characters, escapes already resolved.
     * @return The member's index in members, or std::nullopt when no member carries the name.
     */
    [[nodiscard]] std::optional<std::size_t> find(std::string_view name) const noexcept;

    /**
     * @brief The value of the last member carrying a name.
     * @param name The member name, as characters, escapes already resolved.
     * @return The member's value.
     * @throws std::out_of_range If no member carries the name.
     */
    [[nodiscard]] const Value& at(std::string_view name) const;

    /**
     * @brief Two objects are equal when their member lists are, in order.
     * @return True when equal.
     */
    [[nodiscard]] bool operator==(const Object&) const;
};

/**
 * @brief A parsed JSON value with the source range it was parsed from.
 *
 * The node is one of the six RFC 8259 kinds; strings are held as UTF-8 with every escape resolved. The span covers
 * the value's own text, from its first byte to one past its last, so an array's span runs from its opening bracket
 * through its closing one and a string's from quote to quote.
 */
struct Value
{
    /**
     * @brief The sum of the six kinds.
     */
    using Node_t = std::variant<Null, bool, Number, std::string, Array, Object>;

    /**
     * @brief The value itself.
     */
    Node_t node;

    /**
     * @brief The source range the value was parsed from.
     */
    parse::Source_span span;

    /**
     * @brief Constructs a value from its node and span.
     * @param node The value itself.
     * @param span The source range the value was parsed from.
     */
    Value(Node_t node, parse::Source_span span);

    Value(const Value&) = default;
    Value(Value&&) noexcept = default;
    Value& operator=(const Value&) = default;
    Value& operator=(Value&&) noexcept = default;

    /**
     * @brief Destroys the value and everything under it without recursing.
     *
     * A tree is as deep as its document nested it, and the parser builds one without touching the call stack, so the
     * destructor does the same: it moves the children out onto a worklist and destroys them level by level.
     */
    ~Value();

    /**
     * @brief Whether the value is null.
     * @return True for null.
     */
    [[nodiscard]] bool is_null() const noexcept { return std::holds_alternative<Null>(node); }

    /**
     * @brief The value as a boolean.
     * @return The boolean.
     * @throws std::bad_variant_access If the value is not a boolean.
     */
    [[nodiscard]] bool as_bool() const { return std::get<bool>(node); }

    /**
     * @brief The value as a number.
     * @return The number.
     * @throws std::bad_variant_access If the value is not a number.
     */
    [[nodiscard]] const Number& as_number() const { return std::get<Number>(node); }

    /**
     * @brief The value as a string, escapes resolved, UTF-8.
     * @return The string.
     * @throws std::bad_variant_access If the value is not a string.
     */
    [[nodiscard]] const std::string& as_string() const { return std::get<std::string>(node); }

    /**
     * @brief The value as an array.
     * @return The array.
     * @throws std::bad_variant_access If the value is not an array.
     */
    [[nodiscard]] const Array& as_array() const { return std::get<Array>(node); }

    /**
     * @brief The value as an object.
     * @return The object.
     * @throws std::bad_variant_access If the value is not an object.
     */
    [[nodiscard]] const Object& as_object() const { return std::get<Object>(node); }

    /**
     * @brief Two values are equal when their nodes are; spans do not take part.
     *
     * The comparison recurses through the tree, so its depth is bounded by the call stack where the parser's is not.
     * @param other The value compared with.
     * @return True when the nodes are equal.
     */
    [[nodiscard]] bool operator==(const Value& other) const { return node == other.node; }
};

/**
 * @brief One member of an object: its name and its value.
 */
struct Member
{
    /**
     * @brief The member name, escapes resolved, UTF-8.
     */
    std::string name;

    /**
     * @brief The member's value.
     */
    Value value;

    /**
     * @brief Two members are equal when name and value are.
     * @return True when equal.
     */
    [[nodiscard]] bool operator==(const Member&) const = default;
};

} // namespace hopper::json

#endif // HOPPER_LIBS_JSON_INCLUDE_HOPPER_JSON_VALUE_HPP
