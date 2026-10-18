# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this is

NepLang is a compiled programming language for Nepali developers, mixing romanized Nepali
control-flow/keyword vocabulary with English technical terms (e.g. `vana` = print,
`yedi`/`natara` = if/else, `kaam` = function, `farkau` = return). The compiler/interpreter
is implemented in C (C11), built incrementally, milestone by milestone.

**Current status: Milestone 6 (Arrays) is complete.** A program is a sequence of
newline-terminated statements: `vana(expr)` (print), `name = expr` (assignment, which
creates the variable on first use — no declaration keyword, per `docs/prompt.txt`),
`yedi cond { } natara yedi cond { } natara { }`, `jabasamma cond { }` (while),
`ko_lagi i in a..b { }` (for; end-exclusive, bounds evaluated once), `kaam name(a, b) { }`
(top-level only), `farkau [expr]`, call statements `name(args)`, and `a[i] = v`. Values are
integers and arrays (`[1, 2]`, `a[i]`, built-in `lambai(a)` for length, `ko_lagi x in a`).
Comparisons `== != < <= > >=` work on numbers only, yield 1/0, and any non-zero number is
truthy (there is no bool type yet). There are no strings, booleans, logical operators,
break/continue, push/pop, or classes yet.

## Commands

```bash
make          # build bin/neplang
make debug    # build with -fsanitize=address,undefined (use this while developing)
make test     # build, then run tests/run_tests.sh
make clean    # remove bin/

./bin/neplang examples/hello_calc.np   # run a program
./bin/neplang path/to/file.np          # run any .np source file
```

Run a single test case manually (bypassing the harness):
```bash
./bin/neplang tests/calculator/basic.np
```

There is no test framework beyond `tests/run_tests.sh`, a plain bash script:
- Every subdirectory of `tests/` is a suite (`calculator/`, `variables/`, ...).
- Any `tests/SUITE/NAME.np` with a matching `NAME.expected` is run and its stdout diffed
  against the `.expected` file.
- Any `tests/SUITE/NAME.error.np` is expected to exit non-zero (syntax/runtime errors);
  there is no matching `.expected` file for these.
- To add a test: drop a new `.np`/`.expected` pair (or `.error.np` for a failure case) into
  a suite directory; a new milestone gets a new directory — no registration needed.

## Architecture

