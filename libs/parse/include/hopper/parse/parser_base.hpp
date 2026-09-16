#ifndef HOPPER_LIBS_PARSE_INCLUDE_HOPPER_PARSE_PARSER_BASE_HPP
#define HOPPER_LIBS_PARSE_INCLUDE_HOPPER_PARSE_PARSER_BASE_HPP

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

#include <munch/common/concepts.hpp>
#include <munch/core/lexer.hpp>
#include <munch/tools/tokenizer/token.hpp>

#include "hopper/parse/parse_error.hpp"
#include "hopper/parse/source_span.hpp"
#include "hopper/parse/token_reader.hpp"

namespace hopper::parse
{
/**
 * @brief Base class providing the token-stream plumbing every recursive-descent parser needs.
 *
 * Wraps a Token_reader and exposes the standard LL(1) primitives: peeking, consuming, conditional acceptance,
 * required expectation, and structured error reporting. A concrete parser derives from this and adds only its
 * grammar functions.
 * @tparam Kind The token kind type produced by the lexer, an enum or an integral type.
 */
template <munch::common::concepts::Token_id Kind>
class Parser_base
{
public:
    /**
     * @brief The token type produced by the underlying lexer.
     */
    using Token_t = munch::tools::tokenizer::Token<Kind>;

    /**
     * @brief The token stream a grammar reads.
     */
    using Reader_t = Token_reader<Kind>;

    /**
     * @brief The predicate naming the kinds the stream discards as trivia.
     */
    using Skip_t = typename Reader_t::Skip_t;

    /**
     * @brief Replaces the input and rewinds, so one parser and its compiled lexer serve many inputs in sequence.
     * @param input The new text.
     */
    void load(const std::string& input) { reader_.load(input); }

    /**
     * @brief Replaces the input with a file's contents and rewinds.
     * @param file The file to read.
     */
    void load(const std::filesystem::path& file) { reader_.load(file); }

    /**
     * @brief Rewinds to the beginning of the current input.
     */
    void reset() noexcept { reader_.reset(); }

    /**
     * @brief Moves the stream past a lexical error to the lexer's next certified token start, or refuses.
     *
     * Call it after catching the Parse_error that a read threw for a lexical error; the stream then stands at the
     * failure with nothing buffered. After a syntax error a token is buffered and the call throws, since skipping
     * from there is the parser's own policy, not a certified resynchronization. The lexical contract is munch's,
     * inherited unchanged; what the parser does at the resumed position is the derived parser's policy.
     * std::nullopt means no certified start lies ahead and the position did not move.
     * @return The certified start with its evidence interval, or std::nullopt.
     * @throws std::logic_error If a token is buffered.
     */
    [[nodiscard]] std::optional<munch::core::Lexer::Certified_start> recover() { return reader_.recover(); }

protected:
    /**
     * @brief Constructs the base around a token stream.
     * @param reader The stream the grammar reads.
     */
    explicit Parser_base(Reader_t reader) : reader_{std::move(reader)} {}

    /**
     * @brief Constructs the base over a text held in memory, with the grammar's lexer and trivia predicate.
     * @param lexer The compiled lexer.
     * @param input The text.
     * @param skip The predicate naming the kinds to discard.
     */
    Parser_base(munch::core::Lexer lexer, const std::string& input, Skip_t skip)
        : reader_{std::move(lexer), input, std::move(skip)}
    {}

    /**
     * @brief Constructs the base over a file's contents, with the grammar's lexer and trivia predicate.
     * @param lexer The compiled lexer.
     * @param file The file to read.
     * @param skip The predicate naming the kinds to discard.
     */
    Parser_base(munch::core::Lexer lexer, const std::filesystem::path& file, Skip_t skip)
        : reader_{std::move(lexer), file, std::move(skip)}
    {}

    /**
     * @brief Protected like the constructor: the base is a mixin for a grammar, never a handle a caller deletes
     *        through, so it needs no virtual destructor.
     */
    ~Parser_base() = default;

    /**
     * @brief Consumes the next token.
     * @return The token, or std::nullopt at end of input.
     * @throws Parse_error With kind Lexical when the lexer rejects the input.
     */
    [[nodiscard]] std::optional<Token_t> next_token() { return surface(reader_.next()); }

    /**
     * @brief Looks at the next token without consuming it.
     * @return The token, or std::nullopt at end of input.
     * @throws Parse_error With kind Lexical when the lexer rejects the input.
     */
    [[nodiscard]] std::optional<Token_t> peek_token() { return surface(reader_.peek()); }

