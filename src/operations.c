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

		// Return child number if name matches
		if(strcmp(child->name, name) == 0)
		{
			return child_inode_num;
		}
	}

	return -1;
}

int traverse_path(file_system *fs, const char *path)
{
	// Check if path exists or in correct format
	if(path == NULL || path[0] != '/') return -1;

	int inode_num = fs->root_node;
	inode* inode_ptr = inode_ptr_at_num(fs, inode_num);

	char* token;
	strcpy(token, path);
	token = strtok(token, "/");

	while (token != NULL)
	{
		inode_num = find_child_with_name(fs, inode_ptr, token);
		if(inode_num == -1) return -1;
		inode_ptr = inode_ptr_at_num(fs, inode_num);
	}
	
	return inode_num;
}

int traverse_path_parent(file_system *fs, const char *path)
{
	// Check if path exists or in correct format
	if(path == NULL || path[0] != '/') return -1;

	// Parent and child token
	char* p_tok, c_tok, parent; 
	strcpy(p_tok, path);
	strcpy(c_tok, path);

	// Inode used to traverse
	int inode_num = fs->root_node;
	inode* inode_ptr = inode_ptr_at_num(fs, inode_num);

	p_tok = strtok(p_tok, "/");
	if(p_tok == NULL) return -1;
	c_tok = strtok(c_tok, "/");
	if(c_tok == NULL) return inode_num;

	while (c_tok != NULL)
	{
		strcpy(parent, p_tok);
		p_tok = strtok(NULL, "/");
		c_tok = strtok(NULL, "/");

		inode_num = find_child_with_name(fs, inode_ptr, p_tok);
		if(inode_num == -1) return -1;
		inode_ptr = inode_ptr_at_num(fs, inode_num);
	}
	
	return inode_num;
}

char* get_name(char* path)
{
	char* token, name;
	strcpy(token, path);
	token = strtok(token, "/");
	while (token != NULL)
	{
		strcpy(name, token);
		token = strtok(NULL, "/");
	}
	return name;
}

int
fs_mkdir(file_system *fs, char *path)
{
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
