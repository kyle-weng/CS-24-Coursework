CC = clang-with-asan
CFLAGS = -Iinclude -Wall -Wextra
ASM = clang

TESTS_1 =            $(wildcard progs/stage1-*.bas)
TESTS_2 = $(TESTS_1) $(wildcard progs/stage2-*.bas)
TESTS_3 = $(TESTS_2) $(wildcard progs/stage3-*.bas)
TESTS_4 = $(TESTS_3) $(wildcard progs/stage4-*.bas)
TESTS_5 = $(TESTS_4) $(wildcard progs/stage5-*.bas)
TESTS_6 = $(TESTS_5) $(wildcard progs/stage6-*.bas)
TESTS_7 = $(TESTS_6) $(wildcard progs/stage7-*.bas)

test: test7
test1: $(TESTS_1:progs/%.bas=%-result)
test2: $(TESTS_2:progs/%.bas=%-result)
test3: $(TESTS_3:progs/%.bas=%-result)
test4: $(TESTS_4:progs/%.bas=%-result)
test5: $(TESTS_5:progs/%.bas=%-result)
test6: $(TESTS_6:progs/%.bas=%-result)
test7: $(TESTS_7:progs/%.bas=%-result)

out/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

bin/compiler: out/ast.o out/compile.o out/compiler.o out/parser.o
	$(CC) $(CFLAGS) $^ -o $@

out/%.s: progs/%.bas bin/compiler
	bin/compiler $< > $@

bin/%: out/%.s
	$(ASM) -g $< -o $@

progs/%-expected.txt: progs/%.bas
	grep '^#' $< | sed -e 's/#//' > $@

progs/%-actual.txt: bin/%
	$< > $@

%-result: progs/%-expected.txt progs/%-actual.txt
	diff -u $^ && echo PASSED test $(@F:-result=). || (echo FAILED test $(@F:-result=). Aborting.; false)

clean:
	rm -f out/* bin/* progs/*.txt

.PRECIOUS: out/%.o out/%.s bin/% progs/%-expected.txt progs/%-actual.txt
