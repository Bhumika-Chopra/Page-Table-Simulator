#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include<string.h>

typedef struct pt_type {
	char* vpn;
	int dirty;
	int present;
} pt;

const char* file = NULL;
int size = 0;
static int verbose = 0;
static int mem_acesses = 0;
static char future_add[10000000][8];
static char future_act[10000000][2];

pt table[1000] = {};
int miss = 0;
int write = 0;
int drop = 0;
int frame = 0; //for replacement
int cur_size = 0;

void split_line(char* line, char* address, char* read_write)
{
	int len = strlen(line);
	if (!(line[0] == '0' && line[1] == 'x')) return;
	int idx = 0;
	while(line[idx] != ' ') 
	{
		idx++;
		if(idx == len) return;
	}

	strncpy(address, line, idx);
	address[idx] = 0;
	while(line[idx] == ' ')
	{
		idx++;
		if(idx == len) return;
	}
	*read_write = line[idx];
	return; 
}

void future_access() {
	// printf("future_access set\n");
	FILE* fptr;
	fptr = fopen(file, "r");
	char line[50];
	char* str = malloc(11);
	char t;
	int i;
	int j = 0;
	while(fgets(line, 50, fptr) != NULL) {
		split_line(line, str, &t);
		long add = strtol(str, NULL, 16);
		long mask = strtol("0xFFFFF000", NULL, 16);
		long vpn = (add & mask) >> 12;
		sprintf(str, "0x%05lx", vpn);
		strcpy(future_add[j],str);
		*future_act[j] = t;
		j++;
	}
	mem_acesses = j;
	fclose(fptr);
}


void opt() {
	int i;
	int k = 0;
	int j = 0;
	for(j=0; j<mem_acesses; j++) {
		int flag = 0;
		for(i=0; i<cur_size; i++) {
			if(strcmp(table[i].vpn, future_add[j]) == 0) {
				if(strcmp(future_act[j], "W") == 0) {
					table[i].dirty = 1;
				}
				flag = 1;
				break;
			} 
		}
		if(!flag) {
			miss++;
			if(cur_size < size) {
				table[cur_size].vpn = malloc(strlen(future_add[j]) + 1);
				strcpy(table[cur_size].vpn, future_add[j]);
				if(strcmp(future_act[j], "W") == 0) {
					table[cur_size].dirty = 1;
				}
				else { //default
					table[cur_size].dirty = 0;
				}
				cur_size++;
			}
			else { 	//replacement
				int m = 0;
				int ind = 0;
				int found = 0;
				for(m=0; m<size; m++) {
					found = 0;
					for(i=k; i<mem_acesses; i++) {
						if (strcmp(table[m].vpn, future_add[i]) == 0) {
							found = 1;
							if(ind < i) {
								ind = i;
								frame = m;
							}
							break;
						}
					}	
					if(!found) {
						frame = m;
						break;
					}
				}
				if(table[frame].dirty == 1) { //write back to disk
					if(verbose == 1) {
						printf("Page %s was read from disk, page %s was written to the disk.\n" , future_add[j], table[frame].vpn);
					}
					write++;
				}
				else {
					drop++;
					if(verbose == 1) {
						printf("Page %s was read from disk, page %s was dropped (it was not dirty).\n" , future_add[j], table[frame].vpn);
					}
				}
				strcpy(table[frame].vpn, future_add[j]);
				if(strcmp(future_act[j], "W") == 0) {
					table[frame].dirty = 1;
				}
				else {
					table[frame].dirty = 0;
				}
			}
		}
		k++;
	}
	printf("Number of memory accesses: %d\n", mem_acesses);
	printf("Number of misses: %d\n", miss);
	printf("Number of writes: %d\n", write);
	printf("Number of drops: %d\n", drop);
	return;
}

