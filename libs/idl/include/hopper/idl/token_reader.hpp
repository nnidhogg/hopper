#ifndef PARSER_LIBS_IDL_INCLUDE_PARSER_IDL_TOKEN_READER_HPP
#define PARSER_LIBS_IDL_INCLUDE_PARSER_IDL_TOKEN_READER_HPP

#include <filesystem>
#include <lexer/tools/tokenizer/tokenizer.hpp>

#include "token_lookahead.hpp"
#include "tokens.hpp"

namespace parser::idl
{
class Token_reader
{
public:
    /**
     * @brief Standard token stream result type.
     *
     * Holds a `tokenizer::Token<T>` on success, `std::nullopt` on end of input,
     * or an `tokenizer::Error` on failure.
     */
    using Result_t = lexer::tools::tokenizer::Tokenizer::Result_t<Token_kind>;

    /**
     * @brief Construct a token stream from a lexer.
     * @param lexer Lexer used to recognize tokens.
     */
    explicit Token_reader(lexer::core::Lexer lexer);

    /**
     * @brief Construct a token stream from a lexer and an input string held in memory.
     * @param lexer Lexer used to recognize tokens.
     * @param input Input text to tokenize
     */
    explicit Token_reader(lexer::core::Lexer lexer, const std::string& input);

    /**
     * @brief Construct a token stream by reading the contents of a file.
     * @param lexer Lexer used to recognize tokens.
     * @param file Path to the file whose contents will be tokenized.
     *
     * The file is read in binary mode into an internal std::string using the private read() helper.
     */
    explicit Token_reader(lexer::core::Lexer lexer, const std::filesystem::path& file);

    /**
     * @brief Replace the current input and reset tokenization state.
     */
    void load(const std::string& input);

    /**
     * @brief Load new input from a file path.
     */
    void load(const std::filesystem::path& file);

    /**
     * @brief Reset the reading position to the beginning of the current input.
     */
    void reset() noexcept;

    /**
     * @brief Look at the next token without consuming it.
     *
     * Returns a `tokenizer::Token<T>` on success, `std::nullopt` at end of input,
     * or an `tokenizer::Error` if a lexical issue occurs.
     */
    [[nodiscard]] Result_t peek();

    /**
     * @brief Retrieve the next token from the stream.
     *
     * Returns a `tokenizer::Token<T>` on success, `std::nullopt` at end of input,
     * or an `tokenizer::Error` if a lexical issue occurs.
     */
    [[nodiscard]] Result_t next();

    /**
     * @brief Access the location associated with the current token.
     */
    [[nodiscard]] const Token_location& location() const noexcept;

private:
    /**
     * @brief Returns true if the given token kind should be discarded by the parser.
     */
    static bool skip_token(Token_kind kind) noexcept;

    /**
     * @brief Normalize newline sequences in a string.
     *
     * Converts all platform-dependent newline encodings ("\r\n", "\r") to a single '\n' form.
     * The input string is modified in-place and returned by value.
     *
     * @param input Input string to normalize.
     * @return Normalized string with unified newlines.
     */
    static std::string normalize(const std::string& input);

    /**
     * @brief Read and normalize a file.
     *
     * Reads the entire file contents and normalizes all newline sequences ("\r\n", "\r")
     * to a single '\n' form. The file is read in binary mode.
     *
     * @param file Path to the file to read.
     * @return Normalized file contents as a std::string.
     *
     * @throws std::runtime_error If the file cannot be opened.
     */
    static std::string normalize(const std::filesystem::path& file);

    /**
     * @brief Read the entire file contents into a string.
     *
     * Opens the file in binary mode and reads its contents without any newline normalization.
     *
     * @param file Path to the file to read.
     * @return File contents as a std::string.
     *
     * @throws std::runtime_error If the file cannot be opened.
     */
    static std::string read(const std::filesystem::path& file);

    lexer::tools::tokenizer::Tokenizer tokenizer_;

    Token_lookahead lookahead_;
};

} // namespace parser::idl

#endif // PARSER_LIBS_IDL_INCLUDE_PARSER_IDL_TOKEN_READER_HPP
