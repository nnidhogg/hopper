#ifndef HOPPER_LIBS_PARSE_INCLUDE_HOPPER_PARSE_TOKEN_READER_HPP
#define HOPPER_LIBS_PARSE_INCLUDE_HOPPER_PARSE_TOKEN_READER_HPP

#include <filesystem>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>

#include <munch/common/concepts.hpp>
#include <munch/core/lexer.hpp>
#include <munch/tools/tokenizer/tokenizer.hpp>

#include "hopper/parse/parse_error.hpp"
#include "hopper/parse/token_lookahead.hpp"

namespace hopper::parse
{
/**
 * @brief Reads a whole file into a string, in binary mode and without normalization.
 * @param file The file to read.
 * @return The file's bytes.
 * @throws Parse_error With kind Unreadable_file and an empty span when the file cannot be opened.
 */
[[nodiscard]] std::string read_source(const std::filesystem::path& file);

/**
 * @brief Turns a munch::core::Lexer into a one-token-lookahead stream of tokens.
 *
 * Token kinds accepted by the skip predicate (typically trivia such as whitespace) are discarded transparently; callers
 * only ever see meaningful tokens. The input is tokenized exactly as given: locations count "\r\n" and a lone '\r' as
 * one newline each, and offsets always index the original bytes, so the token set must recognize carriage returns
 * wherever its inputs may carry them.
 * @tparam Kind The token kind type produced by the lexer, an enum or an integral type.
 */
template <munch::common::concepts::Token_id Kind>
class Token_reader
{
public:
    /**
     * @brief Standard token stream result type.
     *
     * A three-state sum type: holds a `tokenizer::Token<Kind>` on success, a `tokenizer::End_of_input` marker once the
     * input is exhausted, or a `tokenizer::Error` on a lexical failure.
     */
    using Result_t = munch::tools::tokenizer::Tokenizer::Result_t<Kind>;

    /**
     * @brief Predicate selecting the token kinds the stream discards; a null one discards nothing.
     */
    using Skip_t = bool (*)(Kind);

    /**
     * @brief Constructs a token stream from a lexer.
     * @param lexer Lexer used to recognize tokens.
     * @param skip Predicate selecting the token kinds to discard.
     */
    explicit Token_reader(munch::core::Lexer lexer, Skip_t skip = {}) : tokenizer_{std::move(lexer)}, skip_{skip} {}

    /**
     * @brief Constructs a token stream from a lexer and an input string held in memory.
     * @param lexer Lexer used to recognize tokens.
     * @param input Input text to tokenize.
     * @param skip Predicate selecting the token kinds to discard.
     */
    explicit Token_reader(munch::core::Lexer lexer, const std::string& input, Skip_t skip = {})
        : tokenizer_{std::move(lexer), input}, skip_{skip}
    {}

    /**
     * @brief Constructs a token stream by reading the contents of a file.
     * @param lexer Lexer used to recognize tokens.
     * @param file Path to the file whose contents will be tokenized.
     * @param skip Predicate selecting the token kinds to discard.
     */
    explicit Token_reader(munch::core::Lexer lexer, const std::filesystem::path& file, Skip_t skip = {})
        : tokenizer_{std::move(lexer), read_source(file)}, skip_{skip}
    {}

    /**
     * @brief Replaces the current input and rewinds, so one reader and its compiled lexer serve many inputs.
     * @param input The new text.
     */
    void load(const std::string& input)
    {
        tokenizer_.load(input);

        lookahead_.reset();
    }

    /**
     * @brief Replaces the current input with a file's contents and rewinds.
     * @param file The file to read.
     * @throws Parse_error With kind Unreadable_file when the file cannot be opened.
     */
    void load(const std::filesystem::path& file)
    {
        tokenizer_.load(read_source(file));

        lookahead_.reset();
    }

    /**
     * @brief Rewinds to the beginning of the current input.
     */
    void reset() noexcept
    {
        tokenizer_.reset();

        lookahead_.reset();
    }

