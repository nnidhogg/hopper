#include "hopper/json/document.hpp"

#include <algorithm>
#include <cstddef>
#include <munch/core/lexer.hpp>
#include <optional>
#include <stdexcept>
#include <string_view>
#include <utility>
#include <vector>

namespace hopper::json
{
namespace
{
/**
 * @brief The compiled JSON lexer, built once and shared by every document.
 *
 * Compiling the token set walks the whole regex-to-DFA pipeline, which costs far more than tokenizing a small
 * document; the tables are immutable once built and the scan carries no state across calls, so one instance serves
 * every document on every thread.
 * @return The lexer.
 */
const munch::core::Lexer& shared_lexer()
{
    static const munch::core::Lexer instance{lexer()};

    return instance;
}

/**
 * @brief How far before an edit the search for a certified start looks before giving up and relexing the whole text.
 *
 * A certified start is common in JSON, every structural byte is one, so the search rarely needs more than a token or
 * two; the budget only bounds the cost on a text that certifies nothing for a long stretch, such as one long string.
 */
constexpr std::size_t search_budget{4096};

/**
 * @brief The reach the anchor search starts at, before widening.
 *
 * Small enough that the common case, a structural byte within a token or two of the edit, costs one short walk, and
 * the widening below is what keeps a rarer case from paying the whole budget up front.
 */
constexpr std::size_t initial_reach{64};

} // namespace

Document::Document(std::string text) : text_{std::move(text)}, tokens_{}, complete_{false}
{
    relex_all();
}

Document::Relex Document::edit(const std::size_t offset, const std::size_t removed, const std::string_view inserted)
{
    if (offset > text_.size() || removed > text_.size() - offset)
    {
        throw std::out_of_range{"Document::edit: the range lies outside the text"};
    }

    // The anchor is decided against the old text, before the replacement, because that is the text whose tokens are
    // being kept: a certificate found in the new bytes would say nothing about the stream already in hand.
    const auto anchor{find_anchor(offset)};

    const auto old_edit_end{offset + removed};

    text_.replace(offset, removed, inserted);

    // An incomplete stream has no boundary past the edit to rejoin at, since the old scan never reached one, and no
    // anchor means nothing before the edit is proved to survive it. Either way only a whole relex is defensible.
    if (!complete_ || !anchor)
    {
        return relex_all();
    }

    return relex_tail(*anchor, offset + inserted.size(), old_edit_end);
}

bool Document::scan(const std::size_t from, std::vector<Token>& out) const
{
    const auto& lexer{shared_lexer()};

    const std::string_view view{text_};

    std::size_t at{from};

    while (at < view.size())
    {
        const auto match{lexer.tokenize<Token_kind>(view.substr(at))};

        // A zero-width match would leave the position where it is, so it ends the scan exactly as no match does.
        if (!match.token || match.length == 0)
        {
            return false;
        }

        out.push_back({.kind = *match.token, .offset = at, .length = match.length});

        at += match.length;
    }

    return true;
}

std::optional<std::size_t> Document::find_anchor(const std::size_t offset) const
{
    const auto& lexer{shared_lexer()};

    const std::string_view view{text_};

    std::optional<std::size_t> anchor;

    for (auto reach{initial_reach}; !anchor && reach <= search_budget; reach *= 4)
    {
        for (auto from{offset > reach ? offset - reach : 0}; from <= offset;)
        {
            const auto found{lexer.next_certified_evidence(view, from)};

            // The walk only runs forward, so once it reports a start past the edit there is nothing nearer to find.
            if (!found || found->start > offset)
            {
                break;
            }

            if (found->evidence_end <= offset)
            {
                anchor = found->start;
            }

            from = found->start + 1;
        }

        // A reach that already covers the whole prefix has nothing left to widen into.
        if (reach >= offset)
        {
            break;
        }
    }

    return anchor;
}

Document::Relex Document::relex_tail(
        const std::size_t anchor, const std::size_t edit_end, const std::size_t old_edit_end)
{
    const auto& lexer{shared_lexer()};

    const std::string_view view{text_};

    // Positions at or after edit_end name the same bytes they did before the edit, only moved; exchanging the two
    // ends converts between the texts without a signed delta, and nothing before edit_end is ever converted.
    const auto in_old_text{[=](const std::size_t at) { return at - edit_end + old_edit_end; }};

    // The old tokens before the anchor stand as they were; the first old token starting at or after the replaced
    // range is the earliest that could still be reached, so the search for a shared boundary starts there.
    const auto keep_before{std::ranges::lower_bound(tokens_, anchor, {}, &Token::offset)};

    auto rejoin{std::ranges::lower_bound(tokens_, old_edit_end, {}, &Token::offset)};

    std::vector<Token> fresh(tokens_.begin(), keep_before);

    std::size_t at{anchor};

    while (at < view.size())
    {
        // The cursor only moves forward, so each step resumes the search where the last one stopped rather than
        // scanning the old stream again; the shared boundary, if there is one, is at or after where it stands.
        const auto shared{at >= edit_end ? in_old_text(at) : old_edit_end};

        rejoin = std::ranges::lower_bound(rejoin, tokens_.end(), shared, {}, &Token::offset);

        if (at >= edit_end && rejoin != tokens_.end() && rejoin->offset == shared)
        {
            take_old_tail(fresh, rejoin, edit_end, old_edit_end);

            tokens_ = std::move(fresh);

            return {.rescanned = at - anchor, .whole = false};
        }

        const auto match{lexer.tokenize<Token_kind>(view.substr(at))};

        if (!match.token || match.length == 0)
        {
            return relex_all();
        }

        fresh.push_back({.kind = *match.token, .offset = at, .length = match.length});

        at += match.length;
    }

    // The scan ran to the end without meeting a shared boundary, so the new stream is the whole tail.
    tokens_ = std::move(fresh);

    return {.rescanned = at - anchor, .whole = false};
}

void Document::take_old_tail(
        std::vector<Token>& out, const std::vector<Token>::const_iterator from, const std::size_t edit_end,
        const std::size_t old_edit_end) const
{
    for (auto it{from}; it != tokens_.end(); ++it)
    {
        out.push_back({.kind = it->kind, .offset = it->offset - old_edit_end + edit_end, .length = it->length});
    }
}

Document::Relex Document::relex_all()
{
    tokens_.clear();

    complete_ = scan(0, tokens_);

    return {.rescanned = text_.size(), .whole = true};
}

} // namespace hopper::json
