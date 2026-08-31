Edge Case Tracker

Format: [ ] open, [x] fixed. Include exact input sequence, expected vs actual, and root cause once found.


Completer-based completion

[]  Duplication when completing from an empty current word inside check_completer branch Input: git checkout <TAB> (trailing space, second tab press) Expected: LCP/candidate logic runs against empty current word, no duplication of previous token Actual: still duplicates something (e.g. "test checkout che")