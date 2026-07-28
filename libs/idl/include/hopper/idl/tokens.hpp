#ifndef PARSER_LIBS_IDL_INCLUDE_PARSER_IDL_TOKENS_HPP
#define PARSER_LIBS_IDL_INCLUDE_PARSER_IDL_TOKENS_HPP

#include <cstdint>

namespace parser::idl
{
enum class Token_kind : uint8_t
{
    // Keywords
    Keyword_interface,
    Keyword_attribute,
    Keyword_operation,
    Keyword_exception,
    Keyword_raises,
    Keyword_in,
    Keyword_out,
    Keyword_inout,
    Keyword_module,
    Keyword_const,
    Keyword_typedef,
    Keyword_struct,
    Keyword_union,
    Keyword_switch,
    Keyword_case,
    Keyword_default,
    Keyword_enum,
    Keyword_sequence,
    Keyword_string,
    Keyword_wstring,
    Keyword_any,
    Keyword_octet,
    Keyword_long,
    Keyword_short,
    Keyword_unsigned,
    Keyword_float,
    Keyword_double,
    Keyword_boolean,
    Keyword_char,
    Keyword_wchar,
    Keyword_void,

    // Symbols
    Symbol_semicolon,
    Symbol_colon,
    Symbol_comma,
    Symbol_equals,
    Symbol_lparen,
    Symbol_rparen,
    Symbol_lbrace,
    Symbol_rbrace,
    Symbol_lbracket,
    Symbol_rbracket,

    // Identifiers and literals
    Identifier,
    Integer_literal,
    Floating_point_literal,
    Fixed_point_literal,
    String_literal,
    Character_literal,

    // Operators
    Operator_plus,
    Operator_minus,
    Operator_asterisk,
    Operator_slash,

    // Trivia
    Whitespace,
    Newline,
    Single_line_comment,
    Multi_line_comment,
};

} // namespace parser::idl

#endif // PARSER_LIBS_IDL_INCLUDE_PARSER_IDL_TOKENS_HPP
