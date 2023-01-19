.POSIX:
.SUFFIXES:
.SUFFIXES: .c .unittest .profraw .profdata .coverage

CC       = clang
CPROF    = xcrun llvm-profdata
CCOV     = xcrun llvm-cov

SANITIZE = -fsanitize=address -fsanitize=undefined-trap -fsanitize-undefined-trap-on-error
COVERAGE = -fprofile-instr-generate -fcoverage-mapping
OPTS     = $(SANITIZE) $(COVERAGE) -Werror -Weverything -Wno-padded -Wno-poison-system-directories
DEFS     = -DNO_COBS_STRICT

.PHONY: all
all: cobs.coverage

cobs.unittest: test_cobs.c

.profdata.coverage:
	$(CCOV) show $*.unittest -instr-profile=$< $*.c > $@
	! grep " 0|" $@
	echo PASS $@

.profraw.profdata:
	$(CPROF) merge -sparse $< -o $@

.unittest.profraw:
	LLVM_PROFILE_FILE=$@ ./$<

.c.unittest:
	$(CC) $(OPTS) $(DEFS) $^ -o $@

.PHONY: clean
clean:
	rm -rf *.coverage *.profdata *.profraw *.unittest*
