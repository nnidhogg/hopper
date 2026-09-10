#ifndef HOPPER_LIBS_JSON_INCLUDE_HOPPER_JSON_DOCUMENT_HPP
#define HOPPER_LIBS_JSON_INCLUDE_HOPPER_JSON_DOCUMENT_HPP

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "hopper/json/tokens.hpp"

namespace hopper::json
{
/**
 * @brief A JSON text with its token stream, kept current across edits by relexing only what an edit can reach.
 *
 * The stream is the maximal-munch segmentation of the text, whitespace tokens included, held as offsets so an edit
 * shifts the untouched tail without rereading it. An edit replaces a byte range; the document then rescans from the
 * last certified token start whose evidence lies wholly before the edit and stops at the first boundary after the edit
 * that the old stream also had, from which the two scans agree byte for byte. The tokens before that start stand as
 * they were: a certificate is decided over every state the scan could be in when it meets the evidence, from the
 * evidence bytes alone, so no token beginning before the start can end differently while the bytes through the evidence
 * are unchanged. Where the edited text no longer tokenizes completely the document relexes from its first byte, and
 * does so on every edit through the one that repairs it; where no certified start lies within the search budget before
 * the edit, the same. Every edit reports how many bytes it rescanned, so the saving the certificates buy is measured
 * rather than assumed; tools/probes measures it on a corpus.
 */
class Document
{
public:
    /**
     * @brief One token of the stream: its kind and the byte range it covers.
     */
    struct Token
    {
        /**
         * @brief The kind the lexer matched.
         */
        Token_kind kind;

        /**
         * @brief The token's first byte, as an offset into the text.
         *
         * An offset rather than a pointer or a view because an edit shifts every token after it: the tail is
         * renumbered by arithmetic and never re-read, which is the whole saving this class exists to make.
         */
        std::size_t offset;

        /**
         * @brief How many bytes the token covers.
         */
        std::size_t length;

        /**
         * @brief Two tokens are equal when kind and range are.
         * @return True when equal.
         */
        [[nodiscard]] bool operator==(const Token&) const = default;
    };

    /**
     * @brief What an edit did to the stream.
     */
    struct Relex
    {
        /**
         * @brief Bytes from the start the scan resumed at to where it rejoined the old stream.
         *
         * Reported rather than inferred because it is the quantity the certificates are supposed to reduce, and a
         * caller measuring them needs the number the run actually produced, not the one the theory predicts.
         */
        std::size_t rescanned;

        /**
         * @brief True when the document had to relex from its first byte.
         */
        bool whole;
    };

    /**
     * @brief Constructs the document over a text, tokenizing it whole.
     * @param text The JSON text; a text that does not tokenize completely is kept with the tokens up to the failure.
     */
    explicit Document(std::string text);

    /**
     * @brief The text as it stands.
     * @return The text.
     */
    [[nodiscard]] const std::string& text() const noexcept { return text_; }

    /**
     * @brief The token stream as it stands, whitespace included, in offset order.
     * @return The tokens.
     */
    [[nodiscard]] const std::vector<Token>& tokens() const noexcept { return tokens_; }

    /**
     * @brief Whether the whole text tokenizes.
     * @return True when the stream covers every byte.
     */
    [[nodiscard]] bool complete() const noexcept { return complete_; }

    /**
     * @brief Replaces a byte range and brings the stream current.
     * @param offset The first byte replaced.
     * @param removed How many bytes are replaced; offset plus removed is at most the text's size.
     * @param inserted The bytes put in their place.
     * @return How many bytes the edit rescanned, and whether it had to start over.
     * @throws std::out_of_range If the range lies outside the text.
     */
    Relex edit(std::size_t offset, std::size_t removed, std::string_view inserted);

private:
    /**
     * @brief Tokenizes from an offset to the end and returns whether the scan reached it.
     *
     * The automaton restarts at every token, so the scan carries nothing across a token boundary and a caller may
     * start it at any boundary it can justify. That is what lets an edit resume mid-text at all.
     * @param from The offset the scan starts at, which must be a token boundary.
     * @param out The tokens appended in order.
     * @return True when every byte from the offset on was covered.
     */
    [[nodiscard]] bool scan(std::size_t from, std::vector<Token>& out) const;

    /**
     * @brief The last certified token start before an edit whose evidence the edit leaves untouched.
     *
     * A rescan may only resume where the old and the new scan provably agree on everything before it, and a
     * certificate says exactly that: it is decided over every state the scan could be in when it meets its
     * evidence, from the evidence bytes alone, so while those bytes are unchanged no earlier token can end
     * differently. Hence the requirement that the evidence end at or before the edit. An anchor whose evidence the
     * edit overlaps proves nothing about the edited text, and trusting one would let a token before the anchor
     * change length with nothing to detect it.
     *
     * The last such start is taken rather than the first because the rescan runs forward from the anchor, so the
     * nearest one is the cheapest. The search starts at a short reach and widens because JSON certifies densely,
     * every structural byte being a certificate, so the first reach almost always answers; the widening is what
     * bounds the cost on a text that certifies nothing for a long stretch, such as one long string, and the budget
     * bounds it absolutely. Finding none is not a failure but an admission, and the caller relexes whole.
     * @param offset The first byte the edit replaces.
     * @return The anchor's offset, or std::nullopt when none lies within the search budget.
     */
    [[nodiscard]] std::optional<std::size_t> find_anchor(std::size_t offset) const;

    /**
     * @brief Rescans from an anchor and rejoins the old stream at the first boundary past the edit that both have.
     *
     * Because the automaton restarts at every token, a position that begins a token in both streams carries no
     * state between them: from there the two scans read the same bytes from the same start state, so they agree
     * for the rest of the text and the old tokens can be taken as they are. That is what makes a shared boundary a
     * stopping point rather than a coincidence, and why the search for one may only begin past the edit, where the
     * bytes are the old bytes again.
     *
     * Positions past the edit are converted to the old text by exchanging the edit's two ends rather than by
     * adding a signed delta, which keeps every value unsigned and in range, since by construction nothing before
     * edit_end is ever converted. A byte that no longer tokenizes abandons the attempt, because the stream has to
     * cover every byte to be the segmentation this class promises.
     * @param anchor The certified start the rescan resumes at.
     * @param edit_end One past the last inserted byte, in the new text.
     * @param old_edit_end One past the last replaced byte, in the old text.
     * @return What the edit did: the bytes rescanned, or a whole relex when the text stopped tokenizing.
     */
    Relex relex_tail(std::size_t anchor, std::size_t edit_end, std::size_t old_edit_end);

    /**
     * @brief Appends the old tokens from a shared boundary onward, renumbered into the new text.
     *
     * The tokens are taken rather than rescanned: the boundary they start at begins a token in both streams, so the
     * scan that produced them would produce them again, and only their offsets have moved. Renumbering is what the
     * offsets are for, and it is the whole saving, so this walks the tail without touching the text.
     * @param out The stream being built, appended to.
     * @param from The first old token at or after the shared boundary.
     * @param edit_end One past the last inserted byte, in the new text.
     * @param old_edit_end One past the last replaced byte, in the old text.
     */
    void take_old_tail(
            std::vector<Token>& out, std::vector<Token>::const_iterator from, std::size_t edit_end,
            std::size_t old_edit_end) const;

    /**
     * @brief Relexes the whole text.
     * @return The relex report, whole set.
     */
    Relex relex_all();

    std::string text_;
    std::vector<Token> tokens_;
    bool complete_;
};

} // namespace hopper::json

#endif // HOPPER_LIBS_JSON_INCLUDE_HOPPER_JSON_DOCUMENT_HPP
