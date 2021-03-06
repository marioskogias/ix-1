/* Freelist to manage LBA mappings 
 * TODO: a stack is probably the best way to manage this..
 *
 */

#include <stdlib.h>
#include <stdio.h> //For debug only..

#include <ix/blk.h>

/*
#define LBA_SIZE 512 //TODO: tune this depending on the device..?
#define MAX_LBA_NUM 100000 //TODO: THIS IS TEMPORARY...
//#define NUM_SZ_CLASS 16
#define META_SZ 32 //size of metadata, TODO: RECOMPUTE LATER..
#define DATA_SZ (LBA_SIZE - META_SZ)
*/

struct freelist_ent { //TODO: keep this internal to this file..?
	uint64_t start_lba;
	uint64_t lba_count;
	struct freelist_ent *next;	
};

static struct freelist_ent *freelist;
//static struct freelist_ent *freelist[NUM_SZ_CLASS];

void mark_used_blk(struct freelist_ent *ent, uint64_t num_blks){

	if (num_blks < ent->lba_count){
		ent->start_lba += num_blks;
		ent->lba_count -= num_blks;

	} else { //remove this entry..
		if (ent == freelist){
			freelist = freelist->next;
		} else {
			struct freelist_ent *tmp = freelist;
			while (tmp->next != ent){ //TODO: maybe use doubly linked list if want to avoid this..
				tmp = tmp->next;
			}
			tmp->next = ent->next; 
		}
		free(ent);
	}

}

/*
* search for contiguous block large enough..
* first fit alg. b/c not likely to do *too* much reclamation (although this may change)
* also last ent guaranteed to be "rest of space" entry
*/ 
uint64_t get_blk(uint64_t num_blks){
	
	struct freelist_ent *iter = freelist;
	//printf("DEBUG: entering loop\n");
	while (iter){
		if (iter->lba_count >= num_blks)
			break;
		iter = iter->next;
	} 
	//printf("DEBUG: exited loop\n");

	//TODO: currently assume will always have storage space..what if run out of blocks 

	uint64_t ret = iter->start_lba;
	mark_used_blk(iter, num_blks); //in case iter is modified

	return ret; //just return lba ?
}

/*
 * coalesce free block info w/ entry ent
 */
static void coalesce_blks(uint64_t lba, uint64_t count, struct freelist_ent *ent){
	ent->start_lba = lba;
	ent->lba_count += count;

}

void free_blk(uint64_t lba, uint64_t num_blks){

	assert(lba < MAX_LBA_NUM);

	struct freelist_ent *iter = freelist; 
	while (iter->next){ //find where to insert free block
		if (lba < iter->start_lba)
			break;
		iter = iter->next;
	}

	if (lba + num_blks == iter->start_lba){ //coalesce info instead of making a new block
		coalesce_blks(lba, num_blks, iter);
	} else {
		struct freelist_ent *newent = malloc(sizeof(struct freelist_ent));
		newent->start_lba = lba;
		newent->lba_count = num_blks;
		newent->next = iter;
		if (iter == freelist)
			freelist = newent;
	}

}

/*
 * Initialize freelist to entire space
 * TODO: will this be per-core or global..?
 */ 
void freelist_init(){
	freelist = malloc(sizeof(struct freelist_ent));

	freelist->start_lba = 0;
	freelist->lba_count = MAX_LBA_NUM;
	freelist->next = NULL;

}

//FOR DEBUGGING ONLY
void print_freelist(){

	printf("DEBUG: List of free blocks:\n");
	struct freelist_ent *tmp = freelist;
	while (tmp){
		printf("Block # %lu for %lu blocks\n", tmp->start_lba, tmp->lba_count);
		tmp = tmp->next;
	}
}



