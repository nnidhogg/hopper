#ifndef HOPPER_LIBS_CLIKE_SRC_TABLES_HPP
#define HOPPER_LIBS_CLIKE_SRC_TABLES_HPP

#include <algorithm>
#include <optional>
#include <string_view>
#include <utility>

namespace hopper::clike
{
/**
 * @brief Looks a spelling up in a constexpr table of spelling and value pairs.
 *
 * The parser's keyword, type and operator tables are all such pairs, so one lookup serves them, and the value
 * type follows the table.
 * @tparam Table The table's type, a range of std::pair<std::string_view, Value>.
 * @param table The table.
 * @param spelling The spelling looked for.
 * @return The entry's value, or std::nullopt when the spelling is not in the table.
 */
template <typename Table>
[[nodiscard]] auto lookup(const Table& table, const std::string_view spelling) noexcept
        -> std::optional<typename Table::value_type::second_type>
{
    const auto found{std::ranges::find(table, spelling, &Table::value_type::first)};

    return found != table.end() ? std::optional{found->second} : std::nullopt;
}

} // namespace hopper::clike

#endif // HOPPER_LIBS_CLIKE_SRC_TABLES_HPP
