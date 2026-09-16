# **Usage Overview**

How the kit is used: parsing JSON and the C-like study grammar, writing a grammar of your own on the kit, editing a JSON
text with the stream kept current, and recovering after a lexical error. The [README](../README.md) shows one complete
program; [design.md](design.md) says why the kit is shaped as it is.

## **Integrating with CMake**

```cmake
add_subdirectory(external/hopper)

target_link_libraries(your_target PRIVATE hopper::json)   # or hopper::clike, or hopper::parse for the kit alone
```

`hopper::hopper` carries all three. To install hopper, build it against an installed munch rather than the submodule,
`-DHOPPER_SYSTEM_MUNCH=ON -DHOPPER_INSTALL=ON`, so the exported targets refer to munch's own installed package; the
libraries, headers and a package config then install under the usual prefix, and a consumer writes
`find_package(hopper)` and links the same `hopper::` names.

## **Parsing JSON**

```cpp
#include <iostream>
#include <string>

#include <hopper/json/parser.hpp>
#include <hopper/parse/parse_error.hpp>

int main()
{
    const std::string text{R"({"name": "hopper", "tags": ["json", "clike"], "stable": false})"};

    try
    {
        hopper::json::Parser parser{text};

        const auto document{parser.parse()};

        const auto& object{document.as_object()};

        std::cout << object.at("name").as_string() << " has " << object.at("tags").as_array().elements.size()
                  << " tags\n";

        // Every value knows where it came from: byte offsets into the original text, and a line and column.
        const auto& stable{object.at("stable")};

        std::cout << "stable spans bytes " << stable.span.begin.offset << " to " << stable.span.end.offset << '\n';
    }
    catch (const hopper::parse::Parse_error& error)
    {
        std::cerr << error.what() << " at " << error.span().begin.line << ':' << error.span().begin.column << '\n';
    }
}
```

`parse()` accepts exactly one JSON text with nothing but whitespace around it, and raises a `Parse_error` whose kind
says what went wrong: `Lexical` for bytes no token covers, a control character inside a string or an ill-formed UTF-8
sequence; `Unexpected_token` for a token out of place, trailing text included; `Unexpected_end` for an input that
ends inside a value; `Invalid_literal` for a `\u` escape that leaves a surrogate unpaired. Numbers are kept as the
document spelled them; `Number::to_double()` gives the nearest double, an infinity past the double range.

## **Parsing the C-like grammar**

```cpp
#include <hopper/clike/parser.hpp>

hopper::clike::Parser parser{std::string{"int total = (a << 2) + b[i] * -c;"}};

const auto statement{parser.parse_statement()};
```

`parse_expression()`, `parse_statement()` and `parse_translation_unit()` each parse the whole input as one construct
and refuse anything left over. The trees are plain structs under `hopper::clike::ast`, one `std::variant` per node
family, each node carrying its span.

## **Writing a grammar on the kit**

A parser derives from `parse::Parser_base<Kind>` over its own token kind, builds a `parse::Token_reader<Kind>` from a
munch lexer and a trivia predicate, and writes its productions as methods:

```cpp
class Parser : public hopper::parse::Parser_base<Token_kind>
{
public:
    explicit Parser(const std::string& input)
        : Parser_base{lexer(), input, is_trivia}
    {}

    Node parse_pair()
    {
        const auto begin{mark()};                     // where this node starts
        const auto key{expect(Token_kind::Name, "a name")};
        expect(Token_kind::Equals, "'=' after the name");
        const auto value{expect(Token_kind::Number, "a value")};
        return {.key = key.lexeme(), .value = value.lexeme(), .span = span_from(begin)};
    }
};
```

The base's `check(kind)`, `accept(kind)` and `expect(kind, what)` look at the next token; `syntax_error(message,
token)`, `eof_error(message)` and `lexical_error(message)` raise the three error kinds with the right span. Both
shipped grammars are written this way and are the reference for the style.

## **Editing a JSON text**

`json::Document` holds a text and its token stream, whitespace included, and keeps the stream current across edits by
relexing between certified positions rather than from the start:

```cpp
#include <hopper/json/document.hpp>

hopper::json::Document document{R"({"a": [1, 2, 3], "b": "text"})"};

const auto relex{document.edit(10, 1, "22")};    // replace one byte at offset 10 with "22"

// relex.rescanned is the bytes reread, a handful here; relex.whole is false unless the document had to start over.
// document.tokens() now equals the stream of the edited text tokenized whole.
```

The scan restarts at the last certified token start whose evidence the edit left untouched, since munch's certificate
promises a boundary there in every completely tokenizable text agreeing on that evidence, and stops at the first
boundary after the edit that the old stream also had. An edit that leaves the text incompletely tokenizable relexes
the whole text, and so does every edit through the one that repairs it. The saving on a real document is measured by
`tools/probes/hopper_edit_relex` and quoted in [docs/design.md](design.md).

## **Error Recovery**

After a `Parse_error` of kind `Lexical`, the stream stands at the failing byte with nothing buffered, and
`recover()` asks munch for the next certified token start:

```cpp
try
{
    document = parser.parse();
}
catch (const hopper::parse::Parse_error& error)
{
    if (error.kind() == hopper::parse::Parse_error_kind::Lexical)
    {
        if (const auto start{parser.recover()})
        {
            // The stream now stands at start->start, a token start in every completely tokenizable repair of the
            // text before start->evidence_begin; what to parse from here is the grammar's decision.
        }
    }
}
```

The answer is munch's `Certified_start`, position and evidence interval, and the guarantee is exactly munch's; hopper
adds the location bookkeeping so spans after the skip stay right, and refuses the call when a token is buffered.
