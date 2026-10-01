# Shell

A POSIX-style Unix shell written from scratch in C. Built as part of the [CodeCrafters "Build your own Shell"](https://codecrafters.io/challenges/shell) challenge.

## Features

- **Command execution**: runs builtins and external programs found via `PATH`
- **Quoting**: single quotes, double quotes and argument tokenizing
- **Variables**: `declare NAME=value`, expansion with `$NAME` and `${NAME}`
  - works inside double quotes, not inside single quotes
  - undefined variables expand to an empty string
  - unclosed `${` is reported as a `bad substitution` error
- **Pipelines**: `cmd1 | cmd2 | cmd3`, including builtins in a pipeline
- **Redirection**: output and error redirection to files
- **Command history**: in-session history with persistence through `HISTFILE`
- **Tab completion**: builtins, executables, files, plus programmable completion through external completer scripts

## Project structure

```
.
├── include/            # Header files (one per module + common.h)
├── src/
│   ├── shell.c         # Entry point and main read-parse-execute loop
│   ├── parser.c        # Tokenizer, quote handling, variable expansion
│   ├── executor.c      # Runs commands and pipelines
│   ├── builtins.c      # Builtin commands
│   ├── pipe.c          # Pipeline setup (pipes, fork, fd handling)
│   ├── redirect.c      # Output / error redirection
│   ├── history.c       # Command history and HISTFILE handling
│   └── completion.c    # Tab completion and completer script support
├── tests/
│   ├── run_tests.sh    # Bash test harness
│   └── completers/     # Python completer scripts used by the tests
├── EDGE_CASES.md       # Known limitations and deviations from bash
└── Makefile
```

### How a command flows through the shell

1. `shell.c` reads a line of input.
2. `parser.c` tokenizes it, handles quotes, expands variables and builds a pipeline.
3. `executor.c` runs it: builtins directly, external programs through `fork` + `exec`.
4. `pipe.c` and `redirect.c` wire up file descriptors where the line contains `|` or redirections.

## Build and run

Requirements: `gcc` and `make`.

```bash
make        # build the ./shell binary
make run    # build and start the shell
```

The project compiles with `-Wall -Wextra -g`.

## Example session

```
$ declare name=world
$ echo hello ${name}
hello world
$ echo 'no $expansion here'
no $expansion here
$ echo one two three | wc -w
3
```

## Testing

```bash
bash tests/run_tests.sh
```

The tests run the shell against expected output. Completion tests use the scripts in `tests/completers/`.

For memory errors, run under Valgrind:

```bash
valgrind --leak-check=full ./shell
```

## Known limitations

Some behavior deliberately differs from bash (for example, empty quoted arguments and escaping `\$`). See [EDGE_CASES.md](EDGE_CASES.md) for the full list.

## What I learned

- Process management with `fork`, `execvp`, `waitpid` and file descriptor lifecycle in pipelines
- Writing a character-by-character tokenizer with quote and expansion state
- Manual memory management and debugging with Valgrind in a multi-module C project