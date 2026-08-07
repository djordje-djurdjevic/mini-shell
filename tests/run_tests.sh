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

	output=$(echo "$input" | ./shell 2>&1)
	output=$(echo "$output" | sed -n '1s/^\$ //p')  
	result=$(echo "$output" | sed 's/^\$ //; s/ *\$ *$//')

	if [ "$output" == "$expected" ]; then
        	echo -e "${GREEN}PASS${NC}  $test_name"
	else
		echo -e "${RED}FAIL${NC}  $test_name"
        	echo -e "      expected: '${YELLOW}${expected}${NC}'"
        	echo -e "      got:      '${YELLOW}${output}${NC}'"
 
		((FAILED_TESTS++))
	fi
	#echo "$output"
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
# Summary
# ---------------------------------------------------------------------------
if [ "$FAILED_TESTS" -gt 0 ]; then
	passed=$(($TOTAL_TESTS - $FAILED_TESTS))
    	echo -e "${RED}${BOLD}Tests passed: ${passed}/${TOTAL_TESTS}${NC}"
	exit 1
fi
echo -e "${GREEN}${BOLD}ALL TESTS PASSED (${TOTAL_TESTS}/${TOTAL_TESTS})${NC}"
exit 0


run_output_test "unknown_command" "xyz" "xyz: command not found"
run_output_test "echo" "echo Hello World!" "Hello World!"
run_output_test "type" "type type" "type is a shell builtin"
run_output_test "type PATH var" "type python3" "python3 is /usr/bin/python3"
run_exit_code_test "exit" "exit" 0

run_output_test "cat existing file" "cat /tmp/shell_tests/sample.txt" "Hello from file"
run_output_test "cat nonexistent file" "cat /tmp/shell_tests/doesnotexist.txt" "No such file or directory"
run_output_test "nonexistent program" "totallyfakecommand123" "totallyfakecommand123: command not found"
run_output_test "pwd" "pwd" "/home/djole/Documents/shell"

run_output_test "cd nonexistent dir" "cd /tmp/ne_postoji_ovaj_folder" "No such file or directory"
run_output_test "cd absolute path" "cd /tmp
pwd" "/tmp"
run_output_test "cd relative path" "cd tests
pwd" "/home/djole/Documents/shell/tests"
run_output_test "cd home directory" "cd ~
pwd" "/home/"

run_exact_output_test "single qoutes 1" "echo 'hello    world'" "hello    world"
run_exact_output_test "single qoutes 2" "echo hello    world"   "hello world"
run_exact_output_test "single qoutes 3" "echo 'hello''world'"   "helloworld"
run_exact_output_test "single qoutes 4" "echo hello''world"     "helloworld"

run_exact_output_test "double qoutes 1" "echo \"hello    world\""  "hello    world"
run_exact_output_test "double qoutes 2" "echo \"hello""world\""    "helloworld"
run_exact_output_test "double qoutes 3" "echo \"hello\"world"      "helloworld"
run_exact_output_test "double qoutes 4" "echo \"hello\" \"world\""   "hello world"
run_exact_output_test "double qoutes 5" "echo \"shell's test\""    "shell's test"
