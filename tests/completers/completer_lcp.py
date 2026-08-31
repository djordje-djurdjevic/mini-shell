#!/usr/bin/env python3
import sys

candidates = ["checkout", "cherry-pick"]

def main():
    word = sys.argv[2] if len(sys.argv) > 2 else ""
    matches = [c for c in candidates if c.startswith(word)]
    print("\n".join(matches))

if __name__ == "__main__":
    main()