# NepLang

**NepLang** is a small programming language for Nepali developers. Its keywords are
romanized Nepali words for control flow and everyday actions (`vana` = print,
`yedi` = if, `natara` = else, `kaam` = function, `farkau` = return), mixed with plain
English for technical ideas.

```
kaam factorial(n) {
    yedi n <= 1 {
        farkau 1
    }
    farkau n * factorial(n - 1)
}

vana(factorial(6))    # 720
```

NepLang is written in C (C11) and is being built one milestone at a time. Right now it
runs programs with a tree-walking interpreter. A bytecode VM and native code generation
are planned for later.

---

## Table of contents

- [Installation](#installation)
  - [Quick install (no build needed)](#quick-install-no-build-needed)
  - [Build from source](#build-from-source)
- [Running a program](#running-a-program)
- [Language guide](#language-guide)
- [Error messages](#error-messages)
- [Keyword reference](#keyword-reference)
- [Current limitations](#current-limitations)
- [Development](#development)
- [Project layout](#project-layout)
- [Roadmap](#roadmap)

---

## Installation

### Quick install (no build needed)

Ready-made binaries for Linux, macOS and Windows are on the
[Releases page](https://github.com/charitraa/Nepa/releases). You don't need to clone the
repository or install a compiler. Run one command:

**Linux / macOS** (in a terminal):

```bash
curl -fsSL https://raw.githubusercontent.com/charitraa/Nepa/main/install.sh | sh
```

This puts `neplang` in `~/.local/bin`. If that folder isn't on your `PATH`, the
installer tells you the line to add to your `~/.bashrc` or `~/.zshrc`.

**Windows** (in PowerShell):

```powershell
irm https://raw.githubusercontent.com/charitraa/Nepa/main/install.ps1 | iex
```

This puts `neplang.exe` in `%LOCALAPPDATA%\Programs\neplang` and adds that folder to your
`PATH`. Open a **new** terminal afterwards.

Then check that it works:

```bash
neplang yourfile.np
```

**Manual download:** you can also download the file for your system from the
[latest release](https://github.com/charitraa/Nepa/releases/latest) yourself:

| System | File |
|---|---|
| Linux (Intel/AMD 64-bit) | `neplang-linux-x86_64` |
| Linux (ARM 64-bit, e.g. Raspberry Pi 4/5) | `neplang-linux-arm64` |
| macOS (Apple Silicon and Intel) | `neplang-macos-universal` |
| Windows (64-bit) | `neplang-windows-x86_64.exe` |

On Linux/macOS, rename it to `neplang`, make it executable (`chmod +x neplang`) and move it
into a folder on your `PATH`. On Windows, rename it to `neplang.exe` and put it in a folder
on your `PATH`.

To update NepLang later, run the install command again.

### Build from source

You only need this if you want to change NepLang itself, or no prebuilt binary exists for
your system. NepLang has no dependencies. You need a C compiler (`gcc` or `clang`) and,
if you want, `make`.

#### Linux

1. Install a compiler, `make` and `git`:

   ```bash
   # Debian / Ubuntu / Linux Mint
   sudo apt install build-essential git

   # Fedora
   sudo dnf install gcc make git

   # Arch / Manjaro / CachyOS
   sudo pacman -S base-devel git
   ```

2. Clone the repository and build it:

   ```bash
   git clone https://github.com/charitraa/Nepa.git neplang
   cd neplang
   make
   ```

   This creates the interpreter at `bin/neplang`.

3. Check that it works:

   ```bash
   ./bin/neplang examples/hello_calc.np
   ```

4. *(Optional)* Install it so you can run `neplang` from any folder:

   ```bash
   sudo cp bin/neplang /usr/local/bin/
   # or, without sudo:
   mkdir -p ~/.local/bin && cp bin/neplang ~/.local/bin/
   ```

   For the second option, `~/.local/bin` must be on your `PATH`.

#### macOS

Install the Xcode command-line tools, which include `clang`, `make` and `git`, then follow
the Linux steps from step 2 onward:

```bash
xcode-select --install
```

#### Windows

You can build NepLang on Windows in either of two ways.

##### Option A: MSYS2 (native Windows `.exe`, recommended)

1. Download and install MSYS2 from <https://www.msys2.org>.
2. Open the **MSYS2 UCRT64** terminal from the Start menu and install the tools:

   ```bash
   pacman -S mingw-w64-ucrt-x86_64-gcc make git
   ```

3. Clone and build:

   ```bash
   git clone https://github.com/charitraa/Nepa.git neplang
   cd neplang
   make
   ```

   This creates `bin/neplang.exe`.

4. Run a program:

   ```bash
   ./bin/neplang examples/hello_calc.np
   ```

   To use `neplang` from PowerShell or `cmd`, add the `bin` folder to your Windows
   `PATH`, or copy `bin\neplang.exe` into a folder that is already on it. If Windows says
   a DLL is missing, also add `C:\msys64\ucrt64\bin` to your `PATH`.

**No `make`?** If you already have `gcc` (for example from MinGW-w64 or MSYS2), you can
build directly from PowerShell or `cmd`:

```powershell
mkdir bin
gcc -std=c11 -Wall -Wextra -Wpedantic -O2 -o bin\neplang.exe src\lexer.c src\ast.c src\parser.c src\interpreter.c src\diagnostics.c src\main.c
bin\neplang.exe examples\hello_calc.np
```

##### Option B: WSL (Linux on Windows)

1. Open PowerShell as Administrator and run:

   ```powershell
   wsl --install
   ```

2. Restart, open the **Ubuntu** app, and follow the [Linux](#linux) build steps.

> **Note for Windows:** `make debug` uses AddressSanitizer, which MinGW does not
> support. Use WSL if you want sanitizer builds. `make test` needs `bash`, which the
> MSYS2 terminal and WSL both provide.

---

## Running a program

NepLang source files use the `.np` extension. Write a program in any text editor, for
example `namaste.np`:

```
# my first NepLang program
umar = 20
vana(umar + 5)
```

Then run it:

```bash
neplang namaste.np            # after the quick install (any OS)
./bin/neplang namaste.np      # if you built from source (Linux / macOS / MSYS2 / WSL)
bin\neplang.exe namaste.np    # if you built from source (Windows PowerShell or cmd)
```

Output:

```
25
```

The included examples:

| File | Shows |
|---|---|
| `examples/hello_calc.np` | Arithmetic and operator precedence |
| `examples/variables.np` | Variables (simple-interest calculation) |
| `examples/control_flow.np` | `yedi`, `jabasamma`, `ko_lagi` (primes below 30) |
| `examples/functions.np` | `kaam` / `farkau`, recursion (factorial, GCD) |

The example files are in the repository's
[`examples/`](https://github.com/charitraa/Nepa/tree/main/examples) folder. Download one,
or copy its code into your own `.np` file.

---

## Language guide

### Comments and statements

Each statement goes on its own line. `#` starts a comment that runs to the end of the line.

```
vana(1)    # this is a comment
vana(2)
```

### Printing: `vana`

`vana(expression)` prints the value followed by a newline.

```
vana(10 + 20 * 3)    # 70
```

### Numbers and arithmetic

Every value is currently an **integer**.

| Operator | Meaning | Example | Result |
|---|---|---|---|
| `+` `-` | add, subtract | `7 - 2` | `5` |
| `*` `/` | multiply, integer divide | `7 / 2` | `3` |
| `%` | remainder | `7 % 2` | `1` |
| `( )` | grouping | `(1 + 2) * 3` | `9` |
| unary `-` `+` | sign | `-5` | `-5` |

`*`, `/` and `%` bind tighter than `+` and `-`. Dividing by zero stops the program with
an error.

### Variables

You create a variable by assigning to it. There is no declaration keyword. Assigning to it
again updates it.

```
mool = 5000
dar = 10
byaj = mool * dar * 3 / 100
vana(byaj)    # 1500
```

Using a variable before it has a value is an error. Names may only use ASCII letters,
digits and `_`, so `baki` works but `bakī` does not.

### Comparisons

`==  !=  <  <=  >  >=` give `1` for true and `0` for false. Any non-zero value counts as
true. You can't chain comparisons: `a < b < c` is an error.

### Conditionals: `yedi` / `natara yedi` / `natara`

```
ank = 75
yedi ank >= 80 {
    vana(1)
} natara yedi ank >= 60 {
    vana(2)
} natara {
    vana(3)
}
```

`natara` must be on the **same line** as the closing `}` before it.

### While loop: `jabasamma`

```
i = 0
jabasamma i < 3 {
    vana(i)
    i = i + 1
}
```

### For loop: `ko_lagi ... in a..b`

`ko_lagi` counts from `a` up to, but **not including**, `b`. Both bounds are evaluated
once, before the loop starts.

```
ko_lagi i in 1..4 {
    vana(i)       # 1, 2, 3
}
```

### Functions: `kaam` and `farkau`

`kaam` defines a function and `farkau` returns a value from it.

```
kaam jamma(a, b) {
    farkau a + b
}

vana(jamma(10, 20))    # 30
```

- Parameters are separated by commas. A function can have no parameters: `kaam f() { }`.
- `farkau` exits right away, even from inside loops.
- A function that ends without `farkau`, or uses a bare `farkau`, returns `0`.
- You can call a function before its definition appears in the file.
- Recursion works. Recursion deeper than 1000 calls stops with a clear error.
- A function can read top-level variables. Assigning inside a function always creates or
  updates a **local** variable, so a function never changes a top-level one.
- Functions are defined at the top level. You can't define a function inside another.
- You can call a function as a statement by itself, e.g. `salam()`.

---

## Error messages

NepLang tries to give errors a beginner can understand. Each error names the file, line
and column, shows the line of code, and points to the problem:

```
Error in program.np at line 5, column 6:

    vana(bhitra)
         ^

Variable 'bhitra' is not defined. Assign it a value first, e.g. 'bhitra = 0'.
```

Only the first syntax error is reported. Fix it and run the program again. Keywords can't
be used as variable names (`yedi = 5` is an error).

---

## Keyword reference

| Keyword | Meaning | English equivalent |
|---|---|---|
| `vana` | say / print | `print` |
| `yedi` | if | `if` |
| `natara` | otherwise | `else` |
| `jabasamma` | as long as | `while` |
| `ko_lagi` | for | `for` |
| `in` | in | `in` |
| `kaam` | work / task | `function` |
| `farkau` | return / send back | `return` |

---

## Current limitations

NepLang is still early. These features are **not** available yet:

- strings, floating-point numbers, booleans (`sahi`/`galat`-style)
- logical operators (and / or / not)
- `break` / `continue`
- arrays, structs and classes, modules / `import`
- expressions that span more than one line

They come in later milestones (see the [Roadmap](#roadmap)).

---

## Development

```bash
make          # build bin/neplang
make debug    # build with AddressSanitizer + UBSan (use while developing)
make test     # build, then run every test suite under tests/
make clean    # delete bin/
```

The compiler is built with `-Wall -Wextra -Wpedantic` and should build with zero warnings.

### Tests

`tests/run_tests.sh` is a plain bash script:

- Each folder in `tests/` is a suite (`calculator/`, `variables/`, `conditionals/`,
  `loops/`, `functions/`, ...).
- `NAME.np` + `NAME.expected`: the program is run and its output must match the
  `.expected` file exactly.
- `NAME.error.np`: the program must fail with a non-zero exit code (syntax or runtime
  error).

To add a test, drop the files into a suite folder. No registration is needed.

### How it works

```
source.np → Lexer → tokens → Parser (recursive descent) → AST → Interpreter (tree-walk) → output
```

| File | Role |
|---|---|
| `src/lexer.c` | Turns source text into tokens; keywords live in the `keywords[]` table |
| `src/parser.c` | Recursive-descent parser that builds the AST |
| `src/ast.c` / `ast.h` | AST node types, constructors, and `ast_free` |
| `src/interpreter.c` | Walks the AST and runs the program |
| `src/diagnostics.c` | Shared error reporter (file, line, column, caret) |
| `src/main.c` | Reads the file and connects the pipeline |

### Releasing prebuilt binaries

`.github/workflows/release.yml` builds static binaries for Linux (x86_64, arm64),
macOS (universal) and Windows (x86_64). It runs the test suite on each platform and
attaches the binaries, plus a `SHA256SUMS` file, to a GitHub Release. To publish a
release, push a version tag:

```bash
git tag v0.5.0
git push origin v0.5.0
```

`install.sh` and `install.ps1` always download from the **latest** release. To install a
specific version, set `NEPLANG_VERSION=v0.5.0` first.

---

## Project layout

```
src/          lexer, parser, AST, interpreter, diagnostics, main
tests/        per-milestone test cases (input + expected output)
examples/     example .np programs
docs/         language design notes
Makefile      build commands
install.sh    one-line installer for Linux / macOS
install.ps1   one-line installer for Windows
.github/      release workflow (builds the prebuilt binaries)
```

---

## Roadmap

1. ✅ Calculator: integer arithmetic
2. ✅ Variables
3. ✅ Conditionals: `yedi` / `natara`
4. ✅ Loops: `jabasamma`, `ko_lagi`
5. ✅ Functions: `kaam` / `farkau`
6. Arrays
7. Structs / classes
8. Symbol table (hashed, scoped)
9. Type checker
10. Modules (`import`)
11. Bytecode VM
12. Intermediate representation (IR)
13. Native code generation

---

## Contributing

Work on NepLang goes one milestone at a time. Please keep each change to the current
milestone, add tests for any new feature (including at least one `.error.np` case), and
make sure `make test` passes with no compiler warnings.
