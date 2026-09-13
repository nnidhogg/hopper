# **Design**

The decisions behind hopper, in the order a reader meets them: the kit every parser is built on, the two grammars
that ship on it, and the resumption question the JSON grammar was chosen to ask.

## **The kit is a token stream, not a parser generator**

`hopper::parse` is a handful of headers and no framework: a `Token_reader` that turns a `munch::core::Lexer` into a
stream with one token of lookahead and discards the kinds a predicate names as trivia; a `Token_location` that counts
lines and columns over the original bytes, so every span indexes the input as given whatever its line endings; a
`Parse_error` that carries a kind and the span it points at; and a `Parser_base` with the handful of operations a
hand-written recursive-descent parser repeats, peek, accept, expect, mark a position and close a span from it. A grammar
is a class deriving from `Parser_base` and writing its productions as methods. There is no grammar description language,
no generated code and no runtime table; the productions are the documentation.

The lexer is munch's, always. hopper never tokenizes bytes itself; the two places it looks inside a token, the C-like
parser fusing adjacent operator bytes and the JSON parser resolving a string's escapes, act on tokens munch has already
cut, so everything munch proves about its lexers, the maximal-munch segmentation, the certified split points and the
certified recovery, holds unchanged under a hopper parser, and a parser inherits munch's contracts rather than restating
them.

## **Recovery is inherited, not invented**

After a lexical error `Parser_base::recover()` asks the reader, and the reader asks munch's `recover_from_failure()`:
the stream moves to the next position the lexer certifies as a token start, under munch's complete-repair invariance,
or does not move at all. The parser learns where the token stream resumes and nothing more; what it does with a
resumed stream is its own policy, and the kit refuses to guess one. A call with a token buffered is a logic error and
throws, because it means the caller is not standing at a lexical error.

## **Two grammars, chosen for what they let the kit ask**

**JSON** is the first grammar because it sits where the next question is decidable. JSON is a visibly pushdown
language: brackets and braces determine the stack, so the parser's state after any prefix is the list of open
containers, and the certified resumption question, what a parser may assume when it restarts at a certified lexical
position, has a home there. The parser is written to make that state literal: it keeps its own stack of open arrays
and objects instead of recursing, so a document nested as deep as memory allows parses without touching the call
stack, and the value tree destroys itself the same way. Every value carries its span; numbers are kept as spelled,
since RFC 8259 sets no precision; strings are unescaped to UTF-8 with surrogate pairs combined and a lone surrogate
refused as an invalid literal; objects keep their members in document order, duplicates included, and answer a
lookup with the last member of a name, as most processors do. The lexer's string interior is built from munch's
UTF-8 code point ranges, so a string whose bytes are not well-formed UTF-8 never tokenizes. The parser is held to
every accepted and rejected case of the JSONTestSuite, and its behaviour on the implementation-defined cases is
recorded as a test rather than left to drift.

**The C-like study grammar** is the second grammar because it is what the papers measured. Its token set is exactly
the recovery campaign's row "c-like conventional with strings and line comments": an identifier, a digit run, one
operator byte, one punctuation byte, a string without escapes, a line comment and a whitespace run of spaces, tabs
and newlines. Nothing finer reaches the parser, so the parser reads what a C lexer would have read: keywords are
identifiers with a reserved spelling, and a multi-byte operator is a run of adjacent operator bytes fused to the
longest spelling the operator table knows. That fusion is maximal munch applied one level up, and it decides the
same way a C lexer does: `a+-b` is `a + (-b)`, `a--b` is `(a--) b` and an error, `<<=` is one operator. The language
on top is the study grammar's: decimal integers, strings, booleans, the C operator ladder, the fundamental types
with `const`, pointers and references, and the statements a C body is made of. The grammar exists so a parser over
the measured token set is available when the resumption question moves past the visibly pushdown class, to a
grammar whose brackets determine most of the stack and not all of it.

## **An edit rescans between two certificates**

The certified-splitting report's edit theorem says where a scan may restart after a change: at the last certified token
start whose evidence the change left untouched, because the certificate promises a boundary there in every completely
tokenizable text agreeing on that evidence, the edited one included; and it may stop at the first boundary after the
change that the old segmentation also had, because from a shared boundary two scans of the same suffix agree.
`hopper::json::Document` is that theorem as a type. It keeps a text and its token stream, whitespace included, and
`edit()` replaces a byte range, searches backward within a budget of 4,096 bytes for a certified start whose evidence
the edit left untouched, rescans from there to the first shared boundary past the edit and reports how many bytes it
read. Where no certified start lies within the budget, as inside one long string, the document relexes the whole text;
the certificate carries nothing for a text that does not tokenize completely, so an edit that breaks tokenization
relexes the whole text too, and so does every edit through the one that repairs it. A document is a stream and not a
tree: what a parser may keep of its result across an edit is the resumption question below, not something the document
answers.

The saving is measured rather than assumed. `tools/probes/hopper_edit_relex` applies a fixed schedule of edits that keep
the text tokenizable, a digit changed at the end of a number, letters inserted or one deleted inside a string, a line
break after a comma, and holds the stream to a whole scan after each; without arguments it runs a generated corpus as a
test, and given a file it prints that file's figures. On twitter.json, the document simdjson ships as
`jsonexamples/twitter.json` at release v3.10.1:

```
$ ./build/tools/probes/hopper_edit_relex twitter.json
bytes 631515 tokens 84090
edits 581 whole 0 rescanned bytes mean 17.8 median 16 p90 29 max 92
```

No edit relexed the whole document, the median edit reread 16 bytes of six hundred thousand, and the worst reread 92.
What an edit rereads is the stretch from the certificate before it to the shared boundary after it; inside a string that
stretch is the string, since its interior certifies nothing. JSON certifies at nearly every structural byte, through the
two-to-four byte windows around its commas, colons and brackets, so the anchor before an edit is rarely more than a
token away. A grammar that certifies less pays the distance to its last certificate instead, which is what the
split-windows report measures grammar by grammar.

## **What the JSON grammar is for next**

The lexical papers answer where a scanner may start. A parser's question is what configuration it may assume when
it restarts at such a position. For JSON the configuration is the open-container stack plus, inside an object, the
name waiting for its value, and a resume certificate would be a position followed by enough matched structure to fix
the control state regardless of what stood before the lexical evidence. The explicit stack in `hopper::json::Parser`
is the object such a certificate speaks about; the design of the public result type of a parser-level `recover()`,
which is the smallest part and the one that decides what the theorem must deliver, is left open on purpose until the
theorem's shape is known.
