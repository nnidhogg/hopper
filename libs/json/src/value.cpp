#include "hopper/json/value.hpp"

#include <charconv>
#include <cstddef>
#include <limits>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

namespace hopper::json
{
namespace
{
/**
 * @brief The decimal exponent a spelling carries: the count of integer digits, or minus the count of leading
 *        fraction zeros when the integer part is zero, plus the written exponent.
 *
 * The written exponent is saturated at a billion so that an exponent of any length is read without overflow; the
 * sign of the result is all a caller needs, since it decides which side of the double range a refused spelling
 * lies on.
 * @param text The spelling, already known to match the RFC grammar.
 * @return The decimal exponent, positive for a magnitude of at least ten.
 */
long long decimal_exponent(const std::string_view text)
{
    std::size_t at{text.front() == '-' ? 1U : 0U};

    long long exponent{0};

    if (text[at] == '0')
    {
        ++at;

        if (at < text.size() && text[at] == '.')
        {
            ++at;

            while (at < text.size() && text[at] == '0')
            {
                --exponent;
                ++at;
            }
        }
    }
    else
    {
        while (at < text.size() && text[at] >= '0' && text[at] <= '9')
        {
            ++exponent;
            ++at;
        }
    }

    const auto marker{text.find_first_of("eE")};

    if (marker == std::string_view::npos)
    {
        return exponent;
    }

    auto digit{marker + 1};

    const bool negative{text[digit] == '-'};

    if (text[digit] == '-' || text[digit] == '+')
    {
        ++digit;
    }

    long long written{0};

    for (; digit < text.size(); ++digit)
    {
        written = written > 1'000'000'000 ? written : written * 10 + (text[digit] - '0');
    }

    return exponent + (negative ? -written : written);
}

/**
 * @brief Moves the children of a node onto the worklist and leaves the node childless.
 * @param node The node whose children are detached.
 * @param pending The worklist the children's nodes are moved onto.
 */
void detach_children(Value::Node_t& node, std::vector<Value::Node_t>& pending)
{
    std::visit(
            [&pending]<typename T>(T& alternative) {
                if constexpr (std::is_same_v<T, Array>)
                {
                    for (auto& element : alternative.elements)
                    {
                        pending.push_back(std::move(element.node));
                    }

                    alternative.elements.clear();
                }
                else if constexpr (std::is_same_v<T, Object>)
                {
                    for (auto& member : alternative.members)
                    {
                        pending.push_back(std::move(member.value.node));
                    }

                    alternative.members.clear();
                }
            },
            node);
}

} // namespace

double Number::to_double() const
{
    double value{0.0};

    const auto [end, error]{std::from_chars(text.data(), text.data() + text.size(), value)};

    // from_chars refuses exactly the spellings whose magnitude exceeds the largest double or falls under the smallest
    // subnormal, and leaves the value untouched, so the side is read off the decimal exponent the spelling carries.
    if (error == std::errc::result_out_of_range)
    {
        const double magnitude{decimal_exponent(text) > 0 ? std::numeric_limits<double>::infinity() : 0.0};

        return text.front() == '-' ? -magnitude : magnitude;
    }

    return value;
}

bool Array::operator==(const Array& other) const
{
    return elements == other.elements;
}

std::optional<std::size_t> Object::find(const std::string_view name) const noexcept
{
    for (auto index{members.size()}; index > 0; --index)
    {
        if (members[index - 1].name == name)
        {
            return index - 1;
        }
    }

    return std::nullopt;
}

const Value& Object::at(const std::string_view name) const
{
    const auto index{find(name)};

    if (!index)
    {
        throw std::out_of_range{"Object::at: no member is named " + std::string{name}};
    }

    return members[*index].value;
}

bool Object::operator==(const Object& other) const
{
    return members == other.members;
}

Value::Value(Node_t node, const parse::Source_span span) : node{std::move(node)}, span{span}
{}

Value::~Value()
{
    std::vector<Node_t> pending;

    detach_children(node, pending);

    while (!pending.empty())
    {
        auto current{std::move(pending.back())};

        pending.pop_back();

        detach_children(current, pending);
    }
}

} // namespace hopper::json
