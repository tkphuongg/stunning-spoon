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
	int inode_num = fs->root_node;
	inode* inode_ptr = inode_ptr_at_num(fs, inode_num);

	// If path like "/" return -1 bc no parent
	if(token == NULL) return -1;

	char last_token[NAME_MAX_LENGTH];
	last_token[0] = '\0';

	while (token != NULL)
	{
		strcpy(last_token, token);
		token = strtok(NULL, "/");
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
		free(new_name);
		return -1;
	} 

	// Create new inode
	int new_inode_num = find_free_inode(fs);
	if(new_inode_num == -1)
	{
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
			fs->free_list[free_block_num] = 0;
		}

		fs->s_block->free_blocks -= used_blocks;
	} 
	else if(new_inode->n_type == directory)
	{
		for(int i = 0; i < DIRECT_BLOCKS_COUNT; i++)
		{
			if(src_inode->direct_blocks[i] == -1) continue;
			inode* child = inode_ptr_at_num(fs, src_inode->direct_blocks[i]);

			char child_src_path[256];
			snprintf(child_src_path, sizeof(child_src_path), "%s/%s", src_path, child->name);


			char child_dst_path[256];
			snprintf(child_dst_path, sizeof(child_dst_path), "%s/%s", dst_path_and_name, child->name);

			if (fs_cp(fs, child_src_path, child_dst_path) == -1) 
			{
    			free(new_name);
    			return -1;
			}
		}
	}
	
	free(new_name);
	return 0;
}

typedef struct _queue_object {
	int num;
	struct _queue_object *next;
} queue_object;

int queue_add(int num, queue_object *queue)
{
	if(queue == NULL) return -1;
	
	queue_object* new = malloc(sizeof(queue_object));
	if(new == NULL) return -1;

	new->num = num;
	new->next = queue->next;
	queue->next = new;
	return 0;
}

int sort_add(int n, queue_object* queue)
{
	queue_object* num_to_check = queue;
    while(num_to_check->next != NULL &&
          (num_to_check->next->num < n))
    {
        num_to_check = num_to_check->next;
    }
    return queue_add(n, num_to_check);
}

queue_object* queue_poll(queue_object *queue)
{
	// TODO
	if(queue->next == NULL || queue == NULL) return NULL;

	queue_object* first = queue->next;
	if(first->next == NULL) queue->next = NULL;
	else queue->next = first->next;
	return first;
}

queue_object *new_queue()
{
	queue_object* queue = malloc(sizeof(queue_object));
	if(queue == NULL) return NULL;
	queue->num = -1;
	queue->next = NULL;
	return queue;
}

void free_queue(queue_object *queue)
{
	if(queue == NULL) return;
	queue_object* item_to_free = queue;
	while (item_to_free->next != NULL)
	{
		queue = queue->next;
		free(item_to_free);
		item_to_free = queue;
	}
	free(item_to_free);
}

void string_from_item(queue_object* queue_item, file_system* fs, char* buffer, size_t buffer_size)
{
	if (queue_item == NULL) return;

	int inode_num = queue_item->num;
	inode* inode_ptr = inode_ptr_at_num(fs, inode_num);

	char line[NAME_MAX_LENGTH + 6];

	if(inode_ptr->n_type == directory)
	{
		snprintf(line, sizeof(line), "DIR %s\n", inode_ptr->name);
	}
	else if(inode_ptr->n_type == reg_file)
	{
		snprintf(line, sizeof(line), "FIL %s\n", inode_ptr->name);
	}

	strncat(buffer, line, buffer_size - strlen(buffer) - 1);
}

void string_from_queue(queue_object* queue, file_system* fs, char* buffer, size_t buffer_size)
{
	if(queue == NULL || queue->next == NULL) return;
	queue_object* item = queue_poll(queue);

	while (item != NULL)
	{
		string_from_item(item, fs, buffer, buffer_size);
		free(item);
		item = queue_poll(queue);
	}
}

char *
fs_list(file_system *fs, char *path)
{
	static char result[4096];
	result[0] = '\0';

	queue_object* queue = new_queue();
	if (queue == NULL) return NULL;
	
	int inode_num = traverse_path(fs, path, strlen(path));
	if(inode_num == -1) 
	{
		free_queue(queue);
		return NULL;
	}

	inode* inode_ptr = inode_ptr_at_num(fs, inode_num);

	for(int i = 0; i < DIRECT_BLOCKS_COUNT; i++)
	{
		if(inode_ptr->direct_blocks[i] == -1) continue;
		if(sort_add(inode_ptr->direct_blocks[i], queue) == -1)
		{
			free_queue(queue);
			return NULL;
		}
	}

	string_from_queue(queue, fs, result, sizeof(result));
	free_queue(queue);
	return result;
}

