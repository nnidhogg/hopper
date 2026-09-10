#ifndef HOPPER_LIBS_PARSE_INCLUDE_HOPPER_PARSE_TOKEN_READER_HPP
#define HOPPER_LIBS_PARSE_INCLUDE_HOPPER_PARSE_TOKEN_READER_HPP

#include <filesystem>
#include <fstream>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>

#include <munch/core/lexer.hpp>
#include <munch/tools/tokenizer/tokenizer.hpp>

#include "hopper/parse/token_lookahead.hpp"

namespace hopper::parse
{
/**
 * @brief Turns a munch::core::Lexer into a one-token-lookahead stream of tokens.
 *
 * Token kinds accepted by the skip predicate (typically trivia such as whitespace) are discarded transparently;
 * callers only ever see meaningful tokens. The input is tokenized exactly as given: locations count "\r\n" and a
 * lone '\r' as one newline each, and offsets always index the original bytes, so the token set must recognize
 * carriage returns wherever its inputs may carry them.
 * @tparam Kind The token kind type (enum or integral) produced by the lexer.
 */
template <typename Kind>
class Token_reader
{
public:
    /**
     * @brief Standard token stream result type.
     *
     * A three-state sum type: holds a `tokenizer::Token<Kind>` on success, a `tokenizer::End_of_input` marker once
     * the input is exhausted, or a `tokenizer::Error` on a lexical failure.
     */
    using Result_t = munch::tools::tokenizer::Tokenizer::Result_t<Kind>;

    /**
     * @brief Predicate selecting the token kinds the stream discards; nullptr discards nothing.
     */
    using Skip_t = bool (*)(Kind);

    /**
     * @brief Construct a token stream from a lexer.
     * @param lexer Lexer used to recognize tokens.
     * @param skip Predicate selecting the token kinds to discard.
     */
    explicit Token_reader(munch::core::Lexer lexer, const Skip_t skip = nullptr)
        : tokenizer_{std::move(lexer)}, skip_{skip}
    {}

    /**
     * @brief Construct a token stream from a lexer and an input string held in memory.
     * @param lexer Lexer used to recognize tokens.
     * @param input Input text to tokenize.
     * @param skip Predicate selecting the token kinds to discard.
     */
    explicit Token_reader(munch::core::Lexer lexer, const std::string& input, const Skip_t skip = nullptr)
        : tokenizer_{std::move(lexer), input}, skip_{skip}
    {}

    /**
     * @brief Construct a token stream by reading the contents of a file.
     * @param lexer Lexer used to recognize tokens.
     * @param file Path to the file whose contents will be tokenized.
     * @param skip Predicate selecting the token kinds to discard.
     */
    explicit Token_reader(munch::core::Lexer lexer, const std::filesystem::path& file, const Skip_t skip = nullptr)
        : tokenizer_{std::move(lexer), read(file)}, skip_{skip}
    {}

    /**
     * @brief Replace the current input and reset tokenization state.
     */
    void load(const std::string& input)
    {
        tokenizer_.load(input);

        lookahead_.reset();
    }

    /**
     * @brief Load new input from a file path.
     */
    void load(const std::filesystem::path& file)
    {
        tokenizer_.load(read(file));

        lookahead_.reset();
    }

    /**
     * @brief Reset the reading position to the beginning of the current input.
     */
    void reset() noexcept
    {
        tokenizer_.reset();

        lookahead_.reset();
    }

    /**
     * @brief Move past a lexical error to the next position the lexer certifies as a token start.
     *
     * Call it only when the last read returned a lexical error, so the stream stands at the failure with nothing
     * buffered; a buffered token means the caller is not at a lexical error, and the call throws rather than drop it.
     * Munch's failure-anchored recovery, its contract inherited unchanged: the answer is a token start in every
     * completely tokenizable repair of the text before the returned evidence, no repair is promised to exist, and
     * the next read may error again. The skipped bytes advance the source location, so later spans stay right.
     * When no certified start lies ahead the position does not move.
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
     * @brief Look at the next token without consuming it.
     *
     * Returns a `tokenizer::Token<Kind>` on success, a `tokenizer::End_of_input` marker at end of input, or a
     * `tokenizer::Error` if a lexical issue occurs.
     */
    [[nodiscard]] Result_t peek()
    {
        if (const auto& token = lookahead_.token(); token)
        {
            return *token;
        }

        for (;;)
        {
            const auto result{tokenizer_.next<Kind>()};

            if (!result.has_token())
            {
                return result;
            }

            const auto& token{result.token()};

            lookahead_.advance(token.kind(), token.lexeme());

            if (skip_ && skip_(token.kind()))
            {
                lookahead_.consume();

                continue;
            }

            return *lookahead_.token();
        }
    }

    /**
     * @brief Retrieve the next token from the stream.
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

        return *lookahead_.consume();
    }

    /**
     * @brief Access the location of the current token's first character.
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
     * @brief Read the entire file contents into a string, in binary mode and without normalization.
     * @param file Path to the file to read.
     * @return File contents as a std::string.
     * @throws std::runtime_error If the file cannot be opened.
     */
    static std::string read(const std::filesystem::path& file)
    {
        if (std::ifstream stream{file, std::ios::binary}; stream.is_open())
        {
            return {std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>()};
        }

        throw std::runtime_error("Token_reader: cannot open file: " + file.string());
    }

    munch::tools::tokenizer::Tokenizer tokenizer_;

    Token_lookahead<Kind> lookahead_;

    Skip_t skip_;
};

} // namespace hopper::parse

#endif // HOPPER_LIBS_PARSE_INCLUDE_HOPPER_PARSE_TOKEN_READER_HPP
