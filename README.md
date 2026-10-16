# Nepa

A compiled programming language for Nepali developers, mixing romanized Nepali
control-flow words with English technical terms. Implemented in C.

## Status: Milestone 1 — Calculator

Supported today:

```
vana(10 + 20 * 3)
```

runs and prints:

```
70
```

Operators: `+ - * / %`, parentheses, unary `+`/`-`. Nothing else yet —
no variables, conditionals, loops, functions, or classes. Those are
later milestones.

## Build

```bash
make          # optimized-ish debug build
make debug    # build with ASan/UBSan for development
make test     # build and run tests/calculator
```

## Run

```bash
./bin/nepa examples/hello_calc.nepa
```

## Project layout

```
src/          lexer, parser, AST, interpreter, diagnostics, main
tests/        per-milestone test cases (input + expected output)
examples/     example .nepa programs
docs/         language grammar notes (added as the grammar grows)
```

## Roadmap

See project milestones: variables → conditionals → loops → functions →
arrays → structs/classes → symbol table → type checker → modules →
bytecode VM → IR → native code generation.
# Nepa
