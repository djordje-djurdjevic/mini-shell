#!/usr/bin/env python3
import sys, os

comp_line = os.environ.get("COMP_LINE", "MISSING")
comp_point = os.environ.get("COMP_POINT", "MISSING")

#print(f"argv={sys.argv[1:]} COMP_LINE='{comp_line}' COMP_POINT='{comp_point}'", file=sys.stderr)

if sys.argv[1] == "git" and sys.argv[2] == "ad":
	print("add", end="")
