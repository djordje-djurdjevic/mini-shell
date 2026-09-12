Edge Case Tracker

Format: [ ] open, [x] fixed. Include exact input sequence, expected vs actual, and root cause once found.


File completion:

[ ] File completion for absolute paths doesn't work. Currently works only for relative path.

Completer-based completion

[ ]  Duplication when completing from an empty current word inside check_completer branch Input: git checkout <TAB> (trailing space, second tab press) Expected: LCP/candidate logic runs against empty current word, no duplication of previous token Actual: still duplicates something (e.g. "test checkout che").

[ ] Completer doesn't check if the the path to the completer script is a an actual path. Input: "complete -C invalid/compelter/path func" Exptected: a message saying that that the path to completer script is of wrong format Actual: nothing happens just eats the path as if it's a legit path. 


Shell input

[ ] Cannot move freely with arrows left/right trough the current input Expected: to move back and fourth trough the current input text Actual: nothing happnes just ignores the arrow keys and escape sequences.

[ ] Completion doesn't  work if first tokens are spaces. Input: "____ech" Expected: to complete the input to "___echo" Actual: Rings the bell like there aren't any matches. 

Piping commands

[ ]