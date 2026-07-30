# **Hopper**

<p align="center">
  <img src="https://img.shields.io/badge/C%2B%2B-23-blue.svg" alt="C++23">
  <img src="https://github.com/nnidhogg/hopper/actions/workflows/ci.yml/badge.svg" alt="CI">
  <img src="https://github.com/nnidhogg/hopper/actions/workflows/codeql.yml/badge.svg" alt="CodeQL">
  <img src="https://codecov.io/gh/nnidhogg/hopper/branch/master/graph/badge.svg" alt="Coverage">
  <img src="https://img.shields.io/github/license/nnidhogg/hopper" alt="License">
  <img src="https://img.shields.io/github/v/release/nnidhogg/hopper?include_prereleases&sort=semver" alt="Release">
</p>

`hopper` is a **lightweight C++23 library** for building **recursive-descent parsers**. It works seamlessly with the
**[`munch`](https://github.com/nnidhogg/munch)** project and provides a **token stream with lookahead**, **source
tracking**, and **error reporting**. The design focuses on **clarity**, **deterministic control flow**, and a small,
predictable API, making it easy to implement **LL(1)-style grammars** for DSLs, configuration formats, or full language
front-ends.

The name honors the painter [Edward Hopper](https://en.wikipedia.org/wiki/Edward_Hopper); that a recursive-descent
parser hops from token to token is the pun.

## **Status: Work in Progress**

This parsing library is actively developed and not yet feature-complete. The core components: **token streaming**,
**lookahead**, **source location tracking**, and **structured error reporting** are stable, but higher-level
abstractions are still evolving.

As the initial application of the library, work is underway on **`libs/cpp`**: a recursive-descent, precedence-climbing
parser for a subset of C++ **expressions, statements, and translation units**. Expressions cover literals, identifiers, parenthesized
subexpressions, unary operators (`+`, `-`, `!`, `~`, prefix `++`/`--`, address-of `&`, dereference `*`), postfix
operators (calls, `.`, `->`, `[]`, `++`, `--`), the
standard left-associative binary precedence ladder (multiplicative, additive, shift, relational, equality, bitwise-and,
bitwise-xor, bitwise-or, logical-and, logical-or), and the right-associative ternary conditional and assignment
(plain `=` and the compound arithmetic/bitwise operators). Statements cover the expression statement, the empty
statement, compound `{}` blocks, `if`/`else` with the dangling `else` binding to the nearest `if`, `while`, `for`
with a declaration, expression, or empty init-statement, `do`/`while`, `return`, and declarations: a possibly
const-qualified fundamental type followed by comma-separated pointer/reference declarators with optional
initializers. Translation units parse as a sequence of function definitions, function prototypes, and variable
declarations, with parameters carrying the same type and declarator shapes plus optional defaults. Casts are not
covered yet.

This serves as both a **reference implementation** and a **validation** of the library's design and usability.

Breaking changes may occur while the API is being refined.

## **Architecture Overview**

| Module        | Responsibility                                                                                          |
|---------------|----------------------------------------------------------------------------------------------------------|
| `hopper::parse` | The generic toolkit: `Token_reader` (one-token lookahead over a munch lexer, trivia skipping, newline normalization, line/column tracking) and `Parser_base` (the LL(1) primitives: `peek`, `check`, `accept`, `expect`, structured errors). |
| `hopper::cpp`   | The C++ front end built on the toolkit: a munch token set, plain-struct variant ASTs (`ast::Expr`, `ast::Stmt`, `ast::Translation_unit`), and a `Parser` with one implementation file per grammar area. |

The split mirrors the library's purpose: everything a recursive-descent parser needs regardless of language lives in
`libs/parse`, and everything specific to the C++ subset lives in `libs/cpp` as the reference consumer. A new language
front end starts from `Token_reader` and `Parser_base` and brings only its token set, its AST, and its grammar
functions.

## **Usage**

```cpp
#include <hopper/cpp/lexer.hpp>
#include <hopper/cpp/parser.hpp>

using namespace hopper::cpp;

Parser parser{build_lexer(), std::string{"int add(int a, int b) { return a + b; }"}};

const auto unit{parser.parse_translation_unit()};

// unit.items holds one ast::Function; walk the variant ASTs with std::visit.
```

`Parser` also accepts a `std::filesystem::path` to parse a file, and exposes `parse_expression()` and
`parse_statement()` for smaller entry points. All three throw `std::runtime_error` with a position-bearing message on
lexical or syntax errors.

## **Building and Testing**

```bash
git clone --recurse-submodules https://github.com/nnidhogg/hopper.git
cmake -S hopper -B hopper/build
cmake --build hopper/build -j 8
cd hopper/build && ctest --output-on-failure
```

munch is vendored as a submodule under `external/munch`; everything else (googletest) is fetched by CMake. Every
library has its own test suite, registered with CTest, and CI builds with GCC and Clang 19, enforces clang-format 19,
and reports coverage.

## **License**

This project is licensed under the terms of the MIT License. See the [LICENSE](LICENSE) file for details.

## **Author**

Developed and maintained by **Nicklas Nidhögg**  
GitHub: [nnidhogg](https://github.com/nnidhogg)
