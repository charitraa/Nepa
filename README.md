# Nepa

A compiled programming language for Nepali developers, mixing romanized Nepali
control-flow words with English technical terms. Implemented in C.

## Status: Milestone 2 — Variables

Supported today:

```
x = 10
y = 20

vana(x + y)
```

runs and prints:

```
30
```

- Integer arithmetic: `+ - * / %`, parentheses, unary `+`/`-`
- Variables: `name = expr` creates a variable on first assignment and
  updates it afterwards; using a variable before assigning it is an error
- One statement per line; `#` starts a comment

Nothing else yet — no strings, conditionals, loops, functions, or
classes. Those are later milestones.

## Build

```bash
make          # optimized-ish debug build
make debug    # build with ASan/UBSan for development
make test     # build and run every suite under tests/
```

## Run

```bash
./bin/nepa examples/hello_calc.nepa
./bin/nepa examples/variables.nepa
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
