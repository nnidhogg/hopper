# JSONTestSuite, the parsing cases

The 318 files under `test_parsing/` are the parsing cases of Nicolas Seriot's JSONTestSuite
(https://github.com/nst/JSONTestSuite), copied unchanged from commit `1ef36fa01286573e846ac449e8683f8833c5b26a`, whose
archive `codeload.github.com/nst/JSONTestSuite/zip/<commit>` has SHA-256 `5b205e1f95331234` as its first sixteen hex
digits at the time of copying, 2026-09-06. The suite is MIT licensed; the licence is beside this file.

The prefix of each file name is its verdict: `y_` must be accepted, `n_` must be rejected, and `i_` is left to the
implementation. The conformance test holds the parser to every `y_` and `n_` case and records what it does on the `i_`
cases without judging them.
