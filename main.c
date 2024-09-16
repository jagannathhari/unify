#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#define IMPLEMENT_VECTOR
#include "./vector.h"

#define SPACE ' '
#define INVERTED_COMMA '\"'

typedef char String;

typedef enum {
    LOCAL_HEADER,
    SYSTEM_HEADER,
    NOT_A_HEADER
} HeaderType;

typedef struct {
    HeaderType type;
    String *path;
} HeaderInfo;

static String *read_line(FILE *f) {
    String *buffer = Vector(*buffer);
    char c;
    while ((c = fgetc(f)) != EOF && c != '\n') {
        vector_append(buffer, (char)c);
    }
    if (c == EOF) {
        return NULL;
    }
    vector_append(buffer, '\0');
    return buffer;
}

static String **read_lines(FILE *f) {
    String **lines = Vector(*lines);
    String *line = Vector(*line);
    while ((line = read_line(f))) {
        vector_append(lines, line);
    }
    return lines;
}

static bool is_include(const String *buffer, HeaderInfo *header) {
#define REMOVE_SPACE(buffer, len) \
    while (*buffer == SPACE) {    \
        buffer++;                 \
        len--;                    \
    }

#define CONSUME_N_CHAR(buffer, len, n) \
    do {                               \
        if (len >= n) {                \
            buffer += n;               \
            len -= n;                  \
        } else {                       \
            goto not_a_valid_header;   \
        }                              \
    } while (0)

    if (!header || !buffer) {
        return false;
    }

    String *header_name = Vector(*header_name);

    if (!header_name) {
        perror("Unable to allocate memory");
        goto not_a_valid_header;
    }

    size_t len = vector_length(buffer) - 1;

    REMOVE_SPACE(buffer, len);

    if (len < 11 || !(*buffer == '#')) {
        goto not_a_valid_header;
    }

    CONSUME_N_CHAR(buffer, len, 1);

    REMOVE_SPACE(buffer, len);
    if (len < 10) {
        goto not_a_valid_header;
    }

    if (strncmp(buffer, "include", 7) != 0) {
        goto not_a_valid_header;
    }

    CONSUME_N_CHAR(buffer, len, 7);
    REMOVE_SPACE(buffer, len);
    if (len < 3) {
        goto not_a_valid_header;
    }
    char opposite;
    switch (*buffer) {
    case INVERTED_COMMA:
        opposite = INVERTED_COMMA;
        break;
    case '<':
        opposite = '>';
        break;
    default:
        goto not_a_valid_header;
    }
    CONSUME_N_CHAR(buffer, len, 1);
    while (*buffer) {
        if (*buffer == opposite) {
            vector_append(header_name, '\0');
            header->type = (opposite == '>') ? SYSTEM_HEADER : LOCAL_HEADER;
            header->path = header_name;
            return true;
        }
        vector_append(header_name, *buffer++);
    }

not_a_valid_header:
    header->type = NOT_A_HEADER;
    header->path = NULL;
    free_vector(header_name);
    return false;
}

bool expand_local_header(FILE *dest, char *src) {
#define GET_NODES(src)                              \
    do {                                            \
        input_file = fopen(src, "r");               \
        if (!input_file) {                          \
            perror("Unable to readfile: ");         \
            success = false;                        \
            goto cleanup;                           \
        }                                           \
        temp = read_lines(input_file);              \
        fclose(input_file);                         \
        while (vector_length(temp)) {               \
            vector_append(stack, vector_pop(temp)); \
        }                                           \
        free_vector(temp);                          \
        temp = NULL;                                \
    } while (0)
    bool success = true;
    FILE *input_file = fopen(src, "r");
    String **stack = Vector(*stack);
    String **temp = NULL;
    String *line = NULL;
    HeaderInfo headerinfo;
    GET_NODES(src);
    while (vector_length(stack)) {
        line = vector_pop(stack);
        if (is_include(line, &headerinfo) && headerinfo.type == LOCAL_HEADER) {
            GET_NODES(headerinfo.path);
        } else {
            fputs(line, dest);
            fputc('\n', dest);
            free_vector(line);
            line = NULL;
        }
    }
cleanup:
    for (size_t i = 0; i < vector_length(stack); i++) {
        free_vector(stack[i]);
    }
    free_vector(stack);
    free_vector(temp);
    free_vector(line);
    return success;
}

void merge_file(FILE *dest, String **sources) {
    size_t len = vector_length(sources);
    for (size_t i = 0; i < len; i++) {
        if (!expand_local_header(dest, sources[i])) {
            fprintf(stderr, "[ERROR] Unable to expand %s", sources[i]);
        }
    }
}

int main(int argc, char **argv) {
    if (argc < 2) {
        printf("USES: unify file1 file2 ....flien");
        return 0;
    }

    String **sources = Vector(*sources);

    // shift argument by 1
    argv++;
    argc--;

    for (int i = 0; i < argc; i++) {
        vector_append(sources, argv[i]);
    }

    merge_file(stdout, sources);
    free_vector(sources);

    return 0;
}