    /**
     * @brief Whether the next token has a kind, without consuming it.
     * @param kind The kind asked for.
     * @return True when the next token has it.
     */
    [[nodiscard]] bool check(const Kind kind)
    {
        const auto token{peek_token()};

        return token && token->kind() == kind;
    }

    /**
     * @brief Consumes the next token if it has a kind.
     * @param kind The kind asked for.
     * @return The token, or std::nullopt when the next token has another kind or the input has ended.
     */
    [[nodiscard]] std::optional<Token_t> accept(const Kind kind)
    {
        if (!check(kind))
        {
            return std::nullopt;
        }

        return next_token();
    }

    /**
     * @brief Consumes the next token, which must exist.
     * @param what What the grammar expected, named in the error.
     * @return The token.
     * @throws Parse_error With kind Unexpected_end when the input has ended.
     */
    [[nodiscard]] Token_t require(const std::string_view what)
    {
        const auto token{next_token()};

        if (!token)
        {
            eof_error("Expected " + std::string(what) + " before end of input");
        }

        return *token;
    }

    /**
     * @brief Consumes the next token, which must have a kind.
     * @param kind The required kind.
     * @param what What the grammar expected, named in the error.
     * @return The token.
     * @throws Parse_error With kind Unexpected_token when the next token has another kind, Unexpected_end when the
     *         input has ended.
     */
    [[nodiscard]] Token_t expect(const Kind kind, const std::string_view what)
    {
        const auto token{require(what)};

        if (token.kind() != kind)
        {
            syntax_error("Expected " + std::string(what), token);
        }

        return token;
    }

    /**
     * @brief Consumes the next token, which must have a kind, when the grammar has no use for the token itself.
     * @param kind The required kind.
     * @param what What the grammar expected, named in the error.
     * @throws Parse_error As expect() does.
     */
    void consume(const Kind kind, const std::string_view what) { static_cast<void>(expect(kind, what)); }

    /**
     * @brief Where the next construct begins: the next token's start, or, when no token remains, the end of the
     *        last consumed token.
     *
     * Capture this before parsing a construct and close the span with span_from() after it.
     * @return The position.
     */
    [[nodiscard]] Source_position mark()
    {
        if (peek_token())
        {
            return reader_.span().begin;
        }

        return reader_.previous_end();
    }

    /**
     * @brief The span from a mark to the end of the most recently consumed token.
     * @param begin The mark the construct began at.
     * @return The span.
     */
    [[nodiscard]] Source_span span_from(const Source_position& begin) const noexcept
    {
        return {.begin = begin, .end = reader_.previous_end()};
    }

    /**
     * @brief Raises the error for a token out of place, pointing at it.
     * @param message What the grammar expected.
     * @param where The token found instead, quoted in the message.
     */
    [[noreturn]] void syntax_error(const std::string_view message, const Token_t& where)
    {
        throw Parse_error{
                Parse_error_kind::Unexpected_token, reader_.span(),
                "Syntax error: " + std::string(message) + ", got '" + std::string(where.lexeme()) + "'"};
    }

    /**
     * @brief Raises the error for input that ended too early, pointing one past the last consumed token.
     * @param message What the grammar expected.
     */
    [[noreturn]] void eof_error(const std::string_view message)
    {
        const auto& at{reader_.previous_end()};

        throw Parse_error{
                Parse_error_kind::Unexpected_end, Source_span{.begin = at, .end = at},
                "Syntax error: " + std::string(message)};
    }

    /**
     * @brief Raises the error for input the lexer rejected, pointing at where tokenization stopped.
     * @param message The lexer's message.
     */
    [[noreturn]] void lexical_error(const std::string& message)
    {
        const auto at{reader_.span().end};

        throw Parse_error{Parse_error_kind::Lexical, Source_span{.begin = at, .end = at}, "Lexical error: " + message};
    }

private:
    /**
     * @brief Turns a read's result into a token, nothing at the end of input, or the lexical error.
     * @param result What the reader answered.
     * @return The token, or std::nullopt at end of input.
     * @throws Parse_error With kind Lexical when the lexer rejected the input.
     */
    [[nodiscard]] std::optional<Token_t> surface(const typename Reader_t::Result_t& result)
    {
        if (result.has_error())
        {
            lexical_error(result.error().message());
        }

        if (result.has_token())
        {
            return result.token();
        }

        return std::nullopt;
    }

    /**
     * @brief The token stream the grammar reads.
     */
    Reader_t reader_;
};

} // namespace hopper::parse

#endif // HOPPER_LIBS_PARSE_INCLUDE_HOPPER_PARSE_PARSER_BASE_HPP
