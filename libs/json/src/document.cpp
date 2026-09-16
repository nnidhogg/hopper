#include "hopper/json/document.hpp"

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <munch/core/lexer.hpp>

namespace hopper::json
{
namespace
{
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
 * the widening is what keeps a rarer case from paying the whole budget up front.
 */
constexpr std::size_t initial_reach{64};

/**
 * @brief The factor the reach widens by when a search finds no anchor within it.
 */
constexpr std::size_t widening{4};

/**
 * @brief A scan of the text from an offset to its end.
 */
struct Scan
{
    /**
     * @brief The tokens the scan produced, in offset order.
     */
    std::vector<Piece> tokens;

    /**
     * @brief Whether every byte from the offset on was covered.
     */
    bool complete;
};

/**
 * @brief A rescan that rejoined the old stream, or ran to the end of the text.
 */
struct Rescan
{
    /**
     * @brief The new stream.
     */
    std::vector<Piece> tokens;

    /**
     * @brief Bytes read from the anchor to the shared boundary, or to the end.
     */
    std::size_t rescanned;
};

/**
 * @brief The token the lexer matches at an offset, or nothing where no token begins there.
 *
 * A zero-width match would leave the position where it is, so it is treated as no match.
 * @param text The whole text.
 * @param at The offset the token would begin at.
 * @return The token, or std::nullopt.
 */
std::optional<Piece> piece_at(const std::string_view text, const std::size_t at)
{
    const auto [token, length]{lexer().tokenize<Token_kind>(text.substr(at))};

    if (!token || length == 0)
    {
        return std::nullopt;
    }

    return Piece{.kind = *token, .offset = at, .length = length};
}

/**
 * @brief Tokenizes the text from an offset to its end.
 *
 * The automaton restarts at every token, so the scan carries nothing across a token boundary and a caller may start
 * it at any boundary it can justify. That is what lets an edit resume mid-text at all.
 * @param text The whole text.
 * @param from The offset the scan starts at, which must be a token boundary.
 * @return The tokens in order, and whether every byte from the offset on was covered.
 */
Scan scan(const std::string_view text, const std::size_t from)
{
    Scan result{.tokens = {}, .complete = true};

    for (std::size_t at{from}; at < text.size();)
    {
        const auto piece{piece_at(text, at)};

        if (!piece)
        {
            result.complete = false;

            break;
        }

        result.tokens.push_back(*piece);

        at += piece->length;
    }

    return result;
}

/**
 * @brief The last certified token start before an edit whose evidence the edit leaves untouched.
 *
 * A rescan may only resume where the old and the new scan provably agree on everything before it, and a certificate
 * says exactly that: it is decided over every state the scan could be in when it meets its evidence, from the evidence
 * bytes alone, so while those bytes are unchanged no earlier token can end differently. Hence the requirement that the
 * evidence end at or before the edit. An anchor whose evidence the edit overlaps proves nothing about the edited text,
 * and trusting one would let a token before the anchor change length with nothing to detect it.
 *
 * The last such start is taken rather than the first because the rescan runs forward from the anchor, so the nearest
 * one is the cheapest. The search starts at a short reach and widens because JSON certifies densely, every structural
 * byte being a certificate, so the first reach almost always answers; the widening is what bounds the cost on a text
 * that certifies nothing for a long stretch, such as one long string, and the budget bounds it absolutely. Finding
 * none is not a failure but an admission, and the caller relexes whole.
 * @param text The text before the edit.
 * @param offset The first byte the edit replaces.
 * @return The anchor's offset, or std::nullopt when none lies within the search budget.
 */
std::optional<std::size_t> anchor_before(const std::string_view text, const std::size_t offset)
{
    const auto& scanner{lexer()};

    std::optional<std::size_t> anchor;

    for (auto reach{initial_reach}; !anchor && reach <= search_budget; reach *= widening)
    {
        for (auto from{offset > reach ? offset - reach : 0}; from <= offset;)
        {
            const auto found{scanner.next_certified_evidence(text, from)};

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

/**
 * @brief Appends the old tokens from a shared boundary onward, renumbered into the new text.
 *
 * The tokens are taken rather than rescanned: the boundary they start at begins a token in both streams, so the scan
 * that produced them would produce them again, and only their offsets have moved. Renumbering is what the offsets are
 * for, and it is the whole saving, so this walks the tail without touching the text.
 * @param out The stream being built, appended to.
 * @param from The first old token at or after the shared boundary.
 * @param end The end of the old stream.
 * @param edit_end One past the last inserted byte, in the new text.
 * @param old_edit_end One past the last replaced byte, in the old text.
 */
void append_renumbered(
        std::vector<Piece>& out, std::vector<Piece>::const_iterator from, const std::vector<Piece>::const_iterator end,
        const std::size_t edit_end, const std::size_t old_edit_end)
{
    for (; from != end; ++from)
    {
        out.push_back({.kind = from->kind, .offset = from->offset - old_edit_end + edit_end, .length = from->length});
    }
}

/**
 * @brief Rescans from an anchor and rejoins the old stream at the first boundary past the edit that both have.
 *
 * Because the automaton restarts at every token, a position that begins a token in both streams carries no state
 * between them: from there the two scans read the same bytes from the same start state, so they agree for the rest of
 * the text and the old tokens can be taken as they are. That is what makes a shared boundary a stopping point rather
 * than a coincidence, and why the search for one may only begin past the edit, where the bytes are the old bytes
 * again.
 *
 * Positions past the edit are converted to the old text by exchanging the edit's two ends rather than by adding a
 * signed delta, which keeps every value unsigned and in range, since by construction nothing before edit_end is ever
 * converted. A byte that no longer tokenizes abandons the attempt, because the stream has to cover every byte to be
 * the segmentation the document promises.
 * @param text The text after the edit.
 * @param old_tokens The stream before the edit, in the old text's offsets.
 * @param anchor The certified start the rescan resumes at.
 * @param edit_end One past the last inserted byte, in the new text.
 * @param old_edit_end One past the last replaced byte, in the old text.
 * @return The new stream and the bytes rescanned, or std::nullopt when the text stopped tokenizing.
 */
std::optional<Rescan> rescan(
        const std::string_view text, const std::vector<Piece>& old_tokens, const std::size_t anchor,
        const std::size_t edit_end, const std::size_t old_edit_end)
{
    // The old tokens before the anchor stand as they were; the first old token starting at or after the replaced
    // range is the earliest that could still be reached, so the search for a shared boundary starts there.
    const auto kept{std::ranges::lower_bound(old_tokens, anchor, {}, &Piece::offset)};

    auto rejoin{std::ranges::lower_bound(old_tokens, old_edit_end, {}, &Piece::offset)};

    Rescan result{.tokens = std::vector<Piece>(old_tokens.begin(), kept), .rescanned = 0};

    for (std::size_t at{anchor}; at < text.size(); at += result.tokens.back().length)
    {
        // The cursor only moves forward, so each step resumes the search where the last one stopped rather than
        // scanning the old stream again; the shared boundary, if there is one, is at or after where it stands.
        const auto shared{at >= edit_end ? at - edit_end + old_edit_end : old_edit_end};

        rejoin = std::ranges::lower_bound(rejoin, old_tokens.end(), shared, {}, &Piece::offset);

        if (at >= edit_end && rejoin != old_tokens.end() && rejoin->offset == shared)
        {
            append_renumbered(result.tokens, rejoin, old_tokens.end(), edit_end, old_edit_end);

            result.rescanned = at - anchor;

            return result;
        }

        const auto piece{piece_at(text, at)};

        if (!piece)
        {
            return std::nullopt;
        }

        result.tokens.push_back(*piece);
    }

    // The scan ran to the end without meeting a shared boundary, so the new stream is the whole tail.
    result.rescanned = text.size() - anchor;

    return result;
}

} // namespace

Document::Document(std::string text) : text_{std::move(text)}, tokens_{}, complete_{false}
{
    relex_all();
}

Relex Document::edit(const std::size_t offset, const std::size_t removed, const std::string_view inserted)
{
    if (offset > text_.size() || removed > text_.size() - offset)
    {
        throw std::out_of_range{"Document::edit: the range lies outside the text"};
    }

    // The anchor is decided against the old text, before the replacement, because that is the text whose tokens are
    // being kept: a certificate found in the new bytes would say nothing about the stream already in hand.
    const auto anchor{anchor_before(text_, offset)};

    const auto old_edit_end{offset + removed};

    text_.replace(offset, removed, inserted);

    // An incomplete stream has no boundary past the edit to rejoin at, since the old scan never reached one, and no
    // anchor means nothing before the edit is proved to survive it. Either way only a whole relex is defensible.
    if (!complete_ || !anchor)
    {
        return relex_all();
    }

    auto result{rescan(text_, tokens_, *anchor, offset + inserted.size(), old_edit_end)};

    if (!result)
    {
        return relex_all();
    }

    tokens_ = std::move(result->tokens);

    return {.rescanned = result->rescanned, .whole = false};
}

Relex Document::relex_all()
{
    auto [tokens, complete]{scan(text_, 0)};

    tokens_ = std::move(tokens);
    complete_ = complete;

    return {.rescanned = text_.size(), .whole = true};
}

const std::string& Document::text() const noexcept
{
    return text_;
}

const std::vector<Piece>& Document::tokens() const noexcept
{
    return tokens_;
}

bool Document::complete() const noexcept
{
    return complete_;
}

} // namespace hopper::json
