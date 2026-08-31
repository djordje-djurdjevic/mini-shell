#!/usr/bin/env python3
import sys
#print(f"argv1={sys.argv[1]} argv2={sys.argv[2]} argv3={sys.argv[3]}", file=sys.stderr)
if sys.argv[1] == "git" and sys.argv[3] == "remote" and sys.argv[2] == "set":
	print("set-url", end="")

#print(f"argv={sys.argv}", file=sys.stderr)
