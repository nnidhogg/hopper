#ifndef HOPPER_LIBS_PARSE_INCLUDE_HOPPER_PARSE_TOKEN_READER_HPP
#define HOPPER_LIBS_PARSE_INCLUDE_HOPPER_PARSE_TOKEN_READER_HPP

#include <filesystem>
#include <fstream>
#include <munch/core/lexer.hpp>
#include <munch/tools/tokenizer/tokenizer.hpp>
#include <stdexcept>
#include <string>
#include <utility>

#include "hopper/parse/token_lookahead.hpp"

namespace hopper::parse
{
/**
 * @brief Turns a munch::core::Lexer into a one-token-lookahead stream of tokens.
 *
 * Token kinds accepted by the skip predicate (typically trivia such as whitespace) are discarded transparently;
 * callers only ever see meaningful tokens. Newlines in the input are normalized to '\n' before tokenization.
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
        : tokenizer_{std::move(lexer), normalize(input)}, skip_{skip}
    {}

    /**
     * @brief Construct a token stream by reading the contents of a file.
     * @param lexer Lexer used to recognize tokens.
     * @param file Path to the file whose contents will be tokenized.
     * @param skip Predicate selecting the token kinds to discard.
     */
    explicit Token_reader(munch::core::Lexer lexer, const std::filesystem::path& file, const Skip_t skip = nullptr)
        : tokenizer_{std::move(lexer), normalize(file)}, skip_{skip}
    {}

    /**
     * @brief Replace the current input and reset tokenization state.
     */
    void load(const std::string& input)
    {
        tokenizer_.load(normalize(input));

        lookahead_.reset();
    }

    /**
     * @brief Load new input from a file path.
     */
    void load(const std::filesystem::path& file)
    {
        tokenizer_.load(normalize(file));

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
     * @brief Access the location associated with the current token.
     */
    [[nodiscard]] const Token_location& location() const noexcept
    {
        return lookahead_.location();
    }

private:
    /**
     * @brief Normalize newline sequences in a string.
     *
     * Converts all platform-dependent newline encodings ("\r\n", "\r") to a single '\n' form.
     * @param input Input string to normalize.
     * @return Normalized string with unified newlines.
     */
    static std::string normalize(const std::string& input)
    {
        std::string output;

        output.reserve(input.size());

        for (auto iterator = input.cbegin(); iterator != input.cend();)
        {
            if (const char c = *iterator++; c == '\r')
            {
                if (iterator != input.cend() && *iterator == '\n')
                {
                    ++iterator;
                }

                output.push_back('\n');
            }
            else
            {
                output.push_back(c);
            }
        }

        return output;
    }

    /**
     * @brief Read and normalize a file.
     * @param file Path to the file to read.
     * @return Normalized file contents as a std::string.
     * @throws std::runtime_error If the file cannot be opened.
     */
    static std::string normalize(const std::filesystem::path& file)
    {
        return normalize(read(file));
    }

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
