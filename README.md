# NepLang

A compiled programming language for Nepali developers, mixing romanized Nepali
control-flow words with English technical terms. Implemented in C.

## Status: Milestone 4 — Loops

Supported today:

```
ko_lagi n in 1..16 {
    yedi n % 15 == 0 {
        vana(-15)
    } natara yedi n % 3 == 0 {
        vana(-3)
    } natara {
        vana(n)
    }
}

umar = 20
jabasamma umar < 25 {
    umar = umar + 1
}
vana(umar)
```

- Integer arithmetic: `+ - * / %`, parentheses, unary `+`/`-`
- Variables: `name = expr` creates a variable on first assignment and
  updates it afterwards; using a variable before assigning it is an error
- Comparisons: `== != < <= > >=` (evaluate to `1` or `0`; can't be chained)
- `yedi cond { } natara yedi cond { } natara { }` — if / else-if / else.
  `natara` must be on the same line as the closing `}`. Any non-zero value is true.
- `jabasamma cond { }` — while loop
- `ko_lagi i in a..b { }` — counts `i` from `a` up to `b - 1` (end is exclusive,
  bounds are evaluated once)
- One statement per line; `#` starts a comment

Nothing else yet — no strings, booleans, `ra`/`wa`-style logical operators,
break/continue, functions, or classes. Those are later milestones.

## Build

```bash
make          # optimized-ish debug build
make debug    # build with ASan/UBSan for development
make test     # build and run every suite under tests/
```

## Run

```bash
./bin/neplang examples/hello_calc.np
./bin/neplang examples/variables.np
./bin/neplang examples/control_flow.np
```

## Project layout

```
src/          lexer, parser, AST, interpreter, diagnostics, main
tests/        per-milestone test cases (input + expected output)
examples/     example .np programs
docs/         language grammar notes (added as the grammar grows)
```

## Roadmap

See project milestones: variables → conditionals → loops → functions →
arrays → structs/classes → symbol table → type checker → modules →
bytecode VM → IR → native code generation.
