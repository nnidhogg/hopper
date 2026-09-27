#include "hopper/json/parser.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

#include "hopper/parse/parse_error.hpp"

namespace hopper::json
{
namespace
{
/**
 * @brief The first code point above the basic plane, where a surrogate pair's arithmetic begins.
 */
constexpr std::uint32_t supplementary_base{0x10000};

/**
 * @brief The six bytes a second \u escape occupies: a backslash, a u and four digits.
 */
constexpr std::size_t escape_width{6};

/**
 * @brief A \u escape as read: the code point it names and where it ends.
 */
struct Unicode_escape
{
    /**
     * @brief The code point, a scalar value, never a surrogate.
     */
    std::uint32_t code_point;

    /**
     * @brief The offset of the escape's last digit, which the caller resumes after.
     */
    std::size_t last;
};

/**
 * @brief The numeric value of one hex digit, already known to be one.
 * @param digit The digit character.
 * @return Its value, zero to fifteen.
 */
std::uint32_t hex_value(const char digit)
{
    if (digit >= '0' && digit <= '9')
    {
        return static_cast<std::uint32_t>(digit - '0');
    }

    if (digit >= 'a' && digit <= 'f')
    {
        return static_cast<std::uint32_t>(digit - 'a' + 10);
    }

    return static_cast<std::uint32_t>(digit - 'A' + 10);
}

/**
 * @brief The four hex digits at an offset, read as one code unit.
 * @param text The string interior being scanned.
 * @param from The offset of the first of the four digits.
 * @return The code unit they spell.
 */
std::uint32_t code_unit_at(const std::string_view text, const std::size_t from)
{
    return (hex_value(text[from]) << 12) | (hex_value(text[from + 1]) << 8) | (hex_value(text[from + 2]) << 4) |
           hex_value(text[from + 3]);
}

/**
 * @brief The character a one-character escape names, or nothing when the escape is a \u.
 *
 * The grammar admits exactly these nine escapes, so anything that is not one of the eight here is the ninth; the caller
 * reads that as its cue rather than testing for 'u' a second time.
 * @param escape The character after the backslash.
 * @return The character it names, or std::nullopt for \u.
 */
std::optional<char> one_character_escape(const char escape)
{
    switch (escape)
    {
    case '"':
        return '"';
    case '\\':
        return '\\';
    case '/':
        return '/';
    case 'b':
        return '\b';
    case 'f':
        return '\f';
    case 'n':
        return '\n';
    case 'r':
        return '\r';
    case 't':
        return '\t';
    default:
        return std::nullopt;
    }
}

/**
 * @brief One code point encoded as UTF-8.
 * @param code_point The scalar value, at most U+10FFFF and never a surrogate.
 * @return Its one to four bytes.
 */
std::string utf8(const std::uint32_t code_point)
{
    const auto byte{[](const std::uint32_t bits) { return static_cast<char>(bits); }};

    if (code_point < 0x80)
    {
        return {byte(code_point)};
    }

    if (code_point < 0x800)
    {
        return {byte(0xC0 | (code_point >> 6)), byte(0x80 | (code_point & 0x3F))};
    }

    if (code_point < supplementary_base)
    {
        return {byte(0xE0 | (code_point >> 12)), byte(0x80 | ((code_point >> 6) & 0x3F)),
                byte(0x80 | (code_point & 0x3F))};
    }

    return {byte(0xF0 | (code_point >> 18)), byte(0x80 | ((code_point >> 12) & 0x3F)),
            byte(0x80 | ((code_point >> 6) & 0x3F)), byte(0x80 | (code_point & 0x3F))};
}

/**
 * @brief Whether a code unit is a high (leading) surrogate.
 * @param unit The code unit.
 * @return True for U+D800 to U+DBFF.
 */
bool is_high_surrogate(const std::uint32_t unit) noexcept
{
    return unit >= 0xD800 && unit <= 0xDBFF;
}

/**
 * @brief Whether a code unit is a low (trailing) surrogate.
 * @param unit The code unit.
 * @return True for U+DC00 to U+DFFF.
 */
bool is_low_surrogate(const std::uint32_t unit) noexcept
{
    return unit >= 0xDC00 && unit <= 0xDFFF;
}

/**
 * @brief Reads the character a \u escape names, taking a second escape when the first is a high surrogate.
 *
 * A \u escape spells a UTF-16 code unit, not a character, so a code point above the basic plane arrives as two of them
 * and only the pair names anything. Either half alone is refused: the grammar accepts the syntax, so this is the only
 * place the document can be told that what it spelled is not a character.
 * @param text The string interior being scanned.
 * @param escape The offset of the 'u', whose four digits follow it.
 * @param span The string token's span, named in the error a lone surrogate raises.
 * @return The code point and the offset of the escape's last digit, which the caller resumes after.
 * @throws parse::Parse_error With kind Invalid_literal when the escape leaves a surrogate unpaired.
 */
Unicode_escape unicode_escape(const std::string_view text, const std::size_t escape, const parse::Source_span& span)
{
    const auto unit{code_unit_at(text, escape + 1)};

    const auto last{escape + 4};

    if (is_low_surrogate(unit))
    {
        throw parse::Parse_error{
                parse::Parse_error_kind::Invalid_literal, span,
                "Invalid string: a low surrogate escape with no high surrogate before it"};
    }

    if (!is_high_surrogate(unit))
    {
        return {.code_point = unit, .last = last};
    }

    const bool paired{
            last + escape_width < text.size() && text[last + 1] == '\\' && text[last + 2] == 'u' &&
            is_low_surrogate(code_unit_at(text, last + 3))};

    if (!paired)
    {
        throw parse::Parse_error{
                parse::Parse_error_kind::Invalid_literal, span,
                "Invalid string: a high surrogate escape with no low surrogate after it"};
    }

    const auto low{code_unit_at(text, last + 3)};

    return {.code_point = supplementary_base + ((unit - 0xD800) << 10) + (low - 0xDC00), .last = last + escape_width};
}

} // namespace

Parser::Parser(Reader_t reader) : Parser_base{std::move(reader)}
{}

Parser::Parser(const std::string& input) : Parser_base{lexer(), input, is_trivia}
{}

Parser::Parser(const std::filesystem::path& file) : Parser_base{lexer(), file, is_trivia}
{}

Value Parser::parse()
{
    // A parse that threw leaves its frames behind; the next one starts from an empty stack.
    open_.clear();

    // Two phases alternate: a value is due, or a value is in hand and belongs somewhere. Which one holds is exactly
    // whether done carries anything, so the stack and this optional are the parser's whole state, and the document is
    // finished when a value is in hand with nothing open to put it in.
    auto done{begin_value()};

    while (!done || !open_.empty())
    {
        done = done ? place(std::move(*done)) : begin_value();
    }

    if (const auto trailing{peek_token()})
    {
        syntax_error("Expected end of input after the value", *trailing);
    }

    return std::move(*done);
}

std::optional<Value> Parser::begin_value()
{
    const auto begin{mark()};

    const auto token{require("a value")};

    switch (token.kind())
    {
    case Token_kind::Left_bracket:
        open_.push_back({.container = Array{}, .name = {}, .begin = begin});

        if (accept(Token_kind::Right_bracket))
        {
            return close();
        }

        return std::nullopt;

    case Token_kind::Left_brace:
        open_.push_back({.container = Object{}, .name = {}, .begin = begin});

        if (accept(Token_kind::Right_brace))
        {
            return close();
        }

        // A non-empty object states its first name before its first value, so the frame takes it now; an array has
        // nothing to read here, which is the only difference between the two cases.
        open_.back().name = member_name();

        return std::nullopt;

    case Token_kind::String:
    case Token_kind::Number:
    case Token_kind::True:
    case Token_kind::False:
    case Token_kind::Null:
        return scalar(token, span_from(begin));

    default:
        syntax_error("Expected a value", token);
    }
}

Value Parser::close()
{
    auto [container, name, begin]{std::move(open_.back())};

    open_.pop_back();

    return Value{std::move(container), span_from(begin)};
}

std::string Parser::member_name()
{
    const auto begin{mark()};

    const auto name{expect(Token_kind::String, "a member name")};

    auto text{unescape(name.lexeme(), span_from(begin))};

    consume(Token_kind::Colon, "':' after the member name");

    return text;
}

std::string Parser::unescape(const std::string_view lexeme, const parse::Source_span& span)
{
    std::string out{};

    out.reserve(lexeme.size());

    // The interior, between the quotes; the grammar guarantees every backslash starts a complete escape, which is what
    // lets this read the character after one without checking that there is a character after one.
    const auto interior{lexeme.substr(1, lexeme.size() - 2)};

    for (std::size_t at{0}; at < interior.size(); ++at)
    {
        if (interior[at] != '\\')
        {
            out.push_back(interior[at]);

            continue;
        }

        ++at;

        if (const auto simple{one_character_escape(interior[at])})
        {
            out.push_back(*simple);

            continue;
        }

        const auto [code_point, last]{unicode_escape(interior, at, span)};

        out += utf8(code_point);

        at = last;
    }

    return out;
}

Value Parser::scalar(const Token_t& token, const parse::Source_span& span)
{
    switch (token.kind())
    {
    case Token_kind::String:
        return Value{unescape(token.lexeme(), span), span};
    case Token_kind::Number:
        return Value{Number{.text = std::string{token.lexeme()}}, span};
    case Token_kind::True:
        return Value{true, span};
    case Token_kind::False:
        return Value{false, span};
    default:
        return Value{Null{}, span};
    }
}

std::optional<Value> Parser::place(Value value)
{
    auto& top{open_.back()};

    if (auto* const array{std::get_if<Array>(&top.container)})
    {
        array->elements.push_back(std::move(value));

        const auto token{require("',' or ']'")};

        if (token.kind() == Token_kind::Comma)
        {
            return std::nullopt;
        }

        if (token.kind() != Token_kind::Right_bracket)
        {
            syntax_error("Expected ',' or ']'", token);
        }

        return close();
    }

    // Only arrays and objects are ever pushed, so the frame that is not an array is an object.
    std::get<Object>(top.container).members.push_back({.name = std::move(top.name), .value = std::move(value)});

    const auto token{require("',' or '}'")};

    if (token.kind() == Token_kind::Comma)
    {
        top.name = member_name();

        return std::nullopt;
    }

    if (token.kind() != Token_kind::Right_brace)
    {
        syntax_error("Expected ',' or '}'", token);
    }

    return close();
}

} // namespace hopper::json
