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
class Value;

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
     * @brief Two numbers are equal when spelled the same.
     * @return True when the texts match.
     */
    [[nodiscard]] bool operator==(const Number&) const = default;

    /**
     * @brief The nearest double to the spelled value.
     * @return The converted value; a magnitude past the double range gives an infinity, one under it a zero.
     */
    [[nodiscard]] double to_double() const;

    /**
     * @brief The number as spelled, already known to match the RFC grammar.
     */
    std::string text;
};

/**
 * @brief A JSON array: its elements in document order.
 */
struct Array
{
    /**
     * @brief Two arrays are equal when their elements are, in order.
     * @return True when equal.
     */
    [[nodiscard]] bool operator==(const Array&) const;

    /**
     * @brief The elements, in the order the document lists them.
     */
    std::vector<Value> elements;
};

/**
 * @brief A JSON object: its members in document order, duplicates kept.
 *
 * RFC 8259 asks for unique names without requiring them, so the tree keeps what the document says and lets the caller
 * decide; the lookups answer as most processors do, with the last member of that name.
 */
struct Object
{
    /**
     * @brief Two objects are equal when their member lists are, in order.
     * @return True when equal.
     */
    [[nodiscard]] bool operator==(const Object&) const;

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
     * @brief The members, in the order the document lists them, every duplicate included.
     */
    std::vector<Member> members;
};

/**
 * @brief A parsed JSON value with the source range it was parsed from.
 *
 * The node is one of the six RFC 8259 kinds; strings are held as UTF-8 with every escape resolved. The span covers the
 * value's own text, from its first byte to one past its last, so an array's span runs from its opening bracket through
 * its closing one and a string's from quote to quote.
 */
class Value
{
public:
    /**
     * @brief The sum of the six kinds.
     */
    using Node_t = std::variant<Null, bool, Number, std::string, Array, Object>;

    /**
     * @brief Constructs a value from its node and span.
     * @param node The value itself.
     * @param span The source range the value was parsed from.
     */
    Value(Node_t node, const parse::Source_span& span);

    /**
     * @brief Copies a value and the whole tree under it.
     *
     * The copy recurses through the tree, so its depth is bounded by the call stack where the parser's is not.
     * @param other The value copied.
     */
    Value(const Value& other) = default;

    /**
     * @brief Moves a value, leaving the source valid but unspecified.
     * @param other The value moved from.
     */
    Value(Value&& other) noexcept = default;

    /**
     * @brief Replaces this value with a copy of another and the whole tree under it.
     *
     * The copy recurses through the tree, so its depth is bounded by the call stack where the parser's is not.
     * @param other The value copied.
     * @return This value.
     */
    Value& operator=(const Value& other) = default;

    /**
     * @brief Replaces this value with another, leaving the source valid but unspecified.
     * @param other The value moved from.
     * @return This value.
     */
    Value& operator=(Value&& other) noexcept = default;

    /**
     * @brief Destroys the value and everything under it without recursing.
     *
     * A tree is as deep as its document nested it, and the parser builds one without touching the call stack, so the
     * destructor does the same: it moves the children out onto a worklist and destroys them one at a time from it.
     */
    ~Value();

    /**
     * @brief Two values are equal when their nodes are; spans do not take part.
     *
     * The comparison recurses through the tree, so its depth is bounded by the call stack where the parser's is not.
     * @param other The value compared with.
     * @return True when the nodes are equal.
     */
    [[nodiscard]] bool operator==(const Value& other) const;

    /**
     * @brief The value itself.
     * @return The node, one of the six kinds.
     */
    [[nodiscard]] const Node_t& node() const noexcept;

    /**
     * @brief The source range the value was parsed from.
     * @return The span, from the value's first byte to one past its last.
     */
    [[nodiscard]] const parse::Source_span& span() const noexcept;

    /**
     * @brief Whether the value is null.
     * @return True for null.
     */
    [[nodiscard]] bool is_null() const noexcept;

    /**
     * @brief The value as a boolean.
     * @return The boolean.
     * @throws std::bad_variant_access If the value is not a boolean.
     */
    [[nodiscard]] bool as_bool() const;

    /**
     * @brief The value as a number.
     * @return The number.
     * @throws std::bad_variant_access If the value is not a number.
     */
    [[nodiscard]] const Number& as_number() const;

    /**
     * @brief The value as a string, escapes resolved, UTF-8.
     * @return The string.
     * @throws std::bad_variant_access If the value is not a string.
     */
    [[nodiscard]] const std::string& as_string() const;

    /**
     * @brief The value as an array.
     * @return The array.
     * @throws std::bad_variant_access If the value is not an array.
     */
    [[nodiscard]] const Array& as_array() const;

    /**
     * @brief The value as an object.
     * @return The object.
     * @throws std::bad_variant_access If the value is not an object.
     */
    [[nodiscard]] const Object& as_object() const;

private:
    /**
     * @brief Moves the nodes of a node's children out and leaves the node childless.
     *
     * The destructor's one step: each child's node is moved out of its value, so that value is destroyed with nothing
     * under it, and the moved nodes are returned for the worklist.
     * @param node The node whose children are detached.
     * @return The children's nodes, in document order.
     */
    [[nodiscard]] static std::vector<Node_t> detach_children(Node_t& node);

    /**
     * @brief The value itself.
     */
    Node_t node_;

    /**
     * @brief The source range the value was parsed from.
     */
    parse::Source_span span_;
};

/**
 * @brief One member of an object: its name and its value.
 */
struct Member
{
    /**
     * @brief Two members are equal when name and value are.
     * @return True when equal.
     */
    [[nodiscard]] bool operator==(const Member&) const = default;

    /**
     * @brief The member name, escapes resolved, UTF-8.
     */
    std::string name;

    /**
     * @brief The member's value.
     */
    Value value;
};

} // namespace hopper::json

#endif // HOPPER_LIBS_JSON_INCLUDE_HOPPER_JSON_VALUE_HPP
