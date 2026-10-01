# Edge Case Tracker

Known bugs, limitations and deviations from bash.

**Status:** `[ ]` open, `[x]` fixed
**Entry format:** input → expected → actual → root cause (once found) → fix idea

---

## File completion

- [x] **Absolute paths not completed**
  - Input: `ls /usr/bi<TAB>`
  - Expected: completes to `/usr/bin/`
  - Actual: worked only for relative paths

---

## Completer-based completion

- [ ] **Duplicated text when the current word is empty**
  - Input: `git checkout <TAB>` (trailing space, second Tab press)
  - Expected: LCP / candidate logic runs against an empty current word, previous tokens are not repeated
  - Actual: previous token is duplicated (e.g. `test checkout che`)
  - Root cause: not found yet, happens in the `check_completer` branch
  - Debug idea: print the word passed to the completer and the prefix used for the LCP when the word is empty

- [ ] **Completer path is never validated**
  - Input: `complete -C invalid/completer/path func`
  - Expected: an error message that the completer script path is invalid
  - Actual: silently accepted as if valid
  - Note: bash itself does not validate at registration time, it fails when completion runs. Decide whether to match bash or validate early with `access(path, X_OK)`, and record the decision here.

---

## Shell input

- [ ] **Left / right arrows are ignored**
  - Expected: cursor moves through the current input, characters are inserted and deleted at the cursor
  - Actual: escape sequences (`ESC [ C`, `ESC [ D`) are ignored
  - Root cause: input is handled as an append-only buffer, there is no cursor position
  - Fix idea: keep `len` and `cursor` separately, insert at `cursor`, redraw the line after every edit. Same change enables Home, End and Delete.

- [ ] **Completion fails when the input starts with spaces**
  - Input: `____ech<TAB>` (underscores are spaces)
  - Expected: `____echo`
  - Actual: bell, as if there were no matches
  - Likely root cause: the first token is taken from the start of the buffer without skipping leading whitespace, so the "command position" check fails
  - Fix idea: skip leading whitespace before deciding whether the word is in command position

---

## Piping

- [ ] **`run_builtin` conflates "not a builtin" and "builtin failed"**
  - Input: `cd bad_dir | cat`
  - Expected: `cd` runs as a builtin, prints its error, the child exits with status 1
  - Actual: `run_builtin` returns `false` in both cases, so the child falls through to `execvp("cd", ...)`
  - Root cause: a single `bool` carries two different meanings
  - Fix idea: return a three-state result

---

## Variable expansion

- [ ] **Empty quoted argument is dropped**
  - Input: `echo "$UNDEFINED" x` or `echo "" x`
  - Expected: `echo` receives two arguments, the first empty
  - Actual: the empty token is skipped because the parser only adds a token when `i_arg > 0`
  - Fix idea: add a `token_started` flag set when a quote opens or a variable expands, and use it instead of `i_arg > 0`

- [ ] **Buffer capacity checks**
  - Input: a variable whose value is longer than the token buffer, or a name longer than the name buffer
  - Expected: no overflow (truncate or report an error)
  - Risk: `name[n++]` and the copy into `arg` need bounds checks

- [ ] **Bare `$` and `${}`**
  - Input: `echo $`, `echo "cost: $"`, `echo ${}`
  - Expected: `$` stays literal for a bare `$`, `${}` gives `bad substitution`

- [ ] **Special parameters** such as `$?`, `$$`, `$0` are not supported

---

## Not checked yet

- Very long input lines (input buffer size)
