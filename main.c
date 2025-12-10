#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <sys/stat.h>

typedef struct 
{
	int capacity;
	int len;
	char** items;
} StrArray;


#define array_push(arr,item)\
do{\
	if((arr)->capacity == (arr)->len){\
		int new_capacity = (arr)->capacity<<1;\
		(arr)->items = realloc((arr)->items,sizeof(*(arr)->items)*new_capacity);\
		assert((arr)->items);\
		(arr)->capacity  = new_capacity;\
	}\
	(arr)->items[(arr)->len++] = item;\
		\
} while(0)

StrArray seen = {0};
StrArray search_path = {0};
StrArray file_list = {0};

int max_dir_len  = 0;

typedef enum{
	PATH_FILE,
	PATH_DIR,
	PATH_INVALID,
}path_t;

path_t path_type(const char *p)
{
    struct stat s;
    if (stat(p, &s) != 0) return PATH_INVALID;
    if (S_ISREG(s.st_mode)) return PATH_FILE;
    if (S_ISDIR(s.st_mode)) return PATH_DIR;
    return PATH_INVALID;
}

void *read_entire_file(const char *path, long *len)
{
	FILE *f = fopen(path, "r");

	if (!f) {
		*len = -1;
		fprintf(stderr,"%s, %s\n",path,strerror(errno));
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
	char* content = NULL; 

	int buffer_len = max_dir_len + strlen(file_path) + 2;
	char* buffer = calloc(buffer_len,sizeof(*buffer));
	assert(buffer);
	
	switch(path_type(file_path)){
		case PATH_DIR:
			fprintf(stderr,"%s is Dir, expected file. Skipping\n",file_path);
			break;
		case PATH_FILE:
			content = read_entire_file(file_path, &len);
			break;
		default:
		{
			for(int i=0;i<search_path.len;i++){

				snprintf(buffer,buffer_len,"%s/%s",search_path.items[i],file_path);
				if(path_type(buffer)==PATH_FILE){
					content = read_entire_file(buffer, &len);
					break;
				}
			}

			if(content==NULL)
				fprintf(stderr,"%s cannot find this file. Skipping..\n",file_path);
		}
	}

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
			for(int idx = 0;idx<seen.len;idx++)
				if(strcmp(seen.items[idx],include_file)==0){
					free(include_file);
					goto skip;
				}

			array_push(&seen,include_file);
			unify(include_file);
		skip:
			include_file = NULL;
			continue;
		}
		putc(c, stdout);
	}

cleanup:
	free(buffer);
	free(content);
}

int main(int argc, char *argv[])
{
	if(argc == 1){
		printf("Uses: unify file1.c file2.c .... filen.c\n");
		return 0;
	}

	argv++;
	argc--;

	const int n = 1<<10;

	seen.items = calloc(n,sizeof(*seen.items));
	assert(seen.items);
	seen.capacity = n;

	search_path.items = calloc(n,sizeof(*search_path.items));
	assert(search_path.items);
	search_path.capacity = n;

	file_list.items = calloc(argc,sizeof(*file_list.items));
	assert(file_list.items);
	file_list.capacity = argc;

	for (int i = 0; i < argc; i++) {
		if(strcmp(argv[i],"-I")==0){
			i++;
			if(i==argc){
				fprintf(stderr,"Expected path, got nothing.\n");
				return 0;
			}

			if(!(path_type(argv[i])==PATH_DIR))
				fprintf(stderr,"%s is not valid path, skiping..\n",argv[i]);
			else{
				array_push(&search_path,argv[i]);
				const int path_len = strlen(argv[i]);
				max_dir_len = (max_dir_len < path_len)?path_len:max_dir_len;
			}
		} else
			array_push(&file_list,argv[i]);
	}

	for(int i = 0;i<file_list.len;i++)
		unify(file_list.items[i]);

	for(int i = 0;i<seen.len;i++)
		free(seen.items[i]);

	free(seen.items);
	free(search_path.items);
	free(file_list.items);

	return 0;
}
