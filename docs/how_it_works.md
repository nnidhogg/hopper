# **How It Works**

The layers from munch's automaton to a grammar's tree, and where each lives in the tree. [design.md](design.md) says why
the kit is shaped as it is and [usage.md](usage.md) how it is used.

## **The Layers**

```
munch::core::Lexer            the automaton; every token hopper sees comes from here
        |
parse::Token_reader<Kind>     one-token lookahead, trivia discarded, locations tracked
        |
parse::Parser_base<Kind>      peek / accept / expect, spans, errors, recover()
        |
json::Parser   clike::Parser  the grammars, each a class of productions
        |
json::Value    clike::ast     the trees, every node with its span
```

## **Directory Structure**

```
docs/                     The documentation, one page per subject as the README's index lists them.
libs/
  parse/                  The kit: Token_reader, Token_lookahead, Token_location, Source_span, Parse_error,
                          Parser_base.
  json/                   The JSON grammar: tokens over munch, the explicit-stack Parser, the Value tree, the
                          edit-relexing Document; the conformance suite under tests/data.
  clike/                  The C-like study grammar: the campaign's tokens, the Parser with its operator fusion, the
                          ast structs.
tools/
  probes/                 hopper_edit_relex, the edit theorem run as a program over a generated corpus or a file.
external/
  munch/                  The lexer library, as a submodule pinned to a release.
  googletest/             The test framework.
```
