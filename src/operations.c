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
	
	char *token = strtok(path_copy, "/");
	token = strtok(NULL, "/");
	if(token == NULL) return 0;


	// Inode used to traverse
	token = strtok(path_copy, "/");
	// printf("%s\n", token);
	int inode_num = fs->root_node;
	inode* inode_ptr = inode_ptr_at_num(fs, inode_num);

	// If path like "/" return -1 bc no parent
	if(token == NULL) return -1;

	char last_token[NAME_MAX_LENGTH];
	last_token[0] = '\0';

	while (token != NULL)
	{
		strcpy(last_token, token);
		// last_token[NAME_MAX_LENGTH - 1] = '\0';

		token = strtok(NULL, "/");
		// printf("dir name to search 4 child: %s\n", inode_ptr->name);
		for(int i = 0; i < DIRECT_BLOCKS_COUNT; i++){
			if(inode_ptr->direct_blocks[i] == -1) continue;
			inode* ptr = inode_ptr_at_num(fs, inode_ptr->direct_blocks[i]);
		}
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
	if(result == NULL) return NULL;
    strcpy(result, last);
    return result;
}

int find_direct_block_with_val(file_system* fs, inode* inode, int val){
	for (int i=0; i<DIRECT_BLOCKS_COUNT; i++) {
		if(inode->direct_blocks[i] == val){
			return i;
		}
	}
	return -1;
}

int
fs_mkdir(file_system *fs, char *path)
{
	size_t size_of_path = strlen(path);
	
	// Get parent inode
	int parent_inode_num = traverse_path_parent(fs, path, size_of_path);

	if(parent_inode_num == -1) return -1;

	inode* parent_inode_ptr = inode_ptr_at_num(fs, parent_inode_num);
	
	// Create child inode
	char* name = get_name(path, size_of_path);

	if(name == NULL || name[0] == '\0') return -1;


	int child_inode_num = find_free_inode(fs);
	inode* child_inode_ptr = inode_ptr_at_num(fs, child_inode_num);

	// Write info to child inode
	inode_init(child_inode_ptr);
	child_inode_ptr->n_type = directory;
	strcpy(child_inode_ptr->name, name);
	child_inode_ptr->parent = parent_inode_num;

	int free_direct_block = find_direct_block_with_val(fs, parent_inode_ptr, -1);
	parent_inode_ptr->direct_blocks[free_direct_block] = child_inode_num;

	free(name);
	return 0;
}

int
fs_mkfile(file_system *fs, char *path_and_name)
{
	size_t size_of_path = strlen(path_and_name);
	
	// Get parent inode
	int parent_inode_num = traverse_path_parent(fs, path_and_name, size_of_path);

	if(parent_inode_num == -1) return -1;

	inode* parent_inode_ptr = inode_ptr_at_num(fs, parent_inode_num);
	
	// Create child inode
	char* name = get_name(path_and_name, size_of_path);

	if(name == NULL || name[0] == '\0') return -1;


	int child_inode_num = find_free_inode(fs);
	inode* child_inode_ptr = inode_ptr_at_num(fs, child_inode_num);

	// Write info to child inode
	inode_init(child_inode_ptr);
	child_inode_ptr->n_type = reg_file;
	strcpy(child_inode_ptr->name, name);
	child_inode_ptr->parent = parent_inode_num;

	int free_direct_block = find_direct_block_with_val(fs, parent_inode_ptr, -1);
	// if(free_direct_block == -1) return -1;
	parent_inode_ptr->direct_blocks[free_direct_block] = child_inode_num;

	free(name);
	return 0;
}

int count_direct_block(inode* inode)
{
	if(inode->n_type == free_block) return -1;

	int count = 0;
	for(int i = 0; i < DIRECT_BLOCKS_COUNT; i++)
	{
		if(inode->direct_blocks[i] != -1) count++;
	}
	return count;
}

int find_free_block(file_system* fs)
{
	int num_blocks = fs->s_block->num_blocks;
	if(num_blocks == 0) return -1;
	for(int i = 0; i < num_blocks; i++)
	{
		if(fs->free_list[i] == 1) return i;
	}
	return -1;
}

data_block* data_block_at_num(file_system* fs, int num)
{
	return &fs->data_blocks[num];
}

int
fs_cp(file_system *fs, char *src_path, char *dst_path_and_name)
{
	// Get size of paths
	size_t size_of_src_path = strlen(src_path);
	size_t size_of_dst_path = strlen(dst_path_and_name);

	// Get src item
	int src_inode_num = traverse_path(fs, src_path, size_of_src_path);
	if(src_inode_num == -1) return -1;

	inode* src_inode = inode_ptr_at_num(fs, src_inode_num);
	
	// Get dst parent
	int dst_parent_inode_num = traverse_path_parent(fs, dst_path_and_name, size_of_dst_path);
	if(dst_parent_inode_num == -1) return -1;
	inode* dst_parent_inode = inode_ptr_at_num(fs, dst_parent_inode_num);

	// Get new name and check for dupe in new parent
	char* new_name = get_name(dst_path_and_name, size_of_dst_path);
	// printf("dest: %s\n", new_name);
	if(find_child_with_name(fs, dst_parent_inode, new_name) != -1)
	{
			printf("chilasadasdad: 248\n");

		free(new_name);
		return -1;
	} 

	// Create new inode
	int new_inode_num = find_free_inode(fs);
	if(new_inode_num == -1)
	{
			printf("chilasadasdad: 258\n");

		free(new_name);
		return -1;
	} 

	inode* new_inode = inode_ptr_at_num(fs, new_inode_num);
	inode_init(new_inode);


	new_inode->n_type = src_inode->n_type;
	new_inode->size = src_inode->size;
	strcpy(new_inode->name, new_name);
	new_inode->parent = dst_parent_inode_num;

	// Add new inode to parent's direct block
	dst_parent_inode->direct_blocks[find_direct_block_with_val(fs, dst_parent_inode, -1)] = new_inode_num;

	if(new_inode->n_type == reg_file)
	{
		int used_blocks = count_direct_block(src_inode);
		if(used_blocks > fs->s_block->free_blocks)
		{
			free(new_inode);
			return -1;
		} 

		for(int i = 0; i < DIRECT_BLOCKS_COUNT; i++)
		{
			if(src_inode->direct_blocks[i] == -1) continue;

			int free_block_num = find_free_block(fs);
			if(free_block_num == -1) 
			{
				free(new_name);
				return -1;
			}
			new_inode->direct_blocks[i] = free_block_num;

			data_block* new_data_block = data_block_at_num(fs, free_block_num);
			data_block* src_data_block = data_block_at_num(fs, src_inode->direct_blocks[i]);

			new_data_block->size = src_data_block->size;
			memcpy(new_data_block->block, src_data_block->block, BLOCK_SIZE);
		}

		fs->s_block->free_blocks -= used_blocks;
	} 
	else if(new_inode->n_type == directory)
	{
		for(int i = 0; i < DIRECT_BLOCKS_COUNT; i++)
		{
			if(src_inode->direct_blocks[i] == -1) continue;
			inode* child = inode_ptr_at_num(fs, src_inode->direct_blocks[i]);

			// printf("%s\n", child->name);
			char child_src_path[256];
			snprintf(child_src_path, sizeof(child_src_path), "%s/%s", src_path, child->name);


			char child_dst_path[256];
			snprintf(child_dst_path, sizeof(child_dst_path), "%s/%s", dst_path_and_name, child->name);


			if (fs_cp(fs, child_src_path, child_dst_path) == -1) {
    			free(new_name);
    			return -1;
			}
		}
	}
	
	free(new_name);
	return 0;
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