    /**
     * @brief Moves past a lexical error to the next position the lexer certifies as a token start.
     *
     * Call it only when the last read returned a lexical error, so the stream stands at the failure with nothing
     * buffered; a buffered token means the caller is not at a lexical error, and the call throws rather than drop it.
     * Munch's failure-anchored recovery, its contract inherited unchanged: the answer is a token start in every
     * completely tokenizable repair of the text before the returned evidence, no repair is promised to exist, and the
     * next read may error again. The skipped bytes advance the source location, so later spans stay right. When no
     * certified start lies ahead the position does not move.
     * @return The certified start with its evidence interval, or std::nullopt.
     * @throws std::logic_error If a token is buffered, so the stream does not stand at a lexical error.
     */
    [[nodiscard]] std::optional<munch::core::Lexer::Certified_start> recover()
    {
        if (lookahead_.token())
        {
            throw std::logic_error{"Token_reader::recover() called with a token buffered, not at a lexical error"};
        }

        const auto before{tokenizer_.offset()};

        const auto answer{tokenizer_.recover_from_failure()};

        if (answer)
        {
            lookahead_.skip(tokenizer_.input().substr(before, answer->start - before));
        }

        return answer;
    }

    /**
     * @brief Looks at the next token without consuming it.
     *
     * Returns a `tokenizer::Token<Kind>` on success, a `tokenizer::End_of_input` marker at end of input, or a
     * `tokenizer::Error` if a lexical issue occurs.
     */
    [[nodiscard]] Result_t peek()
    {
        if (const auto& token{lookahead_.token()}; token)
        {
            return own(*token);
        }

        for (;;)
        {
            const auto result{tokenizer_.next<Kind>()};

            if (!result.has_token())
            {
                return result;
            }

            const auto& token{result.token()};

            // Trivia moves the cursor and nothing else, so the end of the last token a parser consumed stays where that
            // token ended.
            if (skip_ && skip_(token.kind()))
            {
                lookahead_.skip(token.lexeme());

                continue;
            }

            lookahead_.advance(token.kind(), token.lexeme());

            return *lookahead_.token();
        }
    }

    /**
     * @brief Retrieves the next token from the stream.
     *
     * Returns a `tokenizer::Token<Kind>` on success, a `tokenizer::End_of_input` marker at end of input, or a
     * `tokenizer::Error` if a lexical issue occurs.
     */
    [[nodiscard]] Result_t next()
    {
        if (const auto expected{peek()}; !expected.has_token())
        {
            return expected;
        }

        return own(*lookahead_.consume());
    }

    /**
     * @brief The location of the current token's first character.
     *
     * Columns count bytes, not code points; offsets index the original input.
     */
    [[nodiscard]] const Token_location& location() const noexcept { return lookahead_.location(); }

    /**
     * @brief The span of the current token: its first byte to one past its last.
     */
    [[nodiscard]] Source_span span() const noexcept { return lookahead_.span(); }

    /**
     * @brief The end position of the most recently consumed token, where a finished construct actually stops.
     */
    [[nodiscard]] const Source_position& previous_end() const noexcept { return lookahead_.previous_end(); }

private:
    /**
     * @brief A buffered token as a view of this reader's own input.
     *
     * The lookahead keeps the token's text as a view, which in a copied or moved reader still points into the input it
     * was taken from; rebuilding it from the token's offset makes every reader hand out its own bytes.
     * @param token The buffered token.
     * @return The same kind over the same bytes of this reader's input.
     */
    [[nodiscard]] munch::tools::tokenizer::Token<Kind> own(const munch::tools::tokenizer::Token<Kind>& token) const
    {
        return {token.kind(), tokenizer_.input().substr(lookahead_.location().offset(), token.lexeme().size())};
    }

    /**
     * @brief The cursor over the input, owning the compiled lexer.
     */
    munch::tools::tokenizer::Tokenizer tokenizer_;

    /**
     * @brief The one token read ahead of the caller, with the positions around it.
     */
    Token_lookahead<Kind> lookahead_;

    /**
     * @brief The kinds discarded before the caller sees them; a null one discards nothing.
     */
    Skip_t skip_;
};

} // namespace hopper::parse

#endif // HOPPER_LIBS_PARSE_INCLUDE_HOPPER_PARSE_TOKEN_READER_HPP