void fifo() { 		//verify once
	int i;
	int k = 0;
	for(k=0; k<mem_acesses; k++) {
		int flag = 0;
		for(i=0; i<cur_size; i++) {
			if(strcmp(table[i].vpn, future_add[k]) == 0) {
				if(strcmp(future_act[k], "W") == 0) {
					table[i].dirty = 1;
				}
				flag = 1;
				break;
			} 
		}
		if(!flag) {
			miss++;
			if(cur_size < size) {
				table[cur_size].vpn = malloc(strlen(future_add[k]) + 1);
				strcpy(table[cur_size].vpn, future_add[k]);
				if(strcmp(future_act[k], "W") == 0) {
					table[cur_size].dirty = 1;
				}
				else { //default
					table[cur_size].dirty = 0;
				}
				cur_size++;
			}
			else { 	//replacement
				if(table[frame].dirty == 1) { //write back to disk
					if(verbose == 1) {
						printf("Page %s was read from disk, page %s was written to the disk.\n" , future_add[k], table[frame].vpn);
					}
					write++;
				}
				else {
					drop++;
					if(verbose == 1) {
						printf("Page %s was read from disk, page %s was dropped (it was not dirty).\n" , future_add[k], table[frame].vpn);
					}
				}
				strcpy(table[frame].vpn, future_add[k]);
				if(strcmp(future_act[k], "W") == 0) {
					table[frame].dirty = 1;
				}
				else {
					table[frame].dirty = 0;
				}
				frame = (frame+1)%size;
			}
		}
	}
	printf("Number of memory accesses: %d\n", mem_acesses);
	printf("Number of misses: %d\n", miss);
	printf("Number of writes: %d\n", write);
	printf("Number of drops: %d\n", drop);
	return;
}

void Random() {
	srand(5635);
	int i;
	int k = 0;
	for(k=0; k<mem_acesses; k++) {
		int flag = 0;
		for(i=0; i<cur_size; i++) {
			if(strcmp(table[i].vpn, future_add[k]) == 0) {
				if(strcmp(future_act[k], "W") == 0) {
					table[i].dirty = 1;
				}
				flag = 1;
				break;
			} 
		}
		if(!flag) {
			miss++;
			if(cur_size < size) {
				table[cur_size].vpn = malloc(strlen(future_add[k]) + 1);
				strcpy(table[cur_size].vpn, future_add[k]);
				if(strcmp(future_act[k], "W") == 0) {
					table[cur_size].dirty = 1;
				}
				else { //default
					table[cur_size].dirty = 0;
				}
				cur_size++;
			}
			else { 	//replacement
				frame = rand()%size;
				if(table[frame].dirty == 1) { //write back to disk
					if(verbose == 1) {
						printf("Page %s was read from disk, page %s was written to the disk.\n" , future_add[k], table[frame].vpn);
					}
					write++;
				}
				else {
					drop++;
					if(verbose == 1) {
						printf("Page %s was read from disk, page %s was dropped (it was not dirty).\n" , future_add[k], table[frame].vpn);
					}
				}
				strcpy(table[frame].vpn, future_add[k]);
				if(strcmp(future_act[k], "W") == 0) {
					table[frame].dirty = 1;
				}
				else {
					table[frame].dirty = 0;
				}
			}
		}
	}

	printf("Number of memory accesses: %d\n", mem_acesses);
	printf("Number of misses: %d\n", miss);
	printf("Number of writes: %d\n", write);
	printf("Number of drops: %d\n", drop);
	return;
}