int
fs_writef(file_system *fs, char *filename, char *text)
{
	// Get inode number
	int inode_num = traverse_path(fs, filename, strlen(filename));
	if(inode_num == -1) return -1;

	// Get inode pointer and check if inode is a file
	inode* inode_ptr = inode_ptr_at_num(fs, inode_num);
	if(inode_ptr->n_type != reg_file) return -1;

	// Variables for writing
	int text_length = strlen(text);
	int current_size = inode_ptr->size;						// How big the file currently is
	int last_block_index = current_size / BLOCK_SIZE;		// Index of the last written in data_block
	int offset_in_last_block = current_size % BLOCK_SIZE;	// Offset in last written in block

	int written = 0; // How much data has been written

	// If there has already been data in inode
	if(current_size > 0 && offset_in_last_block != 0)
	{
		// Get last written data block and its index in fs
		int block_num = last_block_index;
		data_block* block = data_block_at_num(fs, block_num);

		// Check for space left in data block and how much data fits there
		int space_left = BLOCK_SIZE - offset_in_last_block;
		int copy_size = (text_length < space_left) ? text_length : space_left;
		
		// Copy data to data block
		memcpy(block->block + offset_in_last_block, text, copy_size);
		
		// Mark data block as used in fs
		fs->free_list[last_block_index] = 0;

		// Get data block index in inode's direct block
		int direct_block_num = find_direct_block_with_val(fs, inode_ptr, last_block_index);
		if(direct_block_num == -1)
		{
			direct_block_num = find_direct_block_with_val(fs, inode_ptr, -1);
		} 

		// Assign data block to inode
		inode_ptr->direct_blocks[direct_block_num] = last_block_index;

		// Save how much data has been written
		block->size += copy_size;
		written += copy_size;
	}

	// Write the rest data in new data block if theres any
	while (written < text_length)
	{
		// Get free data block with smallest index
		int free_block_num = find_free_block(fs);
		if(free_block_num == -1) return -1;
		data_block* new_block = data_block_at_num(fs, free_block_num);

		// Mark data block as used in fs
		fs->free_list[free_block_num] = 0;

		// Get data block index in inode's direct block
		int new_block_num = inode_ptr->size / BLOCK_SIZE;
		// If data too big return -1
		if(new_block_num > DIRECT_BLOCKS_COUNT) return -1;
		// Assign data block to inode
		inode_ptr->direct_blocks[new_block_num] = free_block_num;

		// Copy data to data block
		int copy_size = (text_length - written < BLOCK_SIZE) ? text_length - written : BLOCK_SIZE;
		memcpy(new_block->block, text + written, copy_size);

		// Save how much data has been written
		new_block->size = copy_size;
		written += copy_size;

		// Change inode size
		inode_ptr->size += copy_size;
	}

	return written;
}

uint8_t *
fs_readf(file_system *fs, char *filename, int *file_size)
{
	int inode_num = traverse_path(fs, filename, strlen(filename));
	if(inode_num == -1) return NULL;
	inode* inode_ptr = inode_ptr_at_num(fs, inode_num);
	if(inode_ptr->n_type != reg_file) return NULL;
	
	*file_size = 0;
	int size = inode_ptr->size;
	if(size > BLOCK_SIZE * DIRECT_BLOCKS_COUNT || size == 0) return NULL;
	*file_size = size;

	static uint8_t result[BLOCK_SIZE * DIRECT_BLOCKS_COUNT];

	int copied = 0;
	for(int i = 0; i < DIRECT_BLOCKS_COUNT; i++)
	{
		int data_block_index = inode_ptr->direct_blocks[i];
		if(data_block_index == -1) break;

		data_block* data_block = data_block_at_num(fs, data_block_index);
		int to_copy = (size - copied < BLOCK_SIZE) ? size - copied : BLOCK_SIZE;
		memcpy(result + copied, data_block->block, to_copy);
		copied += to_copy;
	}

	return result;
}


int
fs_rm(file_system *fs, char *path)
{
	int parent_inode_num = traverse_path_parent(fs, path, strlen(path));
	if(parent_inode_num == -1) return -1;
	inode* parent_inode_ptr = inode_ptr_at_num(fs, parent_inode_num);

	int inode_num = traverse_path(fs, path, strlen(path));
	if(inode_num == -1) return -1;
	inode* inode_ptr = inode_ptr_at_num(fs, inode_num);

	if(inode_ptr->n_type == reg_file)
	{
		// Free direct blocks
		for(int i = 0; i < DIRECT_BLOCKS_COUNT; i++)
		{
			// Get block index
			int block_index = inode_ptr->direct_blocks[i];
			if(block_index == -1) continue;

			// Get block and reset block's data
			data_block* block = data_block_at_num(fs, block_index);
			block->size = 0;
			memset(block->block, 0, BLOCK_SIZE);
			
			// Mark block as free in free_list and remove reference from inode
			fs->free_list[block_index] = 1;
			inode_ptr->direct_blocks[i] = -1;
		}
	}
	else if (inode_ptr->n_type == directory)
	{
		for(int i = 0; i < DIRECT_BLOCKS_COUNT; i++)
		{
			// Get child inode
			if(inode_ptr->direct_blocks[i] == -1) continue;
			inode* child = inode_ptr_at_num(fs, inode_ptr->direct_blocks[i]);

			// Build path for rm -r
			char child_path[256];
			snprintf(child_path, sizeof(child_path), "%s/%s", path, child->name);

			// Remove children recursively
			if (fs_rm(fs, child_path) == -1) return -1;
		}
	}
	
	// Reset inode's info
	inode_ptr->n_type = free_block;
	inode_ptr->size = 0;
	memset(inode_ptr->name, 0, NAME_MAX_LENGTH);
	inode_ptr->parent = -1;

	// Remove reference from parent
	int parent_direct_block_index = find_direct_block_with_val(fs, parent_inode_ptr, inode_num);
	if(parent_direct_block_index == -1) return -1;
	parent_inode_ptr->direct_blocks[parent_direct_block_index] = -1;

	return 0;
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
