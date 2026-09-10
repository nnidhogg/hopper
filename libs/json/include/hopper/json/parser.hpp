#ifndef HOPPER_LIBS_JSON_INCLUDE_HOPPER_JSON_PARSER_HPP
#define HOPPER_LIBS_JSON_INCLUDE_HOPPER_JSON_PARSER_HPP

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include <munch/core/lexer.hpp>

#include "hopper/json/tokens.hpp"
#include "hopper/json/value.hpp"
#include "hopper/parse/parser_base.hpp"
#include "hopper/parse/token_reader.hpp"

namespace hopper::json
{
/**
 * @brief Parses one RFC 8259 JSON text into a Value tree.
 *
 * The parser keeps its own stack of open arrays and objects rather than recursing, so a document nested as deep as
 * memory allows parses without touching the call stack, and the stack is exactly the state a JSON parser carries: the
 * open containers, and for an object, the name waiting for its value. Every value carries its source span. Numbers
 * are kept as spelled; strings are unescaped to UTF-8, with surrogate pairs in \u escapes combined and a lone
 * surrogate refused.
 */
class Parser : public parse::Parser_base<Token_kind>
{
public:
    /**
     * @brief The token reader type this parser consumes.
     */
    using Token_reader_t = parse::Token_reader<Token_kind>;

    /**
     * @brief Constructs a parser over a prepared reader.
     * @param reader The reader, whose lexer is expected to be lexer() with whitespace discarded.
     */
    explicit Parser(Token_reader_t reader);

    /**
     * @brief Constructs a parser over a text held in memory, using lexer() and discarding whitespace.
     * @param input The JSON text.
     */
    explicit Parser(const std::string& input);

    /**
     * @brief Constructs a parser over a file's contents, using lexer() and discarding whitespace.
     * @param file The file to read.
     */
    explicit Parser(const std::filesystem::path& file);

    using parse::Parser_base<Token_kind>::load;
    using parse::Parser_base<Token_kind>::reset;
    using parse::Parser_base<Token_kind>::recover;

    /**
     * @brief Parses the whole input as one JSON text: a single value with nothing but whitespace around it.
     * @return The value tree.
     * @throws parse::Parse_error On a lexical error, on a token out of place, on an input that ends inside a value,
     *         on trailing text after the value, and on a string whose \u escapes leave a surrogate unpaired.
     */
    [[nodiscard]] Value parse();

    /**
     * @brief Resolves the escapes of a string token's lexeme.
     * @param lexeme The token text, quotes included, already known to match the string grammar.
     * @param span The token's source span, named in the error a lone surrogate raises.
     * @return The string's characters as UTF-8.
     * @throws parse::Parse_error With kind Invalid_literal when a \u escape spells a surrogate that is not one half
     *         of a pair.
     */
    [[nodiscard]] static std::string unescape(std::string_view lexeme, const parse::Source_span& span);

private:
    /**
     * @brief One open container on the parser's stack.
     *
     * An array frame collects elements; an object frame collects members and carries the name whose value is being
     * read, since a JSON object states the name before the value it belongs to and nothing else remembers it. The
     * begin position is the opening bracket's, so the finished container's span can be closed at the closing one.
     */
    struct Frame
    {
        /**
         * @brief The array or object being filled.
         */
        Value container;

        /**
         * @brief For an object frame, the name whose value is next; unused by an array frame.
         */
        std::string name;

        /**
         * @brief The opening bracket's position, which begins the container's span.
         */
        parse::Source_position begin;
    };

    /**
     * @brief Reads the next token or raises the end-of-input error naming what was expected.
     * @param what What the grammar expected, for the message.
     * @return The token.
     */
    [[nodiscard]] Token_t next_or_end(std::string_view what);

    /**
     * @brief Builds a scalar value from a token that spells one.
     * @param token The token, of a scalar kind.
     * @param span The token's source span.
     * @return The value.
     */
    [[nodiscard]] Value scalar(const Token_t& token, const parse::Source_span& span);

    /**
     * @brief Reads a member name and the colon after it, leaving the reader on the member's value.
     *
     * Both places a member name can appear, the first of an object and each one after a comma, need exactly this,
     * so it is stated once; the colon is consumed here because a name without one is not a member.
     * @return The name's characters, escapes resolved.
     */
    [[nodiscard]] std::string read_member_name();

    /**
     * @brief Closes the container just opened if the very next token ends it.
     *
     * An empty container is the one case where opening and closing happen without a value in between, and both
     * array and object need it, so neither case has to special-case its own emptiness.
     * @param stack The parser's stack, whose top frame was just pushed.
     * @param closer The bracket that would end this container.
     * @return The finished empty container, or nothing when the container has contents.
     */
    [[nodiscard]] std::optional<Value> close_if_empty(std::vector<Frame>& stack, Token_kind closer);

    /**
     * @brief Reads the value that is due, opening a frame when it is a container.
     *
     * A scalar is complete the moment it is read; a container is not, so it becomes a frame and the next value due
     * is its first element or member. Returning nothing is how that difference is reported.
     * @param stack The parser's stack, pushed to when the value opens a container.
     * @return The completed value, or nothing when a container opened and its contents are still to come.
     */
    [[nodiscard]] std::optional<Value> open_value(std::vector<Frame>& stack);

    /**
     * @brief Puts a completed value into the container on top of the stack and reads the separator after it.
     *
     * The separator decides what happens next, so it is read here rather than by the caller: a comma means another
     * value is due and the frame stays open, and the closing bracket finishes the container, which then becomes the
     * completed value for whatever frame lies beneath it.
     * @param stack The parser's stack, whose top frame receives the value.
     * @param completed The value to put into it.
     * @return The finished container when it closed, or nothing when more elements or members follow.
     */
    [[nodiscard]] std::optional<Value> close_value(std::vector<Frame>& stack, Value completed);
};
} // namespace hopper::json

#endif // HOPPER_LIBS_JSON_INCLUDE_HOPPER_JSON_PARSER_HPP
