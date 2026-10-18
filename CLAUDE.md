# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this is

Nepa is a compiled programming language for Nepali developers, mixing romanized Nepali
control-flow/keyword vocabulary with English technical terms (e.g. `vana` = print,
`yedi`/`natara` = if/else, `kaam` = function, `farkau` = return). The compiler/interpreter
is implemented in C (C11), built incrementally, milestone by milestone.

**Current status: Milestone 2 (Variables) is complete.** A program is a sequence of
newline-terminated statements: `vana(expr)` (print) and `name = expr` (assignment, which
creates the variable on first use — no declaration keyword, per `docs/prompt.txt`). Values
are integers only; `+ - * / %`, parentheses, unary `+`/`-`, and variable references.
There are no strings, conditionals, loops, user-defined functions, arrays, or classes yet.

## Commands

```bash
make          # build bin/nepa
make debug    # build with -fsanitize=address,undefined (use this while developing)
make test     # build, then run tests/run_tests.sh
make clean    # remove bin/

./bin/nepa examples/hello_calc.nepa   # run a program
./bin/nepa path/to/file.nepa          # run any .nepa source file
```

Run a single test case manually (bypassing the harness):
```bash
./bin/nepa tests/calculator/basic.nepa
```

There is no test framework beyond `tests/run_tests.sh`, a plain bash script:
- Every subdirectory of `tests/` is a suite (`calculator/`, `variables/`, ...).
- Any `tests/SUITE/NAME.nepa` with a matching `NAME.expected` is run and its stdout diffed
  against the `.expected` file.
- Any `tests/SUITE/NAME.error.nepa` is expected to exit non-zero (syntax/runtime errors);
  there is no matching `.expected` file for these.
- To add a test: drop a new `.nepa`/`.expected` pair (or `.error.nepa` for a failure case) into
  a suite directory; a new milestone gets a new directory — no registration needed.

## Architecture

Straight-line pipeline, no bytecode/VM yet (that's a later milestone):

```
source file → Lexer → Token stream → Parser (recursive descent) → AST → Interpreter (tree-walk) → stdout
```

- `src/token.h` — `Token`/`TokenType`. Tokens hold `start`/`length` slices into the original
  source buffer rather than copied strings, to avoid per-token allocation.
- `src/lexer.c` — hand-written scanner, one token at a time via `lexer_next_token()`. Tracks
  line/col for diagnostics. `#` starts a line comment.
  Newlines are significant: `\n` is emitted as `TOKEN_NEWLINE` (statement terminator), so an
  expression cannot currently span lines. `vana` is lexed as the keyword `TOKEN_VANA`; new
  keywords should be added the same way in `ident_token()`.
- `src/parser.c` — recursive descent. Operator precedence is encoded by the *call chain*, not
  a precedence table: `parse_expr` (handles `+ -`) calls `parse_term` (handles `* / %`) calls
  `parse_factor` (numbers, parens, unary sign). When adding new operators, insert a new
  precedence level as a new function in this chain rather than adding a table.
- `src/ast.h`/`ast.c` — tagged-union AST nodes (`AST_NUMBER`, `AST_BINOP`, `AST_UNARY`,
  `AST_VARIABLE`, `AST_ASSIGN`, `AST_PRINT`, and the root `AST_PROGRAM` holding a growable
  statement array). Variable/assign nodes keep their name `Token` for error locations. `ast_free` recursively frees a tree; every node allocated by `ast_new_*` must
  eventually pass through it.
- `src/interpreter.c` — tree-walking evaluator: `execute` runs statements, `eval` evaluates
  expressions, directly on the AST (no IR). Variables live in a flat, linearly-searched global
  list (names are source slices) — deliberately simple; the hashed, scoped symbol table is its
  own later milestone. Undefined variables are reported via `report_error` with a caret;
  division/modulo by zero print a message and `exit(1)`.
- `src/diagnostics.c` — single shared `report_error()` used by the parser (and meant to be
  reused by future phases) to print beginner-friendly errors: file/line/column, the offending
  source line, and a `^` caret under the exact column, per the project's error-message
  philosophy (clear, located, non-cryptic — see also `--lang=ne`/`--lang=en` as a planned
  future CLI option for localized error text).
- `src/main.c` — wires the pipeline together: read file → init lexer → init parser → parse →
  bail out non-zero if `parser.had_error` → interpret → free.

`vana(...)` is currently special-cased as a statement in the parser's `parse_statement()` (it is
*not* a general function call) — general function calls arrive in the Functions milestone.

## Development approach for this project

This codebase is being built as a strict milestone sequence (see `README.md` roadmap:
variables → conditionals → loops → functions → arrays → structs/classes → symbol table →
type checker → modules → bytecode VM → IR → native codegen). When extending it:

- Implement one milestone/subsystem at a time; do not add features from a later milestone
  early (e.g. don't add floats or general function calls while working on variables).
- Every milestone must compile cleanly (`-Wall -Wextra -Wpedantic`, zero warnings) and pass
  `make test` before moving on. Add test cases under `tests/` for every new feature, including
  at least one `.error.nepa` case where relevant.
- Build with `make debug` (ASan/UBSan) while developing; the interpreter does manual
  `malloc`/`free` (see `ast_free`), so sanitizer runs are the main safety net.
- Keep new language keywords consistent with the project's Nepali/English mix — Nepali for
  control flow and everyday verbs (`yedi`, `natara`, `jabasamma`, `ko_lagi`, `kaam`, `farkau`,
  `banau`), English for more technical/structural concepts (`class`, `interface`, `import`).
  Don't blindly translate every keyword to Nepali.
