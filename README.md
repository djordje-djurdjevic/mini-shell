# Shell

An interactive Unix shell written in C for the [CodeCrafters Shell challenge](https://codecrafters.io/challenges/shell). It runs builtins and external programs, connects commands with pipes, and handles terminal input directly with `termios`.

## Quick start

Build and run on Linux with GCC and Make.

```sh
make
./shell
```

You can also use `make run` to build and launch in one step. Type `exit` to leave the shell.

## Example session

```text
$ declare name=world
$ echo "Hello, ${name}"
Hello, world
$ echo 'Literal $name'
Literal $name
$ echo one two three | wc -w
3
$ echo first line > notes.txt
$ echo second line >> notes.txt
$ cat notes.txt
first line
second line
```

## Features

- Run programs by path or look them up through `PATH`. Use `type` to see how a command resolves.
- Define shell-local variables with `declare NAME=value`. Double quotes allow expansion and single quotes preserve literal text.
- Connect builtins and external programs with pipelines. Redirect stdout with `>` or `>>` and stderr with `2>` or `2>>`. Use spaces around operators and put output redirection at the end of the final pipeline stage.
- Run external commands or pipelines in the background with a space-separated trailing `&`. Use `jobs` to list tracked processes.
- Use Up and Down to browse history, Backspace to edit the end of the line, and Tab to complete commands and paths.

### Persistent history

Set `HISTFILE` in your regular terminal when launching the shell. Interactive sessions read the history file at startup and append new entries on exit.

```sh
HISTFILE="$HOME/.codecrafters-shell-history" ./shell
```

Inside the shell, use `history` to view entries. The `-r`, `-w`, and `-a` options read, write, and append entries to a specified file.

### Programmable Tab completion

From the repository root, register the bundled Python 3 completer inside the shell.

```text
$ complete -C ./tests/completers/completer_git_args.py git
```

Type `git remote set` and press **Tab** to complete the last word to `set-url`. A completer receives the command name, current word, and previous word as arguments, plus `COMP_LINE` and `COMP_POINT` in its environment. It prints one candidate per line to stdout.

Use `complete -p git` to inspect the registration and `complete -r git` to remove it.

## How it works

1. [`shell.c`](src/shell.c) reads terminal input and dispatches commands.
2. [`parser.c`](src/parser.c) handles quoting and expansion, then builds a pipeline of argument lists.
3. [`builtins.c`](src/builtins.c) runs standalone builtins in the shell process so commands such as `cd` can update its state. [`executor.c`](src/executor.c) launches external programs with `fork` and `execvp`.
4. [`pipe.c`](src/pipe.c) connects pipeline stages running in child processes. [`redirect.c`](src/redirect.c) opens output files for redirection.

[`history.c`](src/history.c) handles history files and [`completion.c`](src/completion.c) generates Tab completion candidates.

## Testing

```sh
make
bash tests/run_tests.sh
```

Two assertions use the original checkout path, so `pwd` and relative `cd` can fail in another directory. The command lookup test expects `python3` at `/usr/bin/python3`. See [known edge cases](EDGE_CASES.md) for the limits of this implementation.

## What I learned

- Managing child processes and file descriptors with `fork`, `execvp`, `waitpid`, and `dup2`.
- Tracking quotes and variable expansion in a character-by-character tokenizer.
- Managing memory and debugging a multi-module C program with Valgrind.

## License

[MIT](LICENSE)
