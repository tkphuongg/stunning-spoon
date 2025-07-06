#include "../lib/operations.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

inode* inode_ptr_at_num(file_system* fs, int num){
	return &fs->inodes[num];
}

int find_child_with_name(file_system* fs, inode* parent, const char* name)
{
	// Check if parent is a directory
	if(parent->n_type != directory) return -1;

	for(int i = 0; i < DIRECT_BLOCKS_COUNT; i++)
	{
		int child_inode_num = parent->direct_blocks[i];

		// Check other child if no children at this block
		if(child_inode_num == -1) continue;

		inode* child = inode_ptr_at_num(fs, child_inode_num);
		if(child == NULL) continue;

		// Return child number if name matches
		if(strcmp(child->name, name) == 0)
		{
			return child_inode_num;
		}
	}

	return -1;
}

int traverse_path(file_system *fs, const char *path, size_t size_of_path)
{
	// Check if path exists or in correct format
	if(path == NULL || path[0] != '/') return -1;

	int inode_num = fs->root_node;
	inode* inode_ptr = inode_ptr_at_num(fs, inode_num);

	char path_copy[size_of_path + 1];
	strcpy(path_copy, path);

	char* token = strtok(path_copy, "/");

	while (token != NULL)
	{
		inode_num = find_child_with_name(fs, inode_ptr, token);
		if(inode_num == -1) return -1;
		inode_ptr = inode_ptr_at_num(fs, inode_num);
		token = strtok(NULL, "/");
	}
	
	return inode_num;
}

int traverse_path_parent(file_system *fs, const char *path, size_t size_of_path)
{
	// Check if path exists or in correct format
	if(path == NULL || path[0] != '/') return -1;

	// Copy path
	char path_copy[size_of_path + 1];
    strcpy(path_copy, path);

	// Inode used to traverse
	char *token = strtok(path_copy, "/");
	int inode_num = fs->root_node;
	inode* inode_ptr = inode_ptr_at_num(fs, inode_num);

	// If path like "/" return -1 bc no parent
	if(token == NULL) return -1;

	char last_token[NAME_MAX_LENGTH];
	last_token[0] = '\0';

	while (token != NULL)
	{
		strncpy(last_token, token, sizeof(last_token));
		last_token[NAME_MAX_LENGTH - 1] = '\0';

		// theres no next token, return inode_num(parent)
		if(token == NULL) return inode_num;

		inode_num = find_child_with_name(fs, inode_ptr, last_token);
		if(inode_num == -1) return -1;
		inode_ptr = inode_ptr_at_num(fs, inode_num);
	}
	
	return inode_num;
}

char* get_name(const char* path, size_t size_of_path)
{
	char path_copy[size_of_path + 1];
    strcpy(path_copy, path);

	char* token = strtok(path_copy, "/");
    char* last = token;

	while (token != NULL)
	{
		last = token;
		token = strtok(NULL, "/");
	}

	char* result = malloc(strlen(last) + 1);
    strcpy(result, last);
    return result;
}

int
fs_mkdir(file_system *fs, char *path)
{
	char name = get_name(path, strlen(path));

	free(name);
	return -1;
}

int
fs_mkfile(file_system *fs, char *path_and_name)
{
	return -1;
}

int
fs_cp(file_system *fs, char *src_path, char *dst_path_and_name)
{
	return -1;
}

char *
fs_list(file_system *fs, char *path)
{
	return NULL;
}

int
fs_writef(file_system *fs, char *filename, char *text)
{
	return -1;
}

uint8_t *
fs_readf(file_system *fs, char *filename, int *file_size)
{
	return NULL;
}


int
fs_rm(file_system *fs, char *path)
{
	return -1;
}

int
fs_import(file_system *fs, char *int_path, char *ext_path)
{
	return -1;
}

int
fs_export(file_system *fs, char *int_path, char *ext_path)
{
	return -1;
}