Straight-line pipeline, no bytecode/VM yet (that's a later milestone):

```
source file → Lexer → Token stream → Parser (recursive descent) → AST → Interpreter (tree-walk) → stdout
```

- `src/value.h`/`value.c` — runtime `Value` (tagged union: `VAL_INT` or `VAL_ARRAY`).
  Arrays are heap-allocated and **reference-counted** with shared (Python/Java-style)
  semantics. Ownership rule used throughout the interpreter: a function returning a `Value`
  hands over one reference; whoever stores or drops it must `value_release()` it
  (`value_retain()` to take an extra one). Self-containing arrays leak (no cycle collector).
- `src/token.h` — `Token`/`TokenType`. Tokens hold `start`/`length` slices into the original
  source buffer rather than copied strings, to avoid per-token allocation.
- `src/lexer.c` — hand-written scanner, one token at a time via `lexer_next_token()`. Tracks
  line/col for diagnostics. `#` starts a line comment.
  Newlines are significant: `\n` is emitted as `TOKEN_NEWLINE` (statement terminator), so an
  expression cannot currently span lines. Keywords (`vana`, `yedi`, `natara`, `jabasamma`,
  `ko_lagi`, `in`, `kaam`, `farkau`) get their own token types via the `keywords[]` table in `lexer.c` — add new
  keywords there.
- `src/parser.c` — recursive descent. Operator precedence is encoded by the *call chain*, not
  a precedence table: `parse_comparison` (non-associative `== != < <= > >=`; the entry point
  for full expressions) calls `parse_expr` (handles `+ -`) calls `parse_term` (handles `* / %`)
  calls `parse_factor` (unary sign, then `parse_postfix(parse_primary)`: numbers, variables,
  calls, `[...]` array literals, parens, followed by any number of `[index]`). When adding new operators,
  insert a new precedence level as a new function in this chain rather than adding a table.
  `ko_lagi` range bounds use `parse_expr`, not `parse_comparison`; `ko_lagi x in EXPR {` with
  no `..` is a for-each over an array. Newlines are skipped inside `[ ]` array literals only.
  Statements: `parse_statements(parser, block, end)` is shared by the top level (`end` =
  `TOKEN_EOF`) and `{ }` blocks (`end` = `TOKEN_RBRACE`); a statement must be followed by a
  newline or the block's closing `}`. `natara` must follow `}` on the same line, since the
  parser has a single token of lookahead and newlines are tokens. `reject_keyword_as_name()`
  gives the "reserved keyword" error for things like `yedi = 5`. Only the first error is
  reported (`had_error` stops parsing). `advance()` reports `TOKEN_ERROR` (bad/non-ASCII
  characters) immediately. `parser->block_depth` rejects `kaam` inside any block;
  `parser->in_function` rejects `farkau` outside a function. An identifier followed by `(`
  is a call (`parse_call`), both in `parse_factor` and as a statement.
- `src/ast.h`/`ast.c` — every node has a `token` (source location for runtime errors; zeroed
  for nodes without one, e.g. `ast_new_placeholder()` used after parse errors). Tagged-union
  AST nodes (`AST_NUMBER`, `AST_BINOP`, `AST_UNARY`,
  `AST_VARIABLE`, `AST_ASSIGN`, `AST_PRINT`, `AST_IF`, `AST_WHILE`, `AST_FOR`,
  `AST_FOR_EACH`, `AST_ARRAY`, `AST_INDEX`, `AST_INDEX_ASSIGN` (target is an `AST_INDEX`),
  `AST_FUNCTION`, `AST_CALL`, `AST_RETURN` (value may be NULL), and `AST_BLOCK` — a growable statement array used both for `{ }` bodies and as the program
  root). `binop.op` is a `TokenType` (two-char operators don't fit a `char`); `unary.op` is
  still a `char`. Growable arrays (block statements, params, call args) share `grow_array()`
  in `ast.c`. An `else_branch` is either a block or a nested `AST_IF` (for `natara yedi`). Variable/assign nodes keep their name `Token` for error locations. `ast_free` recursively frees a tree; every node allocated by `ast_new_*` must
  eventually pass through it.
- `src/interpreter.c` — tree-walking evaluator: `execute` runs statements, `eval` evaluates
  expressions, directly on the AST (no IR). Variables live in flat, linearly-searched
  `Scope`s (names are source slices) — the hashed symbol table is its own later milestone.
  `interp->locals` is the current call's scope (NULL at top level): reads check locals then
  globals; assignment inside a function always writes a local (spec §12 — functions never
  overwrite globals). No block scope: `ko_lagi` loop variables persist after the loop.
  Functions are registered in a pre-pass over top-level statements before execution (so
  call-before-define and mutual recursion work; duplicate names are reported there).
  Functions and variables are separate namespaces. `farkau` sets `interp->returning`, which
  makes blocks/loops stop until `call_function` clears it; no `farkau` → returns 0. Call
  depth is capped at `MAX_CALL_DEPTH` with a friendly error. `eval` returns an owned `Value`;
  `eval_int` / `eval_condition` enforce "must be a number". Built-ins (`lambai`) are dispatched
  in `call_function` via `is_builtin()` and can't be redefined. All runtime errors go through
  `runtime_error(interp, node->token, msg)` (→ `report_error` with caret + `exit(1)`).
- `src/diagnostics.c` — single shared `report_error()` used by the parser (and meant to be
  reused by future phases) to print beginner-friendly errors: file/line/column, the offending
  source line, and a `^` caret under the exact column, per the project's error-message
  philosophy (clear, located, non-cryptic — see also `--lang=ne`/`--lang=en` as a planned
  future CLI option for localized error text).
- `src/main.c` — wires the pipeline together: read file → init lexer → init parser → parse →
  bail out non-zero if `parser.had_error` → interpret → free.

`vana(...)` is still special-cased as a statement keyword in `parse_print()`, not a builtin
function — it can't be used inside an expression or redefined.

## Development approach for this project

This codebase is being built as a strict milestone sequence (see `README.md` roadmap:
variables → conditionals → loops → functions → arrays → structs/classes → symbol table →
type checker → modules → bytecode VM → IR → native codegen). When extending it:

- Implement one milestone/subsystem at a time; do not add features from a later milestone
  early (e.g. don't add floats or general function calls while working on variables).
- Every milestone must compile cleanly (`-Wall -Wextra -Wpedantic`, zero warnings) and pass
  `make test` before moving on. Add test cases under `tests/` for every new feature, including
  at least one `.error.np` case where relevant.
- Build with `make debug` (ASan/UBSan) while developing; the interpreter does manual
  `malloc`/`free` (see `ast_free`), so sanitizer runs are the main safety net.
- Keep new language keywords consistent with the project's Nepali/English mix — Nepali for
  control flow and everyday verbs (`yedi`, `natara`, `jabasamma`, `ko_lagi`, `kaam`, `farkau`,
  `banau`), English for more technical/structural concepts (`class`, `interface`, `import`).
  Don't blindly translate every keyword to Nepali.
