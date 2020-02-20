#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <inttypes.h>

#include "ast.h"

node_t *init_num_node(char *num_as_str) {
    if (!num_as_str) {
        return NULL;
    }

    num_node_t *node = malloc(sizeof(num_node_t));
    assert(node);
    node->base.type = NUM;
    node->value = strtol(num_as_str, NULL, 0);
    free(num_as_str);
    return (node_t *) node;
}

node_t *init_binary_node(char op, node_t *left, node_t *right) {
    if (!left || !right) {
        free_ast(left);
        free_ast(right);
        return NULL;
    }

    binary_node_t *node = malloc(sizeof(binary_node_t));
    assert(node);
    node->base.type = BINARY_OP;
    node->op = op;
    node->left = left;
    node->right = right;
    return (node_t *) node;
}

node_t *init_var_node(char name) {
    if (name == '\0') {
        return NULL;
    }

    var_node_t *node = malloc(sizeof(var_node_t));
    assert(node);
    node->base.type = VAR;
    node->name = name;
    return (node_t *) node;
}

node_t *init_print_node(node_t *expr) {
    if (!expr) {
        return NULL;
    }

    print_node_t *node = malloc(sizeof(print_node_t));
    assert(node);
    node->base.type = PRINT;
    node->expr = expr;
    return (node_t *) node;
}

node_t *init_let_node(char name, node_t *value) {
    if (name == '\0' || !value) {
        free_ast(value);
        return NULL;
    }

    let_node_t *node = malloc(sizeof(let_node_t));
    assert(node);
    node->base.type = LET;
    node->name = name;
    node->value = value;
    return (node_t *) node;
}

node_t *init_label_node(char *label) {
    if (!label) {
        return NULL;
    }

    label_node_t *node = malloc(sizeof(label_node_t));
    assert(node);
    node->base.type = LABEL;
    node->label = label;
    return (node_t *) node;
}

node_t *init_goto_node(char *label) {
    if (!label) {
        return NULL;
    }

    goto_node_t *node = malloc(sizeof(goto_node_t));
    assert(node);
    node->base.type = GOTO;
    node->label = label;
    return (node_t *) node;
}

node_t *init_cond_node(node_t *condition, node_t *if_branch) {
    if (!condition || !if_branch) {
        free_ast(condition);
        free_ast(if_branch);
        return NULL;
    }

    cond_node_t *node = malloc(sizeof(cond_node_t));
    assert(node);
    node->base.type = COND;
    node->condition = condition;
    node->if_branch = if_branch;
    return (node_t *) node;
}

void free_ast(node_t *node) {
    if (!node) {
        return;
    }

    if (node->type == BINARY_OP) {
        binary_node_t *bin = (binary_node_t *) node;
        free_ast(bin->left);
        free_ast(bin->right);
    }
    else if (node->type == PRINT) {
        free_ast(((print_node_t *) node)->expr);
    }
    else if (node->type == LET) {
        free_ast(((let_node_t *) node)->value);
    }
    else if (node->type == LABEL) {
        free(((label_node_t *) node)->label);
    }
    else if (node->type == GOTO) {
        free(((goto_node_t *) node)->label);
    }
    else if (node->type == COND) {
        cond_node_t *cond = (cond_node_t *) node;
        free_ast(cond->condition);
        free_ast(cond->if_branch);
    }
    free(node);
}

void print_ast(node_t *node) {
    if (node->type == NUM) {
        fprintf(stderr, "%" PRId64, ((num_node_t *) node)->value);
    }
    else if (node->type == BINARY_OP) {
        binary_node_t *bin = (binary_node_t *) node;
        fprintf(stderr, "%c(", bin->op);
        print_ast(bin->left);
        fprintf(stderr, ", ");
        print_ast(bin->right);
        fprintf(stderr, ")");
    }
    else if (node->type == VAR) {
        fprintf(stderr, "%c", ((var_node_t *) node)->name);
    }
    else if (node->type == PRINT) {
        fprintf(stderr, "PRINT(");
        print_ast(((print_node_t *) node)->expr);
        fprintf(stderr, ")");
    }
    else if (node->type == LET) {
        let_node_t *let = (let_node_t *) node;
        fprintf(stderr, "LET(%c, ", let->name);
        print_ast(let->value);
        fprintf(stderr, ")");
    }
    else if (node->type == LABEL) {
        fprintf(stderr, "LABEL(%s)", ((label_node_t *) node)->label);
    }
    else if (node->type == GOTO) {
        fprintf(stderr, "GOTO(%s)", ((goto_node_t *) node)->label);
    }
    else if (node->type == COND) {
        cond_node_t *cond = (cond_node_t *) node;
        fprintf(stderr, "IF(");
        print_ast(cond->condition);
        fprintf(stderr, ", ");
        print_ast(cond->if_branch);
        fprintf(stderr, ")");
    }
    else {
        fprintf(stderr, "\nUnknown node type: %d\n", node->type);
        exit(1);
    }
}
