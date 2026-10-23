# cl_color

Colored diagnostics for MSVC's `cl.exe`.

`cl.exe` has no equivalent of GCC/Clang's `-fdiagnostics-color`. `cl_color` is a tiny
(~20 KB, no dependencies) wrapper that runs the compiler you give it, then colorizes
the diagnostics before printing them:

- file path, `(line,col)`, severity (`error` / `warning` / `note`) and the `C4xxx` code each get their own color
- `'quoted'` names inside messages are highlighted
- the source snippet gets a clang-style `  64 | ` gutter, with `//` comments dimmed
- the `^~~~` caret line is highlighted
- `with [ T=... ]` template blocks are colored too

<!-- Add a before/after screenshot here, e.g. docs/screenshot.png:
![cl_color output](docs/screenshot.png)
-->

The wrapper passes the compiler's exit code through, so build systems behave exactly as before.

## Build

From a Visual Studio / MSVC command prompt:

```bat
build.bat
```

or

```bat
cl /nologo /O2 /EHsc /std:c++17 cl_color.cpp
```

or with CMake:

```bat
cmake -G Ninja -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

Put `cl_color.exe` somewhere on your `PATH` (or use its full path below).

## Use with CMake

Use it as a compiler launcher and add `/diagnostics:caret` so MSVC prints the source line and caret:

```bat
cmake -G Ninja -S . -B build ^
  -DCMAKE_CXX_FLAGS="/nologo /W4 /FC /diagnostics:caret" ^
  -DCMAKE_C_COMPILER_LAUNCHER=cl_color.exe ^
  -DCMAKE_CXX_COMPILER_LAUNCHER=cl_color.exe
cmake --build build
```

Notes:

- Tested with the **Ninja** generator. Makefile generators should work too. Visual Studio generators have not been tried.
- Launchers are stored in the CMake cache: configure into a fresh build directory (or re-run configure with the options above).
- `/FC` makes MSVC print full paths, which is handy for clickable paths in terminals/editors.

## Use without CMake

Put `cl_color` in front of any command:

```bat
cl_color cl /nologo /W4 /FC /diagnostics:caret /EHsc main.cpp
cl_color cl /nologo /W4 /diagnostics:caret /EHsc main.cpp /link /OUT:app.exe
```

If you do this often, a `doskey` macro saves typing:

```bat
doskey clc=cl_color.exe cl /nologo /W4 /FC /diagnostics:caret $*
```

## Configuration

| Setting | Effect |
| --- | --- |
| `NO_COLOR=1` (any non-empty value) | disable colors, output is passed through unchanged |
| `CL_COLOR=0` | disable colors |

Colors are `#define`s at the top of `cl_color.cpp` (ANSI SGR codes). Edit and rebuild to taste.
The gutter, comment and quote colors use 256-color codes (`38;5;N`); if your terminal does not
support those, replace them with basic ones such as `\x1b[33m`.

## How it works

`cl_color` strips its own name from the command line, starts the rest as a child process with
stdout and stderr redirected to a pipe, waits for it to finish, rewrites the output line by line
with ANSI escape codes and prints it. The exit code of the child is returned.

Lines starting with `Note: including file:` (printed by `/showIncludes`) are never modified,
because Ninja parses them to track header dependencies.

## Limitations

- Output is buffered per compiler invocation and printed when the compiler exits (not streamed).
- Colorizing is heuristic, based on MSVC's message format (`path(line,col): severity Cnnnn: message`).
  Output from other tools may be only partly colored.
- Non-English Windows installs print diagnostics in the local codepage and with localized
  severity words. Only the English `error` / `warning` / `note` words are recognized.
- Requires a terminal that understands ANSI escape codes (Windows Terminal, ConEmu with ANSI enabled, modern conhost).

## License

MIT, see [LICENSE](LICENSE).