void lru() {
	int i;
	int k = 0;
	int arr[1000];
	int j = 0;
	for(j=0; j<mem_acesses; j++) {
		int flag = 0;
		for(i=0; i<cur_size; i++) {
			if(strcmp(table[i].vpn, future_add[j]) == 0) {
				if(strcmp(future_act[j], "W") == 0) {
					table[i].dirty = 1;
				}
				arr[i] = k;
				flag = 1;
				break;
			} 
		}
		if(!flag) {
			miss++;
			if(cur_size < size) {
				table[cur_size].vpn = malloc(strlen(future_add[j]) + 1);
				strcpy(table[cur_size].vpn, future_add[j]);
				if(strcmp(future_act[j], "W") == 0) {
					table[cur_size].dirty = 1;
				}
				else { //default
					table[cur_size].dirty = 0;
				}
				arr[cur_size] = k;
				cur_size++;
			}
			else { 	//replacement
				int min = arr[0];
				frame = 0;
				for(i=1; i<size; i++) {
					if(min > arr[i]) {
						min = arr[i];
						frame = i;
					}
				}
				if(table[frame].dirty == 1) { //write back to disk
					if(verbose == 1) {
						printf("Page %s was read from disk, page %s was written to the disk.\n" , future_add[j], table[frame].vpn);
					}
					write++;
				}
				else {
					drop++;
					if(verbose == 1) {
						printf("Page %s was read from disk, page %s was dropped (it was not dirty).\n" , future_add[j], table[frame].vpn);
					}
				}
				strcpy(table[frame].vpn, future_add[j]);
				if(strcmp(future_act[j], "W") == 0) {
					table[frame].dirty = 1;
				}
				else {
					table[frame].dirty = 0;
				}
				arr[frame] = k;
			}
		}
		k++;
	}
	printf("Number of memory accesses: %d\n", mem_acesses);
	printf("Number of misses: %d\n", miss);
	printf("Number of writes: %d\n", write);
	printf("Number of drops: %d\n", drop);
	return;

}

void clock() {
	int i;
	int counter = 0;
	int k = 0;
	for(k=0; k<mem_acesses; k++) {
		int flag = 0;
		for(i=0; i<cur_size; i++) {
			if(strcmp(table[i].vpn, future_add[k]) == 0) {
				if(strcmp(future_act[k], "W") == 0)
					table[i].dirty = 1;
				table[i].present = 1;
				flag = 1;
				break;
			} 
		}
		if(!flag) {
			//not present in table
			miss++;
			if(cur_size < size) {
				table[cur_size].vpn = malloc(strlen(future_add[k]) + 1);
				strcpy(table[cur_size].vpn, future_add[k]);
				if(strcmp(future_act[k], "W") == 0)
					table[cur_size].dirty = 1;
				else //default
					table[cur_size].dirty = 0;
				table[cur_size].present = 1;
				cur_size++;
			}
			else { 	//replacement
				while(table[counter].present != 0) {
					table[counter].present = 0;
					counter = (counter+1)%size;
				}
				frame = counter;
				counter = (counter+1)%size;
				if(table[frame].dirty == 1) { //write back to disk
					if(verbose == 1)
						printf("Page %s was read from disk, page %s was written to the disk.\n" , future_add[k], table[frame].vpn);
					write++;
				}
				else {
					drop++;
					if(verbose == 1)
						printf("Page %s was read from disk, page %s was dropped (it was not dirty).\n" , future_add[k], table[frame].vpn);
				}
				strcpy(table[frame].vpn, future_add[k]);
				if(strcmp(future_act[k], "W") == 0)
					table[frame].dirty = 1;
				else
					table[frame].dirty = 0;

				table[frame].present = 1;
			}
		}
	}
	printf("Number of memory accesses: %d\n", mem_acesses);
	printf("Number of misses: %d\n", miss);
	printf("Number of writes: %d\n", write);
	printf("Number of drops: %d\n", drop);
	return;

}

int main(int argc, char const *argv[])
{
	file = argv[1];
	size = atoi(argv[2]);
	if(argc == 5) {
		//verbose passed
		if(strcmp(argv[4], "-verbose") == 0)
		verbose = 1;
	}

	future_access();
	if(strcmp(argv[3], "OPT") == 0)
		opt();
	else if(strcmp(argv[3], "FIFO") == 0)
		fifo();
	else if(strcmp(argv[3], "CLOCK") == 0)
		clock();
	else if(strcmp(argv[3], "RANDOM") == 0)
		Random();
	else if(strcmp(argv[3], "LRU")== 0)
		lru();
	else
		printf("error\n");

	// clean table
	int i;
	for(i=0; i<cur_size; i++)
		free(table[i].vpn);
	return 0;
}