#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>
#include <ctype.h>

#include "parser.h"

// maxint = -9223372036854775808
#define MAX_KEYWORD_LENGTH 100

typedef struct {
    FILE *stream;
} parser_state_t;

bool is_variable_name(char c) {
    return isupper(c);
}
bool is_number_start(char c) {
    return c == '+' || c == '-' || isdigit(c);
}
bool is_expression_start(char c) {
    return c == '(' || is_variable_name(c) || is_number_start(c);
}
bool is_operator(char c) {
    return c == '+' || c == '-' || c == '*' || c == '/' ||
           c == '<' || c == '=' || c == '>' ||
           c == '(' || c == ')';
}

/*
 * Advances the provided state to the next token.
 */
char advance(parser_state_t *state) {
    int result;

    do {
        result = fgetc(state->stream);
        if (result == EOF) {
            return '\0';
        }
    } while (isspace(result));

    return result;
}

void rewind_one(parser_state_t *state) {
    fseek(state->stream, -1, SEEK_CUR);
}

char peek(parser_state_t *state) {
    char result = advance(state);
    rewind_one(state);
    return result;
}

char *advance_until_separator(parser_state_t *state) {
    char *result = malloc((MAX_KEYWORD_LENGTH + 1) * sizeof(char));
    assert(result);
    size_t index = 0;

    while (true) {
        int c = fgetc(state->stream);
        if (c == EOF) {
            free(result);
            return NULL;
        }

        if (is_operator(c) && index > 0) {
            rewind_one(state);
            c = ' ';
        }
        if (isspace(c)) {
            result[index] = '\0';
            return result;
        }

        result[index++] = c;
    }
}

void skip_line(parser_state_t *state) {
    rewind_one(state);
    while (true) {
        int character = fgetc(state->stream);
        if (character == EOF || character == '\n') {
            break;
        }
    }
}

node_t *expression(parser_state_t *);

node_t *factor(parser_state_t *state) {
    char next = peek(state);
    if (next == '\0') {
        return NULL;
    }
    if (next == '(') {
        advance(state);
        node_t *node = expression(state);
        char next = advance(state);
        if (next != ')') {
            rewind_one(state);
            return NULL;
        }
        return node;
    }
    if (is_variable_name(next)) {
        return init_var_node(advance(state));
    }
    if (is_number_start(next)) {
        return init_num_node(advance_until_separator(state));
    }
    return NULL;
}

node_t *term(parser_state_t *state) {
    node_t *result = factor(state);
    while (true) {
        char next = advance(state);
        if (!(next == '*' || next == '/')) {
            rewind_one(state);
            break;
        }

        result = init_binary_node(next, result, factor(state));
    }
    return result;
}

node_t *expression(parser_state_t *state) {
    if (!is_expression_start(peek(state))) {
        return NULL;
    }

    node_t *result = term(state);
    while (true) {
        char next = advance(state);
        if (!(next == '+' || next == '-')) {
            rewind_one(state);
            break;
        }

        result = init_binary_node(next, result, term(state));
    }
    return result;
}

node_t *comparison(parser_state_t *state) {
    node_t *left = expression(state);
    char op = advance(state);
    if (!(op == '<' || op == '=' || op == '>')) {
        return NULL;
    }

    return init_binary_node(op, left, expression(state));
}

node_t *statement(parser_state_t *state) {
    char *next = advance_until_separator(state);
    if (!next || next[0] == '\0') {
        free(next);
        return NULL;
    }
    if (next[0] == '#') {
        free(next);
        skip_line(state);
        return NULL;
    }

    if (!strcmp("GOTO", next)) {
        free(next);
        return init_goto_node(advance_until_separator(state));
    }
    else if (!strcmp("PRINT", next)) {
        free(next);
        return init_print_node(expression(state));
    }
    else if (!strcmp("LET", next)) {
        free(next);
        char var = advance(state);
        if (isupper(var) && advance(state) == '=') {
            return init_let_node(var, expression(state));
        }
        else {
            return NULL;
        }
    }
    else if (!strcmp("IF", next)) {
        free(next);
        node_t *cond = comparison(state);
        next = advance_until_separator(state);
        if (strcmp(next, "THEN")) {
            free(next);
            return NULL;
        }
        else {
            free(next);
            return init_cond_node(cond, statement(state));
        }
    }
    else {
        return init_label_node(next);
    }
}

node_t *parse(FILE *stream) {
    parser_state_t *state = malloc(sizeof(parser_state_t));
    assert(state);
    state->stream = stream;
    node_t *ast = statement(state);
    free(state);
    return ast;
}
