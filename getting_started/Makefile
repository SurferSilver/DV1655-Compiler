compiler: lex.yy.c parser.tab.o main.cc
	g++ -g -w -ocompiler parser.tab.o lex.yy.c main.cc -std=c++14

AST_symbols: AST_algo.cc Node.h
	g++ -g -w -o AST_symbols AST_algo.cc -std=c++14

semantic: Semantic_Analysis.cc Node.h
	g++ -g -w -o semantic Semantic_Analysis.cc -std=c++14

parser.tab.o: parser.tab.cc
	g++ -g -w -c parser.tab.cc -std=c++14

parser.tab.cc: parser.yy
	bison parser.yy

lex.yy.c: lexer.flex parser.tab.cc
	flex lexer.flex

tree: 
	dot -Tpdf tree.dot -otree.pdf

symbol-table: AST_symbols tree.dot
	@./AST_symbols
	@dot -Tpdf symbol_table.dot -o symbol_table.pdf

# Semantic analysis: ./compiler file.cpm && ./semantic
# Or use: make semantic-check file=test.cpm
semantic-check: compiler semantic
	@./compiler $(file) > /dev/null
	@./semantic

semantic-check-verbose: compiler semantic
	./compiler $(file)
	./semantic

clean:
	rm -f parser.tab.* lex.yy.c* compiler AST_symbols semantic stack.hh position.hh location.hh tree.dot tree.pdf symbol_table.dot symbol_table.pdf
	rm -R compiler.dSYM

test:
	./compiler test.cpm

compile-valid:
	./compiler test_files/valid/test$(test).cpm

test-valid:
	@echo "=== Testing Valid Files ==="
	@passed=0; failed=0; \
	for file in test_files/valid/*.cpm; do \
		echo -n "Testing $$file... "; \
		if ./compiler $$file > /dev/null 2>&1; then \
			echo "✓ PASSED"; \
			passed=$$((passed + 1)); \
		else \
			echo "✗ FAILED"; \
			failed=$$((failed + 1)); \
		fi; \
	done; \
	echo ""; \
	echo "Valid files: $$passed passed, $$failed failed"

test-syntax-errors:
	@echo "=== Testing Syntax Error Files ==="
	@passed=0; failed=0; \
	for file in test_files/syntax_errors/*.cpm; do \
		echo -n "Testing $$file... "; \
		if ./compiler $$file > /dev/null 2>&1; then \
			echo "✓ PASSED"; \
			passed=$$((passed + 1)); \
		else \
			echo "✗ FAILED"; \
			failed=$$((failed + 1)); \
		fi; \
	done; \
	echo ""; \
	echo "Syntax error files: $$passed passed, $$failed failed"

test-semantic-errors:
	@echo "=== Testing Semantic Error Files ==="
	@passed=0; failed=0; \
	for file in test_files/semantic_errors/*.cpm; do \
		echo -n "Testing $$file... "; \
		if ./compiler $$file > /dev/null 2>&1; then \
			echo "✓ PASSED"; \
			passed=$$((passed + 1)); \
		else \
			echo "✗ FAILED"; \
			failed=$$((failed + 1)); \
		fi; \
	done; \
	echo ""; \
	echo "Semantic error files: $$passed passed, $$failed failed"

test-lexical-errors:
	@echo "=== Testing Lexical Error Files ==="
	@passed=0; failed=0; \
	for file in test_files/lexical_errors/*.cpm; do \
		echo -n "Testing $$file... "; \
		if ./compiler $$file > /dev/null 2>&1; then \
			echo "✓ PASSED"; \
			passed=$$((passed + 1)); \
		else \
			echo "✗ FAILED"; \
			failed=$$((failed + 1)); \
		fi; \
	done; \
	echo ""; \
	echo "Lexical error files: $$passed passed, $$failed failed"

test-tree-validation:
	@echo "=== Testing Tree Validation Files ==="
	@passed=0; failed=0; \
	for file in test_files/tree_validation/*.cpm; do \
		echo -n "Testing $$file... "; \
		if ./compiler $$file > /dev/null 2>&1; then \
			echo "✓ PASSED"; \
			passed=$$((passed + 1)); \
		else \
			echo "✗ FAILED"; \
			failed=$$((failed + 1)); \
		fi; \
	done; \
	echo ""; \
	echo "Tree validation files: $$passed passed, $$failed failed"

test-all: test-valid test-syntax-errors test-semantic-errors test-lexical-errors test-tree-validation
	@echo ""
	@echo "=== All Tests Complete ==="