#!/bin/bash

#
# Tema 2 - Bibliotecă stdio - Test Suite
#
# 2019, Operating Systems
#

# ----------------- General declarations and util functions ------------------ #

MAIN_TEST_DIR="./_test/work"
LOG_FILE="/dev/null"
max_points=95

# change this of you want to keep the logs after execution
DO_CLEANUP=${DO_CLEANUP:-yes}

TEST_LIB=_test/test_lib.sh
CHECK_SKIP='_test/'

# load the lib functions
if ! [ -e "$TEST_LIB" ]; then
	echo "Test library not found. Check \$TEST_LIB ($TEST_LIB)"
	exit 1
fi
source "$TEST_LIB"
MEMCHECK=""
MEMCHECK_ERR_CODE=100
[ $(uname -s) == "Linux" ]
IS_LINUX=$?

if [ $IS_LINUX -eq 0 ]; then
	MEMCHECK="valgrind --leak-check=full \
		--show-reachable=yes \
		--vex-iropt-register-updates=allregs-at-mem-access \
		--show-leak-kinds=all \
		--error-exitcode=$MEMCHECK_ERR_CODE \
		$MEMCHECK_EXTRA \
		--log-file=_log "
else
	MEMCHECK="drmemory -batch \
		-exit_code_if_errors $MEMCHECK_ERR_CODE \
		-quiet \
		$MEMCHECK_EXTRA \
		-- "
fi

# ---------------------------------------------------------------------------- #

# ----------------- Init and cleanup tests ----------------------------------- #

# Initializes a test
init_test()
{
	TEST_WORK_DIR="$MAIN_TEST_DIR/test_${test_index}"
	mkdir -p "$TEST_WORK_DIR"
}

# Cleanups a test
cleanup_test()
{
	[ "$DO_CLEANUP" = "yes" ] && rm -rf "$TEST_WORK_DIR" &> /dev/null
}

# Initializes the whole testing environment
# Should be the first test called
init_world()
{
	[ -d $MAIN_TEST_DIR ] && rm -rf $MAIN_TEST_DIR
	mkdir -p $MAIN_TEST_DIR
}

# Cleanups the whole testing environment
# Should be the last test called
cleanup_world()
{
	[ "$DO_CLEANUP" = "yes" ] && rm -rf $MAIN_TEST_DIR
}

# ---------------------------------------------------------------------------- #

# ----------------- Test Suite ----------------------------------------------- #

# Tests if a program exits with success
test_success()
{
	init_test

	bin="$1"

	LD_LIBRARY_PATH=. $MEMCHECK "$bin" "$TEST_WORK_DIR"
	RUN_RC=$?

	if [ $RUN_RC -eq $MEMCHECK_ERR_CODE ]; then
	    echo "Memcheck failed"
	fi

	basic_test test $RUN_RC -eq 0

	cleanup_test
}

test_fopen_args()
{
	test_success "_test/bin/test_fopen_args"
}

test_fgetc()
{
	test_success "_test/bin/test_fgetc"
}

test_fgetc_large()
{
	test_success "_test/bin/test_fgetc_large"
}

test_fread_small()
{
	test_success "_test/bin/test_fread_small"
}

test_fread_large()
{
	test_success "_test/bin/test_fread_large"
}

test_fread_huge()
{
	test_success "_test/bin/test_fread_huge"
}

test_fread_large_chunked()
{
	test_success "_test/bin/test_fread_large_chunked"
}

test_fread_huge_chunked()
{
	test_success "_test/bin/test_fread_huge_chunked"
}

test_fputc()
{
	test_success "_test/bin/test_fputc"
}

test_fputc_large()
{
	test_success "_test/bin/test_fputc_large"
}

test_fwrite_small()
{
	test_success "_test/bin/test_fwrite_small"
}

test_fwrite_large()
{
	test_success "_test/bin/test_fwrite_large"
}

test_fwrite_huge()
{
	test_success "_test/bin/test_fwrite_huge"
}

test_fwrite_large_chunked()
{
	test_success "_test/bin/test_fwrite_large_chunked"
}

