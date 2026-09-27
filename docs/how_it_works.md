# **How It Works**

What happens between munch's automaton and a grammar's tree, and where each piece lives in the tree.
[design.md](design.md) says why the kit is shaped as it is and [usage.md](usage.md) how it is used.

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
json::Value    clike::ast     the trees, with the spans the grammars give them
```

## **A Token's Path**

The reader asks munch's tokenizer for the next token and holds at most one: `peek()` fills that one place and `next()`
empties it. A token the skip predicate names as trivia never takes the place; the reader moves its cursor over the
token's bytes and asks again, so a grammar sees only the tokens it parses. A lexical error comes back as the tokenizer's
error with nothing buffered, and the stream stands at the failure.

The buffered token's text is a view of the reader's input. The reader rebuilds that view from its own input whenever it
hands the token out, so a reader copied or moved while it holds a token reads its own copy of the bytes.

## **Positions and Spans**

A `Token_location` counts a line, a column and a byte offset as it is advanced over consumed text. A `"\n"`, a `"\r\n"`
pair and a lone `'\r'` each end one line, and a pair split between two tokens still ends one, so the count does not
depend on how a lexer divides the input. Offsets always index the original bytes.

The reader keeps three positions: where the buffered token begins, where its cursor stands, and where the last token a
parser consumed ended. Trivia moves the cursor and leaves the end of the last consumed token where that token ended. A
production calls `mark()` before its first token, which gives the start of the next token, and `span_from(begin)` after
its last, which closes the span at the end of the last consumed token, so the trivia after a construct never enters its
span.

## **Errors and Recovery**

A parser fails fast: every error is a `parse::Parse_error` with a kind and a span, and nothing is repaired or guessed.

- **A token out of place** raises `Unexpected_token` at the offending token's span, quoting that token.
- **An input that ends early** raises `Unexpected_end` with an empty span just after the last consumed token.
- **Bytes that do not tokenize** raise `Lexical` with an empty span where tokenization stopped, carrying the tokenizer's
  message.
- **A literal the language rejects**, a lone surrogate escape in JSON or an integer the platform cannot hold in the
  C-like grammar, raises `Invalid_literal` at the literal.
- **A file that cannot be opened** raises `Unreadable_file` with an empty span at the start.

After a lexical error, `recover()` asks munch for the next certified token start and moves the stream there, or refuses
and leaves the position unchanged when none lies ahead: in every completely tokenizable repair of the text before the
returned evidence, that position begins a token. The skipped bytes advance the location, so later spans stay right. With
a token buffered the stream is not at a lexical error, and the call throws `std::logic_error`.

## **The JSON Parser**

The parser keeps an explicit stack of open containers instead of recursing, so a document nests as deeply as memory
allows. Each frame holds the array or object being filled, the member name waiting for its value, and where the
container opened.

Parsing alternates two phases. When a value is due, `begin_value()` reads one: a scalar is complete at once, and a
bracket or brace pushes a frame. When a value is in hand, `place()` puts it into the frame on top and reads the
separator after it: a comma leaves the frame open, and the closing bracket pops it with `close()`, which gives the
container its span from its opening byte to its closing one. The document is finished when a value is in hand and no
frame is open. A `Value` destroys its tree the same way, moving each level's children onto a worklist rather than
recursing.

## **The Incremental Document**

A `json::Document` keeps its text and the text's whole token stream, whitespace included, as pieces of kind, offset and
length. An edit replaces a byte range in four steps.

1. **It looks for an anchor.** Before changing the text it searches backward from the edit for a certified token start
   whose evidence ends at or before the edit. The search reaches 64 bytes back, then four times as far each time it
   finds nothing, to at most 4,096 bytes, and keeps the last certified start it meets. A certificate is decided over
   every state the scan could be in when it meets its evidence, so while the evidence bytes are unchanged no token
   before the anchor can end differently.
2. **It rescans from the anchor.** The tokens before the anchor stand as they were, and the scan resumes at the anchor
   over the edited text.
3. **It rejoins the old stream.** At each new token boundary past the edit, the document looks for an old token starting
   at the same place. The first boundary both streams share ends the rescan, since from there the two scans read the
   same bytes from the same start state.
4. **It renumbers the tail.** The old tokens from the shared boundary on are kept, their offsets shifted by the edit's
   change in length, without reading their bytes again.

Where the search finds no certified start, as inside one long string, or where the edited text stops tokenizing, the
document relexes the whole text instead, and an incomplete stream relexes whole on every edit until one repairs it. An
incomplete stream holds the tokens up to the first byte that does not tokenize. Every edit reports the bytes its scan
covered and whether it started over.

## **The C-like Parser**

The C-like grammar reads the seven token kinds the certified-recovery campaign measured: identifiers, digit runs, one
operator byte, one punctuation byte, strings without escapes, line comments and whitespace, the last two discarded as
trivia. Nothing finer reaches the parser, so it reads what a C lexer would read from those tokens. A keyword is an
identifier with a reserved spelling. A multi-byte operator is a run of adjacent operator bytes that the parser fuses
into the longest spelling its operator table knows, holding the fused operator, with its span and where the input stood
before it, as its current token until a production takes it, so `a+-b` reads `a + (-b)` and `<<=` is one operator. Every
parse begins by clearing that fused operator, so nothing of an earlier input survives a `load()`, a `reset()` or a
failed parse.

Binary expressions are parsed by precedence climbing over C's ladder, one table from logical or at the loosest to the
multiplicative operators at the tightest. The rest of the grammar is recursive descent, so its nesting is bounded by the
call stack.

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
  fuzz/                   The two libFuzzer harnesses over the JSON and C-like parsers, with seeds/clike.
external/
  munch/                  The lexer library, as a submodule pinned to a release.
  googletest/             The test framework, fetched when tests are built without USE_SYSTEM_GTEST.
```
