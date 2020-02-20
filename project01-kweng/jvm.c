#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>

#include "jvm.h"
#include "read_class.h"

/** The name of the method to invoke to run the class file */
const char *MAIN_METHOD = "main";
/**
 * The "descriptor" string for main(). The descriptor encodes main()'s signature,
 * i.e. main() takes a String[] and returns void.
 * If you're interested, the descriptor string is explained at
 * https://docs.oracle.com/javase/specs/jvms/se12/html/jvms-4.html#jvms-4.3.2.
 */
const char *MAIN_DESCRIPTOR = "([Ljava/lang/String;)V";

/** 
 * Used in arithmetic instructions.
 */
int32_t add(int32_t a, int32_t b) {
	return a + b;
}

int32_t subtract(int32_t a, int32_t b) {
	return a - b;
}

int32_t multiply(int32_t a, int32_t b) {
	return a * b;
}

int32_t divide(int32_t a, int32_t b) {
	return a / b;
}

int32_t remain(int32_t a, int32_t b) {
	return a % b;
}

int32_t (*arithmetic[5])(int32_t a, int32_t b) =
	{add, subtract, multiply, divide, remain};

/**
 * Used in jump instructions.
 *
 * Note re: conditional_jump: The constant -1 applied to every pc increment
 * (given a successful jump) is an offset for pc++, which lies outside the
 * switch statement but within the while loop.
 */
size_t conditional_jump(bool condition, method_t* method, size_t pc) {
	if (condition) {
		pc += (int16_t)((method->code.code[pc + 1] << 8) |
			method->code.code[pc + 2]) - 1;
	}
	else {
		pc += 2;
	}
	return pc;
}
bool eq(int32_t a, int32_t b) {
	return a == b;
}
bool ne(int32_t a, int32_t b) {
	return a != b;
}
bool lt(int32_t a, int32_t b) {
	return a < b;
}
bool ge(int32_t a, int32_t b) {
	return a >= b;
}
bool gt(int32_t a, int32_t b) {
	return a > b;
}
bool le(int32_t a, int32_t b) {
	return a <= b;
}
bool (*compare[6])(int32_t a, int32_t b) = {eq, ne, lt, ge, gt, le};

/**
 * Used in the switch statement in execute() to avoid usage of magic numbers.
 */
const uint8_t const_offset = 0x3;
const uint8_t load_offset = 0x1a;
const uint8_t store_offset = 0x3b;
const uint8_t arithmetic_offset = 0x60;
const uint8_t jump_zero_offset = 0x99;
const uint8_t jump_offset = 0x9f;

/**
 * Runs a method's instructions until the method returns.
 *
 * @param method the method to run
 * @param locals the array of local variables, including the method parameters.
 *   Except for parameters, the locals are uninitialized.
 * @param class the class file the method belongs to
 * @return if the method returns an int, a heap-allocated pointer to it;
 *   if the method returns void, NULL
 */