test_fwrite_huge_chunked()
{
	test_success "_test/bin/test_fwrite_huge_chunked"
}

test_append()
{
	test_success "_test/bin/test_append"
}

test_append_read()
{
	test_success "_test/bin/test_append_read"
}

test_fflush()
{
	test_success "_test/bin/test_fflush"
}

test_fread_ftell()
{
	test_success "_test/bin/test_fread_ftell"
}

test_fwrite_ftell()
{
	test_success "_test/bin/test_fwrite_ftell"
}

test_fseek_ftell()
{
	test_success "_test/bin/test_fseek_ftell"
}

test_fseek_fread()
{
	test_success "_test/bin/test_fseek_fread"
}

test_fseek_fwrite()
{
	test_success "_test/bin/test_fseek_fwrite"
}

test_fread_fwrite()
{
	test_success "_test/bin/test_fread_fwrite"
}

test_fwrite_fread()
{
	test_success "_test/bin/test_fwrite_fread"
}

test_feof_fgetc()
{
	test_success "_test/bin/test_feof_fgetc"
}

test_feof_fread()
{
	test_success "_test/bin/test_feof_fread"
}

test_ferror_read_small()
{
	test_success "_test/bin/test_ferror_read_small"
}

test_ferror_read_large()
{
	test_success "_test/bin/test_ferror_read_large"
}

test_ferror_write_small()
{
	test_success "_test/bin/test_ferror_write_small"
}

test_ferror_write_large()
{
	test_success "_test/bin/test_ferror_write_large"
}

test_popen_read()
{
	test_success "_test/bin/test_popen_read"
}

test_popen_write()
{
	test_success "_test/bin/test_popen_write"
}

# ---------------------------------------------------------------------------- #

# ----------------- Run test ------------------------------------------------- #

test_fun_array=(									\
	test_coding_style		"Sources check"				5	\
	test_fopen_args			"Test fopen arguments"			2	\
	test_fgetc			"Test fgetc"				2	\
	test_fgetc_large		"Test fgetc large file"			3	\
	test_fread_small		"Test fread small file"			2	\
	test_fread_large		"Test fread large file"			3	\
	test_fread_large_chunked	"Test chunked fread large file"		3	\
	test_fread_huge			"Test fread huge file"			3	\
	test_fread_huge_chunked		"Test chunked fread huge file"		3	\
	test_fputc			"Test fputc"				2	\
	test_fputc_large		"Test fputc large file"			3	\
	test_fwrite_small		"Test fwrite small file"		2	\
	test_fwrite_large		"Test fwrite large file"		3	\
	test_fwrite_large_chunked	"Test chunked fwrite large file"	3	\
	test_fwrite_huge		"Test fwrite huge file"			3	\
	test_fwrite_huge_chunked	"Test chunked fwrite huge file"		3	\
	test_append			"Test append"				3	\
	test_append_read		"Test append read"			3	\
	test_fflush			"Test fflush"				3	\
	test_fread_ftell		"Test fread ftell"			2	\
	test_fwrite_ftell		"Test fwrite ftell"			2	\
	test_fseek_ftell		"Test fseek ftell"			2	\
	test_fseek_fread		"Test fseek fread"			2	\
	test_fseek_fwrite		"Test fseek fwrite"			2	\
	test_fread_fwrite		"Test fread fwrite"			3	\
	test_fwrite_fread		"Test fwrite fread"			3	\
	test_feof_fgetc			"Test feof fgetc"			2	\
	test_feof_fread			"Test feof fread"			2	\
	test_ferror_read_small		"Test ferror read small"		2	\
	test_ferror_read_large		"Test ferror read large"		3	\
	test_ferror_write_small		"Test ferror write small"		3	\
	test_ferror_write_large		"Test ferror write large"		3	\
	test_popen_read			"Test popen read"			5	\
	test_popen_write		"Test popen write"			5	\
)

# ---------------------------------------------------------------------------- #

# ----------------- Run test ------------------------------------------------- #

# first we check if we have everything defined
check_tests

# run tests
run_tests $@

exit 0
