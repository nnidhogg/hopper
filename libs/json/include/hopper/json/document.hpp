#ifndef HOPPER_LIBS_JSON_INCLUDE_HOPPER_JSON_DOCUMENT_HPP
#define HOPPER_LIBS_JSON_INCLUDE_HOPPER_JSON_DOCUMENT_HPP

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

#include "hopper/json/tokens.hpp"

namespace hopper::json
{
/**
 * @brief One token of a document's stream: its kind and the byte range it covers.
 *
 * An offset rather than a pointer or a view because an edit shifts every token after it: the tail is renumbered by
 * arithmetic and never re-read, which is the whole saving the document exists to make.
 */
struct Piece
{
    /**
     * @brief The kind the lexer matched.
     */
    Token_kind kind;

    /**
     * @brief The token's first byte, as an offset into the text.
     */
    std::size_t offset;

    /**
     * @brief How many bytes the token covers.
     */
    std::size_t length;

    /**
     * @brief Two pieces are equal when kind and range are.
     * @return True when equal.
     */
    [[nodiscard]] bool operator==(const Piece&) const = default;
};

/**
 * @brief What an edit did to a document's stream.
 */
struct Relex
{
    /**
     * @brief Bytes from the start the scan resumed at to where it rejoined the old stream.
     *
     * Reported rather than inferred because it is the quantity the certificates are supposed to reduce, and a caller
     * measuring them needs the number the run actually produced, not the one the theory predicts.
     */
    std::size_t rescanned;

    /**
     * @brief True when the document had to relex from its first byte.
     */
    bool whole;
};

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
     * @brief Constructs the document over a text, tokenizing it whole.
     * @param text The JSON text; a text that does not tokenize completely is kept with the tokens up to the failure.
     */
    explicit Document(std::string text);

    /**
     * @brief The text as it stands.
     * @return The text.
     */
    [[nodiscard]] const std::string& text() const noexcept;

    /**
     * @brief The token stream as it stands, whitespace included, in offset order.
     * @return The tokens.
     */
    [[nodiscard]] const std::vector<Piece>& tokens() const noexcept;

    /**
     * @brief Whether the whole text tokenizes.
     * @return True when the stream covers every byte.
     */
    [[nodiscard]] bool complete() const noexcept;

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
     * @brief Relexes the whole text and records whether it tokenizes.
     * @return The relex report, whole set.
     */
    Relex relex_all();

    /**
     * @brief The text as it stands, owned because an edit rewrites it in place.
     */
    std::string text_;

    /**
     * @brief The maximal-munch segmentation of the text, whitespace included, in offset order.
     *
     * Offsets rather than views into text_, so an edit that reallocates the string leaves the stream valid and the
     * untouched tail is renumbered rather than rescanned.
     */
    std::vector<Piece> tokens_;

    /**
     * @brief Whether the stream covers every byte of the text.
     *
     * An incomplete stream stops the incremental path outright: there is no boundary past an edit to rejoin at when
     * the scan that would have produced one never got there.
     */
    bool complete_;
};

} // namespace hopper::json

#endif // HOPPER_LIBS_JSON_INCLUDE_HOPPER_JSON_DOCUMENT_HPP
