#include <stdlib.h>
#include <stdio.h>
#include <inttypes.h>

#include "compile.h"

const int64_t bytes_per_int = 8;

// corresponds to A
const int64_t ascii_offset = (int64_t)'A';

// keeps track of the number of labels used in implementing else statements
int64_t label_no = 0;


bool compile_ast(node_t *node) {
	switch (node->type) {
		case NUM: {
			int64_t v = (int64_t)(((num_node_t *) node)->value);
			printf("    mov $%ld, %%rax\n", v);
			break;
		}
		case PRINT: {
			node_t *e = ((print_node_t *) node)->expr;
			compile_ast(e);
			printf("    mov %%rax, %%rdi\n");
			printf("    call print_int\n");
			break;
		}
		case VAR: { 
			char n = ((var_node_t *)node)->name;
			int64_t n_ascii = (int64_t)n;
			int64_t diff = n_ascii - ascii_offset; // ascii_offset = 0x41
			printf("    mov -0x%" PRIx64 "(%%rbp), %%rax\n", diff * bytes_per_int);
			break;
		}
		case LET: {
			char n = ((let_node_t *)node)->name;
			int64_t n_ascii = (int64_t)n;
			int64_t diff = n_ascii - ascii_offset;
			compile_ast(((let_node_t*)node)->value);

			printf("    mov %%rax, -0x%" PRIx64 "(%%rbp)\n", diff * bytes_per_int);
			break;
		}
		case BINARY_OP: {
			node_t* l = ((binary_node_t*)node)->left;
			node_t* r = ((binary_node_t*)node)->right;
			compile_ast(l);
			printf("    push %%rax\n");
			compile_ast(r);
			printf("    pop %%rdi\n");
			// After these operations, %rax contains the right operand and 
			// %rdi contains the left operand.
			switch (((binary_node_t*)node)->op) {
				case '+':
					printf("    addq %%rdi, %%rax\n");
					break;
				case '-':
					printf("    subq %%rax, %%rdi\n");
					printf("    mov %%rdi, %%rax\n");
					break;
				case '*':
					printf("    imulq %%rdi, %%rax\n");
					break;
				case '/':
					// Initially: %rax is your divisor; %rdi is your dividend.
					// Moves the divisor to %rsi.
					printf("    mov %%rax, %%rsi\n");

					// Moves the dividend to %rax.
					printf("    mov %%rdi, %%rax\n");

					// 64-bit sign extend.
					printf("    cqto\n");

					// Do the do.
					printf("    idiv %%rsi\n");
					break;
				default:
					// captures all non-arithmetic operations (<, >, =)
					printf("    cmp %%rax, %%rdi\n");
					printf("    mov $0x0, %%rax\n");
					printf("    mov $0x1, %%rsi\n");
					switch (((binary_node_t*)node)->op) {
						case '<':
							printf("    cmovl %%rsi, %%rax\n");
							break;
						case '>':
							printf("    cmovg %%rsi, %%rax\n");
							break;
						case '=':
							printf("    cmove %%rsi, %%rax\n");
							break;
						default:
							return false;
					}
					break;
			}
			break;
		}
		case LABEL: {
			printf("    L%s:\n", ((label_node_t *)node)->label);
			break;
		}
		case GOTO: {
			printf("    jmp L%s\n", ((goto_node_t *)node)->label);
			break;
		}
		case COND: {
			compile_ast(((cond_node_t *)node)->condition);
			printf("    test %%rax, %%rax\n");
			printf("    je C%ld\n", label_no);
			compile_ast(((cond_node_t *)node)->if_branch);
			printf("    C%ld:\n", label_no);
			label_no++;
			break;
		}
		default:
			return false;
	}
	return true;
}
