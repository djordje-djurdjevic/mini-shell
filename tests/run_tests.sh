#!/bin/bash
# Test suite for the custom shell (ParseInput / BuiltinFunction / ProgramFunction)
# Run: ./tests/run_tests.sh


SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR/.."


# ---------------------------------------------------------------------------
# Colors
# ---------------------------------------------------------------------------
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
BOLD='\033[1m'
NC='\033[0m' # No Color
 

# ---------------------------------------------------------------------------
# Fixtures
# ---------------------------------------------------------------------------
mkdir -p /tmp/shell_tests
echo "Hello from file" > /tmp/shell_tests/sample.txt

TOTAL_TESTS=0
FAILED_TESTS=0


# ---------------------------------------------------------------------------
# Test helpers
# ---------------------------------------------------------------------------
run_output_test() {
	local test_name=$1
	local input=$2
	local expected=$3
	
	#echo "DEBUG: $input"
	output=$(echo "$input" | ./shell 2>&1)

	if [[ "$output" == *"$expected"* ]]; then
	        echo -e "${GREEN}PASS${NC}  $test_name"
	else
		echo -e "${RED}FAIL${NC}  $test_name"
        	echo -e "      expected to contain: ${YELLOW}${expected}${NC}"
        	echo -e "      got:                 ${YELLOW}${output}${NC}"
		((FAILED_TESTS++))
	fi

	((TOTAL_TESTS++))
}

run_exit_code_test() {
	local test_name=$1
	local input=$2
	local expected=$3

	#echo "DEBUG: $input"
	echo "$input" | ./shell > /dev/null #redirect stdout to null
	local actual_code=$?

	if [ "$actual_code" -eq "$expected" ]; then
        	echo -e "${GREEN}PASS${NC}  $test_name"
	else
		echo -e "${RED}FAIL${NC}  $test_name"
		echo -e "      expected exit code: ${YELLOW}${expected}${NC}, got: ${YELLOW}${actual_code}${NC}"
		((FAILED_TESTS++))
	fi

	((TOTAL_TESTS++))
}

run_exact_output_test() {
	local test_name=$1
	local input=$2
	local expected=$3

	#echo "DEBUG: $input"
	output=$(echo "$input" | ./shell 2>&1)
	output=$(echo "$output" | sed -n '1s/^\$ //p')  
	output=$(echo "$output" | sed 's/^\$ //; s/ *\$ *$//')

	if [ "$output" == "$expected" ]; then
        	echo -e "${GREEN}PASS${NC}  $test_name"
	else
		echo -e "${RED}FAIL${NC}  $test_name"
        	echo -e "      expected: '${YELLOW}${expected}${NC}'"
        	echo -e "      got:      '${YELLOW}${output}${NC}'"
 
		((FAILED_TESTS++))
	fi
	#echo "DEBUG: $output"
	((TOTAL_TESTS++))
}

# ---------------------------------------------------------------------------
# Builtins
# ---------------------------------------------------------------------------
echo -e "${BOLD}-- builtins --${NC}"
run_output_test    "unknown command"      "xyz"                 "xyz: command not found"
run_output_test    "echo"                 "echo Hello World!"   "Hello World!"
run_output_test    "type builtin"         "type type"           "type is a shell builtin"
run_output_test    "type PATH lookup"     "type python3"        "python3 is /usr/bin/python3"
run_exit_code_test "exit"                 "exit"                0
 
# ---------------------------------------------------------------------------
# External programs
# ---------------------------------------------------------------------------
echo -e "${BOLD}-- external programs --${NC}"
run_output_test "cat existing file"    "cat /tmp/shell_tests/sample.txt"        "Hello from file"
run_output_test "cat nonexistent file" "cat /tmp/shell_tests/doesnotexist.txt"  "No such file or directory"
run_output_test "nonexistent program"  "totallyfakecommand123"                  "totallyfakecommand123: command not found"
run_output_test "pwd"                  "pwd"                                    "/home/djole/Documents/shell"
 
# ---------------------------------------------------------------------------
# cd
# ---------------------------------------------------------------------------
echo -e "${BOLD}-- cd --${NC}"
run_output_test "cd nonexistent dir" "cd /tmp/ne_postoji_ovaj_folder" "No such file or directory"
run_output_test "cd absolute path"   "cd /tmp
pwd" "/tmp"
run_output_test "cd relative path"   "cd tests
pwd" "/home/djole/Documents/shell/tests"
run_output_test "cd home directory"  "cd ~
pwd" "/home/"
 
