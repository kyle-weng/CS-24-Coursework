#ifndef AST_H
#define AST_H

/**
 * Definitions for the abstract syntax tree representation of TeenyBASIC.
 * Parsing source code into an AST allows us to traverse it in a structured way.
 * The AST is a recursive data structure consisting of several types of "nodes".
 */

#include <stdint.h>

/** The types of AST nodes */
typedef enum {
    NUM,
    BINARY_OP,
    VAR,
    PRINT,
    LET,
    LABEL,
    GOTO,
    COND
} node_type_t;

/** The base struct for all nodes */
typedef struct {
    /**
     * The node's type. Determines how to interpret the node.
     * For example, if `node->type == BINARY_OP`,
     * then `node` can be cast to a `binary_node_t *`.
     */
    node_type_t type;
} node_t;

/** A number literal */
typedef struct {
    node_t base;
    /** The value of the literal */
    int64_t value;
} num_node_t;

/**
 * An expression representing a binary operation or comparison.
 * For example, (1 + 2) would be represented as a binary_node_t with:
 *   op == '+'
 *   ((num_node_t *) left)->value == 1
 *   ((num_node_t *) right)->value == 2
 */
typedef struct {
    node_t base;
    /** The operator, either '+', '-', '*', '/', '<', '=', or '> */
    char op;
    /** The left-hand side of the expression */
    node_t *left;
    /** The right-hand side of the expression */
    node_t *right;
} binary_node_t;

/** An expression that evaluates a variable */
typedef struct {
    node_t base;
    /** The variable whose value to read ('A' to 'Z') */
    char name;
} var_node_t;

/** A PRINT statement */
typedef struct {
    node_t base;
    /** The expression to evaluate and print */
    node_t *expr;
} print_node_t;

/** A LET statement */
typedef struct {
    node_t base;
    /** The variable to assign a value to ('A' to 'Z') */
    char name;
    /** The expression to evaluate and store in the variable */
    node_t *value;
} let_node_t;

/** A label */
typedef struct {
    node_t base;
    /** The text of a label (e.g. "00", "10", etc. in the primes program) */
    char *label;
} label_node_t;

/** A GOTO statement */
typedef struct {
    node_t base;
    /** The label to jump to (e.g. "80" in the primes program) */
    char *label;
} goto_node_t;

/** An IF statement */
typedef struct {
    node_t base;
    /** The condition to check, a binary_op_t with operator '<', '=', or '>' */
    node_t *condition;
    /** The statement to run if the condition evaluates to true */
    node_t *if_branch;
} cond_node_t;

/** Constructs a num_node_t */
node_t *init_num_node(char *num_as_str);

/** Constructs a binary_node_t */
node_t *init_binary_node(char op, node_t *left, node_t *right);

/** Constructs a var_node_t */
node_t *init_var_node(char name);

/** Constructs a print_node_t */
node_t *init_print_node(node_t *expr);

/** Constructs a let_node_t */
node_t *init_let_node(char name, node_t *value);

/** Constructs a label_node_t */
node_t *init_label_node(char *label);

/** Constructs a goto_node_t */
node_t *init_goto_node(char *label);

/** Constructs a cond_node_t */
node_t *init_cond_node(node_t *condition, node_t *if_branch);

/** Frees an AST node and all its descendants */
void free_ast(node_t *node);

/** Prints a string representation of an AST node to stderr */
void print_ast(node_t *node);

#endif /* AST_H */
