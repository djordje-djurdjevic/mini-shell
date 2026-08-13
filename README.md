# mini-shell

A custom Unix shell implemented in C, built from scratch on top of raw POSIX syscalls (`fork`, `execvp`, `dup2`, `open`, termios) — no `readline`, no external parsing libraries.

## Features

- **Interactive line editing** — raw terminal mode (non-canonical, no echo) with manual backspace and character handling, so completion and prompt redraw work byte-by-byte.
- **Tab completion** — completes against shell builtins and executables found on `$PATH`.
  - Single match: completes inline.
  - Multiple matches: first `Tab` fills in the longest common prefix (LCP); second `Tab` lists all candidates, shell-style.
  - No match: terminal bell.
- **Quoting & escaping** — supports single quotes (fully literal), double quotes (with `\`, `"`, `$`, `` ` `` escaping inside), and backslash escaping outside quotes.
- **Output redirection** — `>`, `1>`, `2>` (truncate) and `>>`, `1>>`, `2>>` (append), for both stdout and stderr.
- **Builtins** — `echo`, `type`, `pwd`, `cd` (including `cd ~`), `exit`.
- **External programs** — resolved and executed via `$PATH` using `fork` + `execvp`.
- **Non-interactive mode** — reads commands from stdin (e.g. piped input or a script) when stdin isn't a TTY.

## Building

```sh
make
make all
```

No external dependencies beyond the standard C library and POSIX headers (`unistd.h`, `termios.h`, `dirent.h`, `fcntl.h`, `sys/wait.h`).

## Running

```sh
make run
./shell
```

```
$ echo hello world
hello world
$ type echo
echo is a shell builtin
$ ls > out.txt
$ cat out.txt
```

## Known limitations / roadmap

- No piping (`|`) between commands yet.
- No input redirection (`<`).
- Fixed-size buffers (`MAX_SIZE = 1024`) for input, arguments, and `$PATH` — very long arguments or an unusually long `$PATH` are not yet bounds-checked everywhere.
- No job control / background processes (`&`).
- Arrow-key history navigation is not implemented (escape sequences are currently swallowed and ignored).

## Project structure

Single-file implementation (`main.c`), organized around:

- **Input loop** — raw-mode key reading, backspace/tab handling, prompt redraw.
- **Parser** (`parse_input`) — tokenizes a raw input line into `argv`-style array, handling quotes/escapes.
- **Redirection** (`check_output_redirect` / `restore_std`) — detects and strips redirection operators from args, opens the target file, and swaps `stdout`/`stderr` via `dup2` around command execution.
- **Dispatch** (`builtin` / `program`) — routes parsed commands to either a builtin handler or `fork`+`execvp`.
- **Tab completion** (`handle_tab_completion` and helpers) — matches builtins and `$PATH` executables, computes LCP, renders completion or candidate list.