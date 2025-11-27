/*
TODO:
	1. Add include path directory
	2. Include file only once
*/


#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void *read_entire_file(const char *path, long *len)
{
	FILE *f = fopen(path, "r");
	if (!f) {
		*len = -1;
		perror("Error in opening file");
		return NULL;
	}

	fseek(f, 0L, SEEK_END);
	long file_size = ftell(f);
	fseek(f, 0L, SEEK_SET);

	*len = file_size;

	unsigned char *content = calloc(file_size + 1, sizeof(*content));
	if (!content) {
		fclose(f);
		*len = -1;
		return NULL;
	}

	long length = 0;
	int c;
	while ((c = getc(f)) != EOF)
		content[length++] = c;

	content[length] = '\0';

	*len = length;
	fclose(f);
	return content;
}

char *get_include_path(char *content, long len, int *pos)
{
	int i = *pos;
	int tmp = 0;
	bool found_include = false;
	char *res = NULL;
	int include_offset = 0;
	int include_start = 0;

	while (i < len) {
		char c = content[i++];
		switch (c) {
		case ' ':
			continue;
		case '\n':
			i--;
			goto process;
		case '"': {

			include_start = i;
			while (i < len && found_include && content[i++] != '"')
				include_offset++;

			goto process;
		}
		default: {
			// content is null terminated
			if (c == 'i' &&
			    strncmp(&content[i - 1], "include", 7) == 0) {
				i += 6;
				found_include = true;
			}
		}
		}
	}
process:
	if (found_include && include_offset) {
		res = calloc(include_offset + 1, sizeof(*res));
		assert(res);
		memcpy(res, &content[include_start], include_offset);
		res[include_offset] = '\0';
		*pos = i;
	}
	return res;
}

void unify(const char *file_path)
{
	long len = 0;
	char *content = read_entire_file(file_path, &len);

	if (!len)
		goto cleanup;

	bool is_start_of_line = true;
	int pos = 0;
	char *include_file = NULL;
	while (pos < len) {
		char c = content[pos++];
		switch (c) {
		case '#': {
			if (!is_start_of_line)
				break;

			include_file = get_include_path(content, len, &pos);
			break;
		}
		case '\n':
			is_start_of_line = true;
			break;
		default:
			is_start_of_line = false;
			break;
		}

		if (include_file) {
			unify(include_file);
			free(include_file);
			include_file = NULL;
			continue;
		}
		putc(c, stdout);
	}

cleanup:
	free(content);
}

int main(int argc, char *argv[])
{
	argv++;
	argc--;
	for (int i = 0; i < argc; i++) {
		unify(argv[i]);
	}
	return 0;
}