# ---------------------------------------------------------------------------
# Quoting - single quotes
# ---------------------------------------------------------------------------
echo -e "${BOLD}-- single quotes --${NC}"
run_exact_output_test "preserves internal spaces"   "echo 'hello    world'" "hello    world"
run_exact_output_test "collapses unquoted spaces"   "echo hello    world"   "hello world"
run_exact_output_test "adjacent quotes concatenate" "echo 'hello''world'"   "helloworld"
run_exact_output_test "empty quotes concatenate"    "echo hello''world"     "helloworld"
 
# ---------------------------------------------------------------------------
# Quoting - double quotes
# ---------------------------------------------------------------------------
echo -e "${BOLD}-- double quotes --${NC}"
run_exact_output_test "preserves internal spaces"   "echo \"hello    world\""    "hello    world"
run_exact_output_test "adjacent quotes concatenate" "echo \"hello\"\"world\""    "helloworld"
run_exact_output_test "quote mid-word"              "echo \"hello\"world"        "helloworld"
run_exact_output_test "two separate quoted args"    "echo \"hello\" \"world\""   "hello world"
run_exact_output_test "single quote literal inside" "echo \"shell's test\""      "shell's test"

# ---------------------------------------------------------------------------
# Backslash - outside quotes
# ---------------------------------------------------------------------------
echo -e "${BOLD}-- backslash --${NC}"
run_exact_output_test "preserves internal spaces" "echo three\ \ \ spaces" "three   spaces"
run_exact_output_test "preserves first space" "echo before\     after" "before  after"
run_exact_output_test "newline is just n" "echo test\nexample" "testnexample"
run_exact_output_test "preserves backslash" "echo hello\\\\world" "hello\world"
run_exact_output_test "single quote as literal" "echo \'hello\'" "'hello'"

# ---------------------------------------------------------------------------
# Backslash - in single quotes
# ---------------------------------------------------------------------------
echo -e "${BOLD}-- backslash in single qoutes --${NC}"
run_exact_output_test "backslash as literal" 'echo '\''shell\\\nscript'\''' 'shell\\\nscript'
run_exact_output_test "literal inside single quotes" $"echo 'example\"test'" "example\"test"
echo "content1" > "/tmp/shell_tests/no slash 1"
echo "content2" > '/tmp/shell_tests/one slash \2'
echo "content3" > '/tmp/shell_tests/two slashes \3\'
run_output_test "single-quote backslash literal" \
  "cat /tmp/shell_tests/'no slash 1' /tmp/shell_tests/'one slash \\2' /tmp/shell_tests/'two slashes \\3\\'" \
"content1
content2
content3"

# ---------------------------------------------------------------------------
# Backslash - in double quotes
# ---------------------------------------------------------------------------
echo -e "${BOLD}-- backslash in double qoutes --${NC}"
run_exact_output_test "escape backslash in \"\"" "echo \"A \\ escapes itself\"" "A \ escapes itself"
run_exact_output_test "\Double qoute in double qoutes" "echo \"A \\\" inside double quotes\"" "A \" inside double quotes"
echo "content1" > "/tmp/shell_tests/number 1"
echo "content2" > '/tmp/shell_tests/doublequote " 2'
echo "content3" > '/tmp/shell_tests/backslash \ 3'
run_output_test 'double-quote backslash escaping' 'cat /tmp/shell_tests/"number 1" /tmp/shell_tests/"doublequote \" 2" /tmp/shell_tests/"backslash \\ 3"' 'content1
content2
content3'
#run_exact_output_test "" "" ""

# ---------------------------------------------------------------------------
#  Quoted Executable Names
# ---------------------------------------------------------------------------
echo -e "${BOLD}-- quoted executable names --${NC}"
mkdir -p /tmp/shell_tests/bin

cat > '/tmp/shell_tests/bin/exe with "quotes"' << 'EOF'
#!/bin/bash
cat "$1"
EOF
chmod u+x '/tmp/shell_tests/bin/exe with "quotes"'

cat > "/tmp/shell_tests/bin/exe with 'single quotes'" << 'EOF'
#!/bin/bash
cat "$1"
EOF
chmod u+x "/tmp/shell_tests/bin/exe with 'single quotes'"

