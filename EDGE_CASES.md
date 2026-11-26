# Known edge cases

This page records the behavior and limits of the completed shell project.

## Parsing and variable expansion

- Empty quoted arguments are dropped, as in `echo "" x` or `echo "$UNDEFINED" x`. If the entire line produces zero arguments, the parser can access the argument list out of bounds.
- A bare `$` and an empty `${}` expand to an empty string. An unclosed `${` produces a `bad substitution` error.
- Special parameters such as `$?`, `$$`, and `$0` are unsupported. Variable lookup uses the shell's own `declare` list.
- Quoting or escaping an operator can still leave a token the shell treats as syntax.
- The parser uses fixed-size buffers. Variable expansion can exceed the available token space, and long input lines have limited test coverage.

## Pipelines, redirection, and jobs

- A failed builtin in a pipeline can fall through to external command lookup. For example, `cd bad_dir | cat` can attempt to run an external `cd`.
- The shell handles one output redirection per command. In a pipeline, it handles redirection on the final stage.
- Background pipelines are tracked by the PID of their final stage. Background execution can inspect an uninitialized wait status, and the job and pipeline arrays have fixed capacities without insertion checks.

## Interactive input and completion

- Left and Right arrows do not move the cursor. Backspace edits the end of the line, and Up and Down navigate history.
- Leading spaces can prevent command-name completion.
- With some external completers, pressing Tab again after a trailing space can duplicate part of the previous word.
- `complete -C` stores the completer path as given. The shell tries to execute it when completion runs.
- Absolute paths are supported. For example, `ls /usr/bi` followed by Tab can complete to `ls /usr/bin/`.

## Test portability

The `pwd` and relative `cd` assertions use the original checkout's absolute path. The `type python3` assertion expects `/usr/bin/python3`.
