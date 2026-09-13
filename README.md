# **Hopper**

<p align="center">
  <img src="https://img.shields.io/badge/C%2B%2B-23-blue.svg" alt="C++23">
  <img src="https://github.com/nnidhogg/hopper/actions/workflows/ci.yml/badge.svg" alt="CI">
  <img src="https://github.com/nnidhogg/hopper/actions/workflows/codeql.yml/badge.svg" alt="CodeQL">
  <img src="https://codecov.io/gh/nnidhogg/hopper/branch/master/graph/badge.svg" alt="Coverage">
  <img src="https://img.shields.io/github/license/nnidhogg/hopper" alt="License">
</p>

`hopper` is a **C++23 library** for building **recursive-descent parsers** on top of
**[`munch`](https://github.com/nnidhogg/munch)** lexers. It supplies the parts every hand-written parser repeats and
nothing else: a **token stream with one token of lookahead** that discards trivia, **source tracking** so every node
carries the span it was parsed from with offsets indexing the original bytes, **structured errors** with a kind and
the span they point at, and **certified recovery** after a lexical error, inherited from munch under munch's own
contract. A grammar is a class that derives from the kit and writes its productions as methods; there is no grammar
language, no generated code and no runtime table.

Two grammars ship on the kit. **JSON**, complete to RFC 8259 and held to every accepted and rejected case of the
JSONTestSuite, parsed with an explicit stack so nesting depth is bounded by memory and not by the call stack. And the
**C-like study grammar**, the token set of one row of munch's recovery-quality campaign, with a parser that reads
keywords and multi-byte operators out of the campaign's coarse tokens the way a C lexer would have, so a parser over the
measured grammar exists beside the measurements.

## **Status: pre-1.0**

The kit is stable in shape and used by both grammars; the public names may still change before 1.0, after which the
versioning rule is munch's, additive within a major version. What is not here yet is the parser-level half of
certified resumption: after a lexical error the kit moves the stream to munch's next certified token start, and what
a parser may assume about its own state at that point is the open question the JSON grammar was chosen to ask; see
[docs/design.md](docs/design.md).

## **Features**

- **A token reader over any munch lexer.** `parse::Token_reader<Kind>` wraps a `munch::core::Lexer` with one token of
  lookahead, a skip predicate for trivia, and locations that count `"\r\n"` and a lone `'\r'` as one newline each while
  offsets index the original bytes.
- **A parser base with the operations a recursive-descent parser repeats.** `parse::Parser_base<Kind>` gives peek,
  check, accept and expect over token kinds, `mark()` and `span_from()` to close a node's span, and three error raisers
  whose messages name what was expected.
- **Structured errors.** `parse::Parse_error` carries a kind, lexical, an unexpected token, an unexpected end, or an
  invalid literal, and the source span it points at, with line, column and byte offset.
- **Certified recovery.** `Parser_base::recover()` moves the stream past a lexical error to the next token start munch
  certifies, under complete-repair invariance: in every completely tokenizable repair of the text before the returned
  evidence, the answer begins a token. No repair is promised to exist, the next read may error again, and a call with a
  token buffered throws rather than drop it.
- **JSON.** `json::Parser` parses one RFC 8259 text into a `json::Value` tree: null, booleans, numbers kept as spelled
  with a conversion to double on request, strings unescaped to UTF-8 with surrogate pairs combined, arrays, and objects
  that keep members in document order with duplicates and answer a lookup with the last member of a name. The lexer's
  string interior is built from munch's UTF-8 code point ranges, so a string that is not well-formed UTF-8 never
  tokenizes.
- **Edits that relex only what they reach.** `json::Document` keeps a text and its token stream and brings the stream
  current after an edit by rescanning from the last certified token start before it to the first boundary after it that
  the old stream shared, the edit theorem of the certified-splitting report as a type; every edit reports how many bytes
  it reread.
- **The C-like study grammar.** `clike::Parser` parses expressions, statements and translation units over the campaign's
  seven token kinds; its language is decimal integers, strings without escapes, booleans, the C operator ladder with
  assignment and the ternary, calls, subscripts, member access, the four named casts, the fundamental types with
  `const`, pointers and references, and `if`, `while`, `for`, `do`, `return`, blocks and declarations.

## **A First Parser**

A complete program: a JSON text parsed into a tree, two members looked up, and one value's source span read back. It
prints `hopper has 2 tags` and `stable spans bytes 56 to 61`:

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

Parsing the C-like grammar, writing a grammar of your own on the kit, editing a JSON text with the stream kept current,
and recovering after a lexical error are documented in [docs/usage.md](docs/usage.md).

## **Getting Started**

### **Requirements**

- A C++23 compiler; GCC and Clang on Linux are the toolchains built and tested (GCC 13.3 and Clang 19 in CI), on
  x86-64 and 64-bit ARM. Clang 18 and older cannot compile munch's tokenizer, which every hopper parser reads through.
- CMake 3.20+.
- munch as the git submodule under `external/munch`, checked out at the release hopper builds against; googletest
  beside it for the tests. Everything else is the standard library.

### **Building the Project**

```bash
git clone --recurse-submodules https://github.com/nnidhogg/hopper
cd hopper
cmake -S . -B build
cmake --build build -j 8
```

The default `CMAKE_BUILD_TYPE` is `Release` when unset.

## **Documentation**

The library is documented in `docs/`, one page per subject; the README is the entry.

- [docs/usage.md](docs/usage.md): consuming hopper from CMake; parsing JSON and the C-like study grammar; writing a
  grammar of your own on the kit; editing a JSON text with the token stream kept current; recovering after a lexical
  error.
- [docs/how_it_works.md](docs/how_it_works.md): the layers from munch's automaton to a grammar's tree, and where each
  lives in the tree.
- [docs/design.md](docs/design.md): the decisions behind the kit and the two grammars, the edit theorem as a type with
  its measured figures, and the open question the JSON grammar was chosen to ask.

## **Testing**

Each library under `libs/` has a GoogleTest suite in a `tests/` subdirectory, registered with CTest:

```bash
cd build
ctest --output-on-failure
```

The JSON suite includes the 318 parsing cases of the JSONTestSuite, vendored under
`libs/json/tests/data/JSONTestSuite/` with their licence and provenance: every `y_` case must parse, every `n_` case
must be refused, and what the parser does on the `i_` cases is asserted rather than left to drift. Tests and
warnings-as-errors are enabled by default only when hopper is the top-level project; a build consuming hopper through
`add_subdirectory` opts in with `-DHOPPER_BUILD_TESTS=ON` or `-DHOPPER_WERROR=ON`. The probe under `tools/probes/`
is a self-checking executable registered with CTest as well; given a JSON file it prints the edit figures instead.

## **Versioning and Stability**

hopper is pre-1.0: the kit's public names, `parse::Token_reader`, `parse::Parser_base`, `parse::Parse_error`,
`parse::Source_span` and their members, and the two grammars' `Parser` and tree types may still change before 1.0.
From 1.0 the rule is munch's: a minor release adds and never removes or renames on the stable surface named here,
and a major release is the only place a name disappears. The munch submodule is pinned to a release, and a hopper
release names the munch release it was built and tested against.

## **License**

MIT, see [LICENSE](LICENSE). The vendored JSONTestSuite cases are MIT as well; their notice is in
[THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md).

## **Author**

Developed and maintained by **Nicklas Nidhögg** GitHub: [nnidhogg](https://github.com/nnidhogg)
