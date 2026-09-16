#include "hopper/json/value.hpp"

#include <algorithm>
#include <charconv>
#include <cstddef>
#include <iterator>
#include <limits>
#include <optional>
#include <ranges>
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
 * @brief Whether a spelling from_chars refused lies past the largest double rather than under the smallest.
 *
 * The spelling matches the RFC grammar, so its parts sit at fixed separators: the sign, the point, the exponent
 * marker. The side follows from the sign of the decimal exponent, the count of integer digits, or minus the count of
 * leading fraction zeros when the integer part is zero, plus the written exponent. The written exponent is clamped
 * to a billion either way, so an exponent of any length is read without overflow and still names its side.
 * @param text The spelling, already known to match the RFC grammar and refused as out of range.
 * @return True when the magnitude is at least ten to some positive power, which is the overflow side.
 */
bool overflows(const std::string_view text)
{
    constexpr long long bound{1'000'000'000};

    const auto unsigned_text{text.substr(text.front() == '-' ? 1 : 0)};
    const auto marker{unsigned_text.find_first_of("eE")};
    const auto mantissa{unsigned_text.substr(0, marker)};
    const auto point{mantissa.find('.')};
    const auto integer{mantissa.substr(0, point)};
    const auto fraction{point == std::string_view::npos ? std::string_view{} : mantissa.substr(point + 1)};

    const auto leading_zeros{std::min(fraction.find_first_not_of('0'), fraction.size())};

    long long exponent{
            integer == "0" ? -static_cast<long long>(leading_zeros) : static_cast<long long>(integer.size())};

    if (marker != std::string_view::npos)
    {
        const auto written{unsigned_text.substr(marker + 1 + (unsigned_text[marker + 1] == '+' ? 1 : 0))};

        long long value{0};

        const auto [end, error]{std::from_chars(written.data(), written.data() + written.size(), value)};

        exponent += error == std::errc::result_out_of_range ? (written.front() == '-' ? -bound : bound) :
                                                              std::clamp(value, -bound, bound);
    }

    return exponent > 0;
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
    // subnormal, and leaves the value untouched, so the side is read off the spelling.
    if (error == std::errc::result_out_of_range)
    {
        const double magnitude{overflows(text) ? std::numeric_limits<double>::infinity() : 0.0};

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
    // The last member of a name answers, so the search runs from the back.
    const auto reversed{std::views::reverse(members)};
    const auto found{std::ranges::find(reversed, name, &Member::name)};

    if (found == reversed.end())
    {
        return std::nullopt;
    }

    return static_cast<std::size_t>(std::distance(members.begin(), found.base())) - 1;
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