int32_t *execute(method_t *method, int32_t *locals, class_file_t *class) {

	size_t size = 0;
	int32_t stack[method->code.max_stack];
	size_t pc = 0;

	while (method->code.code[pc] != i_return) {
		switch (method->code.code[pc]) {
			/**
			 * Constant instructions
			 *
			 * Note: An offset is used to allow for simplifying implementation.
			 * This is because i_iconst_0 corresponds to opcode 0x3.
			 */
			case i_iconst_m1:
			case i_iconst_0:
			case i_iconst_1:
			case i_iconst_2:
			case i_iconst_3:
			case i_iconst_4:
			case i_iconst_5:
				stack[size++] = method->code.code[pc] - const_offset;
				break;

			/**
			 * Pushing instructions
			 */
			case i_bipush:
				stack[size++] = (int8_t)method->code.code[++pc];
				break;
			case i_sipush:
				stack[size++] = (int16_t)(method->code.code[pc + 1] << 8) |
					(int16_t)(method->code.code[pc + 2]);
				pc += 2;
				break;
			case i_ldc: {
				cp_info* constant_pool = get_constant(&class->constant_pool,
					method->code.code[++pc]);
				CONSTANT_Integer_info *info =
					(CONSTANT_Integer_info *)constant_pool->info;
				stack[size++] = info->bytes;
				break;
			}

			/** 
			 * Local variable instructions
			 *
			 * Note: Offsets are used to simplify implementation because
			 * i_iload_0 corresponds to opcode 0x1a and i_istore_0
			 * corresponds to opcode 0x3b.
			 */
			case i_iload:
				stack[size++] = locals[method->code.code[++pc]];
				break;
			case i_iload_0:
			case i_iload_1:
			case i_iload_2:
			case i_iload_3:
				stack[size++] = locals[method->code.code[pc] - load_offset];
				break;
			case i_istore:
				locals[method->code.code[++pc]] = stack[--size];
				break;
			case i_istore_0:
			case i_istore_1:
			case i_istore_2:
			case i_istore_3:
				locals[method->code.code[pc] - store_offset] = stack[--size];
				break;

			/**
			 * Arithmetic instructions
			 * 
			 * Abstracted out using an array of function pointers.
			 * Note: An offset is used to simplify implementation because
			 * i_iadd corresponds to opcode 0x60.
			 */
			case i_iadd:
			case i_isub:
			case i_imul:
			case i_idiv:
			case i_irem:
				stack[size - 2] = 
					arithmetic[(method->code.code[pc] - arithmetic_offset) / 4]
					(stack[size - 2], stack[size - 1]);
				size--;
				break;
			case i_ineg:
				stack[size - 1] = stack[size - 1] * -1;
				break;
			case i_iinc:
				locals[method->code.code[pc + 1]] += 
					(int8_t)method->code.code[pc + 2];
				pc += 2;
				break;

			/**
			 * Jump instructions
			 *
			 * Abstracted out using an array of function pointers.
			 * Note: Offsets are used to simplify implementation because i_ifeq
			 * corresponds to opcode 0x99 and i_if_icmple corresponds to opcode
			 * 0x9f.
			 */
			case i_ifeq:
			case i_ifne:
			case i_iflt:
			case i_ifge:
			case i_ifgt:
			case i_ifle:
				pc = conditional_jump(
					compare[method->code.code[pc] - jump_zero_offset](stack[--size], 0),
					method, pc);
				break;
			case i_if_icmpeq:
			case i_if_icmpne:
			case i_if_icmplt:
			case i_if_icmpge:
			case i_if_icmpgt:
			case i_if_icmple: {
				int32_t arg1 = stack[size - 2];
				int32_t arg2 = stack[size - 1];
				pc = conditional_jump(
					compare[method->code.code[pc] - jump_offset](arg1, arg2),
					method, pc);
				size -= 2;
				break;
			}
			case i_goto:
				pc = conditional_jump(1, method, pc);
				break;

			case i_ireturn: {
				int32_t* toReturn = (int32_t*)malloc(sizeof(int32_t));
				assert(toReturn != NULL);
				*toReturn = stack[--size];
				return toReturn;
				break;
			}
			case i_getstatic:
				pc += 2;
				break;
			case i_invokevirtual:
				printf("%d\n", stack[--size]);
				pc += 2;
				break;
			case i_invokestatic: {
				// Resolve the provided unsigned index into a constant pool.
				// Use this to find the next method.
				uint16_t index = (method->code.code[pc + 1] << 8) |
					(method->code.code[pc + 2]);
				method_t * next_method = find_method_from_index(index, class);

				// Set up the locals array of the callee and transfer arguments
				// from the operand stack of the caller to the new locals array.
				int32_t* locals_new = malloc(sizeof(int32_t) * 
					next_method->code.max_locals);
				uint16_t params = get_number_of_parameters(next_method);
				for (uint16_t i = 0; i < params; i++) {
					locals_new[params - 1 - i] = stack[--size];
				}

				// Recursively execute code of the next method.
				int32_t *returned_value = execute(next_method, locals_new, class);
				free(locals_new);
				pc += 2;

				// If the return value exists, push it to the top of the stack.
				if (returned_value != NULL) {
					stack[size++] = *returned_value;
					free(returned_value);
				}
				break;
			}
			default:
				fprintf(stderr, "Unrecognized opcode.\n");
				break;
		}
		pc++;
	}
    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "USAGE: %s <class file>\n", argv[0]);
        return 1;
    }

    // Open the class file for reading
    FILE *class_file = fopen(argv[1], "r");
    assert(class_file && "Failed to open file");

    // Parse the class file
    class_file_t class = get_class(class_file);
    int error = fclose(class_file);
    assert(!error && "Failed to close file");

    // Execute the main method
    method_t *main_method = find_method(MAIN_METHOD, MAIN_DESCRIPTOR, &class);
    assert(main_method && "Missing main() method");
    /* In a real JVM, locals[0] would contain a reference to String[] args.
     * But since TeenyJVM doesn't support Objects, we leave it uninitialized. */
    int32_t locals[main_method->code.max_locals];

    int32_t *result = execute(main_method, locals, &class);
    assert(!result && "main() should return void");

    // Free the internal data structures
    free_class(&class);
}
