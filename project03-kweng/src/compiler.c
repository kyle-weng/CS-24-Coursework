#include <stdlib.h>
#include <stdio.h>
#include <inttypes.h>

#include "parser.h"
#include "compile.h"

void usage(char *program) {
    fprintf(stderr, "USAGE: %s <program file>\n", program);
    exit(1);
}

/**
 * Prints the start of the the x86-64 assembly output.
 * The assembly code implementing the TeenyBASIC statements
 * goes between the header and the footer.
 */
void header(void) {
    puts(
        "# Declares a string constant to pass to printf()\n"
        ".section .rodata\n"
        "intfmt:\n"
        "    .string \"%" PRId64 "\\n\"\n"
        "\n"
        "# The code section of the assembly file\n"
        ".text\n"
        "print_int:\n"
        "    # Calls printf(intfmt, %rdi)\n"
        "    movq %rdi, %rsi\n"
        "    leaq intfmt(%rip), %rdi # LABEL(%rip) computes the address of LABEL relative to %rip\n"
        "    movb $0, %al # %al stores the number of float arguments to printf()\n"
        "    call printf@plt\n"
        "    ret\n"
        "\n"
        ".globl main\n"
        "main:\n"
        "    # The main() function\n"
		"    push %rbp\n"
		"    mov %rsp, %rbp\n"
		"    sub $0xd0, %rsp"
    );
}

/**
 * Prints the end of the x86-64 assembly output.
 * The assembly code implementing the TeenyBASIC statements
 * goes between the header and the footer.
 */
void footer(void) {
    puts(
		"    add $0xd0, %rsp\n"
		"    pop %rbp\n"
        "    movl $0, %eax # return 0 from main()\n"
        "    ret"
    );
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        usage(argv[0]);
    }

    FILE *program = fopen(argv[1], "r");
    if (!program) {
        usage(argv[0]);
    }

    header();

    while (!feof(program)) {
        node_t *ast = parse(program);
        if (ast) { // skip comments; only compile statements
            // Display the AST for debugging purposes
            print_ast(ast);
            fprintf(stderr, "\n");

            // Compile the AST into assembly instructions
            if (!compile_ast(ast)) {
                fprintf(stderr, "Compilation Error.\n");
                fclose(program);
                exit(3);
            }
            free_ast(ast);
        }
    }
    fclose(program);

    footer();
}