echo "content1" > /tmp/shell_tests/file1.txt
echo "content2" > /tmp/shell_tests/file2.txt

export PATH="/tmp/shell_tests/bin:$PATH"

run_output_test 'quoted executable - double quotes with embedded quotes' \
  "'exe with \"quotes\"' /tmp/shell_tests/file1.txt" \
  'content1'

run_output_test 'quoted executable - single quotes with embedded quotes' \
  "\"exe with 'single quotes'\" /tmp/shell_tests/file2.txt" \
  'content2'

# ---------------------------------------------------------------------------
# Redirect - The > Operator
# ---------------------------------------------------------------------------
echo -e "${BOLD}-- the > operator --${NC}"
echo hello > /tmp/shell_tests/output.txt
run_output_test "Redirect to a file" "cat /tmp/shell_tests/output.txt" "hello"
echo hello 1> /tmp/shell_tests/output.txt
run_output_test "Redirect to a file" "cat /tmp/shell_tests/output.txt" "hello"
run_output_test "Error on stdout" "cat nonexistent > /tmp/shell_tests/output.txt" "cat: nonexistent: No such file or directory"

# ---------------------------------------------------------------------------
# Redirect - The 2> Operator - Errors
# ---------------------------------------------------------------------------
echo -e "${BOLD}-- the 2> operator --${NC}"
cat nonexistent 2> /tmp/shell_tests/output.txt
run_output_test "Error into file" "cat /tmp/shell_tests/output.txt" "cat: nonexistent: No such file or directory"

echo contents of existing file > /tmp/shell_tests/existing.txt
cat /tmp/shell_tests/existing.txt nonexistent 2> /tmp/shell_tests/errors.txt
run_output_test "cat normal text 2> operator" "cat /tmp/shell_tests/existing.txt nonexistent 2> /tmp/shell_tests/errors.txt" "contents of existing file"
run_output_test "cat error text 2> operator" "cat /tmp/shell_tests/errors.txt" "cat: nonexistent: No such file or directory"

# ---------------------------------------------------------------------------
# Redirect - The >> Operator
# ---------------------------------------------------------------------------
echo -e "${BOLD}-- the >> operator --${NC}"
echo first > /tmp/shell_tests/output.txt
echo second >> /tmp/shell_tests/output.txt
run_output_test "Append >> operator" "cat /tmp/shell_tests/output.txt" "first
second"

echo third >> /tmp/shell_tests/output.txt
echo fourth >> /tmp/shell_tests/output.txt
run_output_test "Append >> operator" "cat /tmp/shell_tests/output.txt" "first
second
third
fourth"

# ---------------------------------------------------------------------------
# Redirect - The 2>> Operator
# ---------------------------------------------------------------------------
echo -e "${BOLD}-- the 2>> operator --${NC}"
echo "" > /tmp/shell_tests/errors.txt
cat nonexistent1 2>> /tmp/shell_tests/errors.txt
cat nonexistent2 2>> /tmp/shell_tests/errors.txt
run_output_test "Append errors 2>>" "cat /tmp/shell_tests/errors.txt" "cat: nonexistent1: No such file or directory
cat: nonexistent2: No such file or directory"


mkdir -p /tmp/foo
rm -f /tmp/foo/baz.md /tmp/foo/qux.md /tmp/foo/quz.md

run_output_test '2>> writes stderr to file' \
  "ls nonexistent 2>> /tmp/foo/qux.md
cat /tmp/foo/qux.md" \
  "No such file or directory"

  run_output_test '2>> appends multiple errors' \
  "cat nonexistent 2>> /tmp/foo/quz.md
ls nonexistent 2>> /tmp/foo/quz.md
cat /tmp/foo/quz.md" \
"No such file or directory"

# ---------------------------------------------------------------------------
# Summary
# ---------------------------------------------------------------------------
if [ "$FAILED_TESTS" -gt 0 ]; then
	passed=$(($TOTAL_TESTS - $FAILED_TESTS))
    	echo -e "${RED}${BOLD}Tests passed: ${passed}/${TOTAL_TESTS}${NC}"
	exit 1
fi
echo -e "${GREEN}${BOLD}ALL TESTS PASSED (${TOTAL_TESTS}/${TOTAL_TESTS})${NC}"
exit 0
