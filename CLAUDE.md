# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this is

Nepa is a compiled programming language for Nepali developers, mixing romanized Nepali
control-flow/keyword vocabulary with English technical terms (e.g. `vana` = print,
`yedi`/`natara` = if/else, `kaam` = function, `farkau` = return). The compiler/interpreter
is implemented in C (C11), built incrementally, milestone by milestone.

**Current status: Milestone 1 (Calculator) is complete.** Only integer arithmetic through
a hardcoded `vana(expr)` print statement works — `+ - * / %`, parentheses, and unary `+`/`-`.
There are no variables, conditionals, loops, user-defined functions, arrays, or classes yet.

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
- Any `tests/calculator/NAME.nepa` with a matching `tests/calculator/NAME.expected` is run
  and its stdout diffed against the `.expected` file.
- Any `tests/calculator/NAME.error.nepa` is expected to exit non-zero (syntax/runtime errors);
  there is no matching `.expected` file for these.
- To add a test: drop a new `.nepa`/`.expected` pair (or `.error.nepa` for a failure case) into
  `tests/calculator/` — no registration needed, the script globs the directory.

## Architecture

Straight-line pipeline, no bytecode/VM yet (that's a later milestone):

```
source file → Lexer → Token stream → Parser (recursive descent) → AST → Interpreter (tree-walk) → stdout
```

- `src/token.h` — `Token`/`TokenType`. Tokens hold `start`/`length` slices into the original
  source buffer rather than copied strings, to avoid per-token allocation.
- `src/lexer.c` — hand-written scanner, one token at a time via `lexer_next_token()`. Tracks
  line/col for diagnostics. `#` starts a line comment.
- `src/parser.c` — recursive descent. Operator precedence is encoded by the *call chain*, not
  a precedence table: `parse_expr` (handles `+ -`) calls `parse_term` (handles `* / %`) calls
  `parse_factor` (numbers, parens, unary sign). When adding new operators, insert a new
  precedence level as a new function in this chain rather than adding a table.
- `src/ast.h`/`ast.c` — tagged-union AST nodes (`AST_NUMBER`, `AST_BINOP`, `AST_UNARY`,
  `AST_PRINT`). `ast_free` recursively frees a tree; every node allocated by `ast_new_*` must
  eventually pass through it.
- `src/interpreter.c` — tree-walking evaluator (`eval`), operates directly on the AST (no IR).
  Division/modulo by zero are runtime errors that print a message and `exit(1)`.
- `src/diagnostics.c` — single shared `report_error()` used by the parser (and meant to be
  reused by future phases) to print beginner-friendly errors: file/line/column, the offending
  source line, and a `^` caret under the exact column, per the project's error-message
  philosophy (clear, located, non-cryptic — see also `--lang=ne`/`--lang=en` as a planned
  future CLI option for localized error text).
- `src/main.c` — wires the pipeline together: read file → init lexer → init parser → parse →
  bail out non-zero if `parser.had_error` → interpret → free.

`vana(...)` is currently special-cased directly in the parser's `parser_parse_program()` (it is
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
