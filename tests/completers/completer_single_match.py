#!/usr/bin/env python3
import sys

if sys.argv[1] == "docker" and sys.argv[2] in ("docker", "", None):
    print(" run", end="")

#print(f"argv={sys.argv}", file=sys.stderr)