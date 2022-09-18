#include <iostream>
#include "memlab.h"
#include <climits>
#include <cstring>
#include <time.h>
using namespace std;

char* user_mem; // User memory
char* info_mem; // Additional info memory
int SIZE;
int TABLE_SIZE;
int Q_SZ;
bool gc_on;

// Array of free segs
node* q;
int q_end;

// PT
table* t;
table* table_st;
table* table_end;
int table_cnt;
double gc_timer;

// Stack
stk_node* stk;
int stk_end;
int STK_SZ;
int stk_cnt;
pthread_mutex_t stk_lock; // For compaction of stack
pthread_mutex_t stk_cnt_lock; // For actual stack cnt
pthread_mutex_t compaction_lock; // lock for compaction algo
pthread_mutex_t q_end_lock; // lock for compaction algo

pthread_t tid;

int free_space;

my_type::my_type(short type, int sz) {
	id = allocate_space(type, sz);
	stk_off = insert_stk(id);
}

void my_type::freeElem() {
	::freeElem(id);
	stk[stk_off].addr = -2; 
	id = -1;
}

my_int::my_int(): my_type(TYPE_INT) {}
my_int::my_int(int ar_sz, bool is_arr): my_type(TYPE_INT, is_arr? ar_sz: 1) {
	if(!is_arr)
		assignval(ar_sz);
}

int my_int::getval(int index) {
	valid_ref(id);
	table* tmp = t + id;
	char* dest = user_mem + tmp -> addr;
	bool is_arr = (index >= 0);
	if(index == -1) index = 0;
	int val = 0;

	if(4 * index >= tmp -> numbytes) // Invalid access
	{
		cout << "## Error: Invalid memory access. Aborting...\n";
		exit(-1);
	}
	copy_data((char *)&val, dest + 4 * index, 4); // Read the data
	return val;
}

void my_int::assignval(int val, int index) {
	assignVar(id, val, index);
	if(index == -1)
		cout << "$$ assignVar: Variable assignment for varID " << id << " successful...\n";
	else
		cout << "$$ assignVar: Variable assignment for varID " << id << "[" << index << "] successful...\n";
}

void my_int::operator=(int val) {
	assignVar(id, val, -1);
	cout << "$$ assignVar: Variable assignment for varID " << id << " successful...\n";
}

my_medint::my_medint(): my_type(TYPE_MED_INT) {}
my_medint::my_medint(int ar_sz, bool is_arr): my_type(TYPE_MED_INT, is_arr? ar_sz: 1) {
	if(!is_arr)
		assignval(ar_sz);
}

int my_medint::getval(int index) {
	valid_ref(id);
	table* tmp = t + id;
	char* dest = user_mem + tmp -> addr;
	bool is_arr = (index >= 0);
	if(index == -1) index = 0;
	int val = 0;

	if(3 * index >= tmp -> numbytes) // Invalid access
	{
		cout << "## Error: Invalid memory access. Aborting...\n";
		exit(-1);
	}
	copy_data((char *)&val, dest + 3 * index, 3); // Read the data
	if((val >> 23) & 1) { // Negative number => Perform sign extension
		for(int j = 24; j < 32; ++j)
			val = val ^ (1LL << j);
	}
	return val;
}

void my_medint::assignval(int val, int index) {
	assignVar(id, val, index);
	if(index == -1)
		cout << "$$ assignVar: Variable assignment for varID " << id << " successful...\n";
	else
		cout << "$$ assignVar: Variable assignment for varID " << id << "[" << index << "] successful...\n";
}

void my_medint::operator=(int val) {
	assignVar(id, val, -1);
	cout << "$$ assignVar: Variable assignment for varID " << id << " successful...\n";
}

my_char::my_char(): my_type(TYPE_CHAR) {}
my_char::my_char(int ar_sz, bool is_arr): my_type(TYPE_CHAR, is_arr? ar_sz: 1) {
	if(!is_arr)
		assignval(ar_sz);
}

char my_char::getval(int index) {
	valid_ref(id);
	table* tmp = t + id;
	char* dest = user_mem + tmp -> addr;
	bool is_arr = (index >= 0);
	if(index == -1) index = 0;
	int val = 0;

	if(index >= tmp -> numbytes) // Invalid access
	{
		cout << "## Error: Invalid memory access. Aborting...\n";
		exit(-1);
	}

	return *(dest + index);
}

void my_char::assignval(int val, int index) {
	assignVar(id, val, index);
	if(index == -1)
		cout << "$$ assignVar: Variable assignment for varID " << id << " successful...\n";
	else
		cout << "$$ assignVar: Variable assignment for varID " << id << "[" << index << "] successful...\n";
}

void my_char::operator=(char val) {
	assignVar(id, val, -1);
	cout << "$$ assignVar: Variable assignment for varID " << id << " successful...\n";
}

my_bool::my_bool(): my_type(TYPE_BOOLEAN) {}
my_bool::my_bool(int ar_sz, bool is_arr): my_type(TYPE_BOOLEAN, is_arr? ar_sz: 1) {
	if(!is_arr)
		assignval(ar_sz);
}

bool my_bool::getval(int index) {
	valid_ref(id);
	table* tmp = t + id;
	char* dest = user_mem + tmp -> addr;
	bool is_arr = (index >= 0);
	if(index == -1) index = 0;
	bool val = 0;

	// First get the corresponding byte
	int byte_no = index / 8;
	int bit_no = index % 8;
	if(byte_no < tmp -> numbytes or (byte_no == tmp -> numbytes and bit_no < tmp -> numbits)) {
		char c = *(dest + byte_no);
		val = ((c >> bit_no) & 1);
	}
	else {
		cout << "## Error: Invalid memory access. Aborting...\n";
		exit(-1);
	}

	return val;
}

void my_bool::assignval(int val, int index) {
	assignVar(id, val, index);
	if(index == -1)
		cout << "$$ assignVar: Variable assignment for varID " << id << " successful...\n";
	else
		cout << "$$ assignVar: Variable assignment for varID " << id << "[" << index << "] successful...\n";
}

void my_bool::operator=(bool val) {
	assignVar(id, val, -1);
	cout << "$$ assignVar: Variable assignment for varID " << id << " successful...\n";
}

stk_node::stk_node(int addr_, bool alive_): addr(addr_), alive(alive_) {
  	if (pthread_mutex_init(&lock, NULL) != 0) {
        printf("\n mutex init has failed\n");
        exit(EXIT_FAILURE);
    }
}

int createMem(size_t size, const string& type) {
	size_t tmp = size;
	int extra = 0;
	string type_str;
	if(type == "KB")
		size *= 1024, type_str = "KB";
	else if(type == "MB")
		size = size * 1024 * 1024, type_str = "MB";
	else if(type == "GB")
		size = size * 1024 * 1024 * 1024, type_str = "GB";
	else if(type != "B") {
		cout << "## Error: Memory size is not defined! Aborting...\n";
		exit(-1);
	}	
	gc_timer = 0;
	if (pthread_mutex_init(&stk_lock, NULL) != 0) {
        printf("\n mutex init for stk_lock has failed\n");
        exit(EXIT_FAILURE);
    }
    
    if (pthread_mutex_init(&stk_cnt_lock, NULL) != 0) {
        printf("\n mutex init for stk_cnt_lock has failed\n");
        exit(EXIT_FAILURE);
    }

    if (pthread_mutex_init(&compaction_lock, NULL) != 0) {
        printf("\n mutex init for stk_cnt_lock has failed\n");
        exit(EXIT_FAILURE);
    }

    if (pthread_mutex_init(&q_end_lock, NULL) != 0) {
        printf("\n mutex init for stk_cnt_lock has failed\n");
        exit(EXIT_FAILURE);
    }

    // Use 50% of the user requested space as additional space
    // If user space is very high start with 3 * 2^12 bytes of extra space
	TABLE_SIZE = min((size / 6 + sizeof(table) - 1) / sizeof(table) + 1, (size_t)MAX_T_SZ);
	STK_SZ = min((size / 6 + sizeof(stk_node) - 1) / sizeof(stk_node) + 3, (size_t)MAX_ST_SZ);
	Q_SZ = min((size / 6 + sizeof(node) - 1) / sizeof(node) + 2, (size_t)MAX_Q_SZ);

	extra = TABLE_SIZE * sizeof(table) + STK_SZ * sizeof(stk_node) + Q_SZ * sizeof(node);
	// Allocate sufficient bytes (multiple of word size)
	SIZE = ((long long)size + WORD_SZ - 1) / WORD_SZ * WORD_SZ;
	user_mem = (char *) malloc(SIZE);
	info_mem = (char *) malloc(Q_SZ * sizeof(node) + TABLE_SIZE * sizeof(table));
	stk = (stk_node*) malloc(STK_SZ * sizeof(stk_node));
	stk_end = -1;
	stk_cnt = 0;

	if(user_mem == 0 or info_mem == 0)
		return -1;

	cout << "$$ createMem: " << tmp << " " << type << " memory allocated successfully\n";
	cout << "$$ createMem: " << extra << " " << "bytes" << " memory allocated successfully\n";
	// Init the queue of free space
	q = (node *) info_mem;
	q_end = 0;
	q[0] = node(0, SIZE - 1);

	// Create the page table
	t = (table *)((node *)info_mem + Q_SZ);
	table_st = t;
	table_end = t;
	table_cnt = 0;
	table* ptr = t;
	for(int i = 0; i < TABLE_SIZE; ++i) {
		ptr[i] = table();
		if(i != 0)
			ptr[i].prev = (ptr + i - 1);
		if(i != TABLE_SIZE - 1)
			ptr[i].next = (ptr + i + 1);
	}

	free_space = SIZE;
	int err = pthread_create(&tid, NULL, &GC, NULL);
	if(err == -1) {
		cout << "## Error: GC thread initialization failed. Aborting...\n";
		exit(-1);
	}
	return 0;
}

int alloc_space_helper(const short type, int req_words, int req_bytes, int req_bits) {
	// Traverse the free mem segments to find a sufficient free slot
	int q_st = 0;
	while(q_st <= q_end) {
		int sz = q[q_st].r - q[q_st].l + 1;
		if(sz >= req_words * WORD_SZ) {
			// Found space
			int l = q[q_st].l, r = q[q_st].r;

			int id = table_end - t;

			// Update the page table
			table_cnt++;
			table_end -> addr = l; // offset
			table_end -> numbytes = req_bytes;
			table_end -> numbits = req_bits;
			table_end -> type = (char)('0' + type);
			if(table_end -> next) {
				table_end = table_end -> next;
			}

			cout << "$$ allocate_space: Memory allocated to varID " << id << " [" << l << ',' << (l + req_words * WORD_SZ - 1);
			cout << "] (bytes) (" << req_words << " words)\n";

			// Update the free page queue
			l += req_words * WORD_SZ;
			free_space -= req_words * WORD_SZ;
			if(l <= r)
				q[q_st].l = l;
			else {
				// Delete this slot (no longer free)
				if(q_end != q_st) {
					// Copy the last free slot here
					q[q_st] = q[q_end];
				}
				q_end--;
			}
			return id; // Return the offset
		}
		q_st++;
	}
	return -1;
}

int allocate_space(const short type, int cnt) {
	int req_bytes;
	short req_bits = 0;
	int req_words;

	// Compute the req space based on type and cnt
	switch(type) {
		case TYPE_INT:
			req_bytes = 4 * cnt;
			break;
		case TYPE_MED_INT:
			req_bytes = 3 * cnt;
			break;
		case TYPE_BOOLEAN:
			req_bytes = cnt / 8;
			req_bits = cnt % 8;
			break;
		case TYPE_CHAR:
			req_bytes = cnt;
			break;
		default:
			cout << "## Error: Invalid type!! Aborting...\n";
			exit(-1);
	}

	req_words = (req_bytes + (req_bits > 0) + WORD_SZ - 1) / WORD_SZ;

	if(req_words * WORD_SZ > free_space) {
		cout << "## Error: Not enough memory!! Aborting...\n";
		exit(-1);
	}

	pthread_mutex_lock(&compaction_lock);
	if(table_cnt == TABLE_SIZE) {
		// Start and end offset of the initial PT
		int start_off = table_st - t;
		int end_off = table_end - t;
		table* old_t = t;

		// Realloc to increase space
		cout << "$$ allocate_space: Page Table size exceeded. Calling Realloc...\n";
		TABLE_SIZE *= 2; // Increase table size 2 fold
		info_mem = (char *)realloc(info_mem, Q_SZ * sizeof(node) + TABLE_SIZE * sizeof(table));
		if(info_mem == NULL) {
			cout << "## Error: Realloc failed! Aborting...\n";
			exit(-1);
		}
		else {
			// info_mem = tmp;
			// Reinit the ptrs
			q = (node *) info_mem;

			// Create the page table
			t = (table *)((node *)info_mem + Q_SZ);
			table_st = t + start_off;
			table_end = t + end_off;

			table* ptr = table_st;
			// next, prev may get invalidated. Update them
			while(ptr) {
				if(ptr != table_end) {
					int off = ptr -> next - old_t;
					ptr -> next = t + off;
				}
			
				if(ptr != table_st) {
					int off = ptr -> prev - old_t;
					ptr -> prev = t + off;
				}
				ptr = ptr -> next;
			}
			
			// Connect the new slots
			table* new_t = t + TABLE_SIZE / 2;
			
			for(int i = 0; i < TABLE_SIZE / 2; ++i) {
				new_t[i] = table();
				if(i != 0)
					new_t[i].prev = (new_t + i - 1);
				if(i != TABLE_SIZE / 2 - 1)
					new_t[i].next = (new_t + i + 1);
			}

			// connect with old table
			if(table_end -> next) {
				cout << "## Error: Table end is not NULL! Aborting...\n"; exit(-1);
			}

			table_end -> next = new_t;
			new_t -> prev = table_end;
			table_end = new_t;
			cout << "$$ allocate_space: Suceessfully extended PT\n";
		}
	}

	int id = alloc_space_helper(type, req_words, req_bytes, req_bits);
	pthread_mutex_unlock(&compaction_lock);
	if(id == -1) {
		// External fragmentation. Perform compaction
		cout << "$$ allocate_space: External fragmentation. Performing compaction...\n";
		compaction();
		pthread_mutex_lock(&compaction_lock);
		id = alloc_space_helper(type, req_words, req_bytes, req_bits);
		pthread_mutex_unlock(&compaction_lock);
	}
	cerr << "$$ allocate_space: " << SIZE - free_space << '\n';
	return id;
}

void freeElem(int id) {
	if(id < 0) return;
	valid_ref(id);
	table* tmp = t + id;
	pthread_mutex_lock(&q_end_lock);
	++q_end;
	if(q_end == Q_SZ) {
		// Too much fragmentation
		cout << "$$ freeElem: Too much fragmentation. Initiating compaction...\n";
		compaction();
		++q_end;
	}
	int req_words = (tmp -> numbytes + (tmp -> numbits > 0) + WORD_SZ - 1) / WORD_SZ;
	q[q_end].l = tmp -> addr;
	q[q_end].r = tmp -> addr + req_words * WORD_SZ - 1;
	free_space += req_words * WORD_SZ;
	cout << "$$ freeElem: Memory freed for varID " << id << " [" << tmp -> addr << "," << tmp -> addr + req_words * WORD_SZ - 1 << "] ";
	cout << "(bytes)\n";

	tmp -> addr = -1;

	if(tmp == table_st) { // Head of the LL
		// Change head
		table_st = table_st -> next;
		table_st -> prev = NULL;

		// Change current node
		tmp -> next = table_end -> next;
		tmp -> prev = table_end;

		if(table_end -> next)
			table_end -> next -> prev = tmp;

		// Extend table_end
		table_end -> next = tmp;
		if(table_cnt == TABLE_SIZE) // If table full
			table_end = tmp;
	}
	else if(tmp == table_end) {
		// Tail of the LL
		// This case means table is full. Just free the end => Do nothing
	}
	else {
		// Intermediate node
		tmp -> prev -> next = tmp -> next;
		tmp -> next -> prev = tmp -> prev;

		tmp -> next = table_end -> next;
		tmp -> prev = table_end;
		if(table_end -> next)
			table_end -> next -> prev = tmp;

		// Extend table_end
		table_end -> next = tmp;
		// If table full
		if(table_cnt == TABLE_SIZE) 
			table_end = tmp;
	}
	table_cnt--;
	pthread_mutex_unlock(&q_end_lock);
	cerr << "$$ freeElem: " << SIZE - free_space << '\n';
}

void assignVar(const int id, const int val, int index) {
	pthread_mutex_lock(&compaction_lock);
	valid_ref(id);
	table* tmp = t + id;
	short type = tmp -> type - '0';
	char* dest = user_mem + tmp -> addr;
	bool is_arr = (index >= 0);
	if(index == -1) index = 0;

	if(type == TYPE_INT) {
		if(4 * index >= tmp -> numbytes) // Invalid access
		{
			cout << "## Error: Invalid memory access. Aborting...\n";
			exit(-1);
		}
		copy_data(dest + 4 * index, (char *)&val, 4);
		// print_helper2(tmp, id, val, 0, index, is_arr);
	}
	else if(type == TYPE_MED_INT) {
		if(val > ((1 << 23) - 1) or val < -(1 << 23)) {
			cout << "## Error: Type mismatch! Aborting...\n";
			exit(-1);
		}
		if(3 * index >= tmp -> numbytes) // Invalid access
		{
			cout << "## Error: Invalid memory access. Aborting...\n";
			exit(-1);
		}
		copy_data(dest + 3 * index, (char *)&val, 3);
		// print_helper2(tmp, id, val, 0, index, is_arr);
	}
	else if(type == TYPE_BOOLEAN) {
		if(!(val == 0 or val == 1)) {
			cout << "## Error: Type mismatch! Aborting...\n"; exit(-1);
		}
		// First get the corresponding byte
		int byte_no = index / 8;
		int bit_no = index % 8;
		if(byte_no < tmp -> numbytes or (byte_no == tmp -> numbytes and bit_no < tmp -> numbits)) {
			unsigned char c = *(dest + byte_no);
			unsigned char c_msk = 255;
			if(val == 0) {
				c_msk = c_msk ^ (1 << bit_no);
				*((unsigned char *)(dest + byte_no)) = (c & c_msk);
			}
			else {
				c_msk = (1 << bit_no);
				*((unsigned char *)(dest + byte_no)) = (c | c_msk);
			}
			// print_helper2(tmp, id, val, 0, index, is_arr);
		}
		else {
			cout << "## Error: Invalid memory access. Aborting...\n";
			exit(-1);
		}

	}
	else {
		if(val < -128 or val > 127) {
			cout << "## Error: Type mismatch! Aborting...\n"; exit(-1);
		}
		if(index >= tmp -> numbytes) // Invalid access
		{
			cout << "## Error: Invalid memory access. Aborting...\n";
			exit(-1);
		}
		copy_data(dest + index, (char *)&val, 1); // Type checking is already done
		// print_helper2(tmp, id, val, 1, index, is_arr);
	}
	pthread_mutex_unlock(&compaction_lock);
}

table* merge_sort(table* start, table* end, int cnt) {
	end -> next = NULL;
	start -> prev = NULL;
	if(cnt > 1) {
		int m = cnt / 2;
		int c = 0;
		table* pt = start;
		while(c < m) {
			pt = pt -> next; c++;
		}
		pt -> prev -> next = NULL;
		table* s1 = merge_sort(start, pt -> prev, m);
		pt -> prev = NULL;
		table* s2 = merge_sort(pt, end, cnt - m);

		table* st = NULL; // Start of new list
		table* ptr = NULL;
		// Merge the two lists
		while(s1 != NULL or s2 != NULL) {
			if(s1 == NULL) {
				while(s2) {
					ptr -> next = s2;
					s2 -> prev = ptr;
					ptr = ptr -> next;
					s2 = s2 -> next;
				}
			}
			else if(s2 == NULL) {
				while(s1) {
					ptr -> next = s1;
					s1 -> prev = ptr;
					ptr = ptr -> next;
					s1 = s1 -> next;
				}
			}
			else {
				if(s1 -> addr < s2 -> addr) {
					if(ptr) {
						ptr -> next = s1;
						s1 -> prev = ptr;
						ptr = ptr -> next;
					}
					else {
						ptr = s1;
						st = s1;
					}
					s1 = s1 -> next;
				}
				else {
					if(ptr) {
						ptr -> next = s2;
						s2 -> prev = ptr;
						ptr = ptr -> next;
					}
					else {
						ptr = s2;
						st = s2;
					}
					s2 = s2 -> next;
				}
			}
		}
		ptr -> next = NULL;
		return st;
	}
	return start;
}

void print_table() {
	cout << "\n============== PT ===============\n";
	table* ptr = table_st;
	while(ptr != table_end) {
		int req_words = (ptr -> numbytes + (ptr -> numbits > 0) + WORD_SZ - 1) / WORD_SZ;
		cout << "[" << ptr -> addr << "," << ptr -> addr + req_words * WORD_SZ - 1 << "]" << '\n';
		ptr = ptr -> next;
	}
	if(table_cnt == TABLE_SIZE) {
		int req_words = (ptr -> numbytes + (ptr -> numbits > 0) + WORD_SZ - 1) / WORD_SZ;
		cout << "[" << ptr -> addr << "," << ptr -> addr + req_words * WORD_SZ - 1 << "]" << '\n';
		ptr = ptr -> next;
	}
	cout << "============== PT ===============\n\n";
	// table* end = table_end;
	// if(table_cnt != TABLE_SIZE)
	// 	end = end -> prev;
	// int c = 0;
	// while(end) {
	// 	cout << end - t << endl;
	// 	end = end -> prev;
	// 	c++;
	// }

	// table* ptr = table_st;
	// while(ptr -> next) {
	// 	cout << ptr - t << ' ';
	// 	ptr = ptr -> next;
	// }
	// cout << ptr - t << " | " << table_st - t << " | " << table_end - t << endl;
	// while(ptr) {
	// 	cout << ptr - t << ' ';
	// 	ptr = ptr -> prev;
	// }
	// cout << " | " << table_st - t << " | " << table_end - t << endl;
	// cout << endl;
}

void compaction() {
	pthread_mutex_lock(&compaction_lock);
	if(gc_on) {
		cout << "Initiating Compaction:\n";
		cout << "->->->->->->->->->->->->->->->->->->->\n";
		cout << "Before sorting PT:\n";
		print_table();
		// Step-1: Rearrange the occupied spaces in sorted order
		table* end = table_end;
		if(table_cnt != TABLE_SIZE)
			end = end -> prev;
		table_st = merge_sort(table_st, end, table_cnt);

		table* ptr = table_st;
		while(ptr -> next) {
			ptr = ptr -> next;
		}
		if(table_cnt == TABLE_SIZE)
			table_end = ptr;
		else {
			table_end -> prev = ptr;
			ptr -> next = table_end;
		}
		cout << "After sorting PT: \n";
		print_table();

		// Step-2: Greedily move the pages to free spaces

		table* tptr = table_st;
		int t_c = 0;
		int last_fr = 0; // last free byte
		int last_occ = 0; // last occupied byte

		while(t_c < table_cnt) {
			int req_bytes = (tptr -> numbytes + (tptr -> numbits > 0) + WORD_SZ - 1) / WORD_SZ * WORD_SZ;
			if(last_fr < tptr -> addr) {
				// Copy byte by byte
				for(int i = 0; i < req_bytes; ++i) {
					user_mem[last_fr + i] = user_mem[tptr -> addr + i];
				}
				tptr -> addr = last_fr;
			}
			
			last_occ = tptr -> addr + req_bytes - 1;
			last_fr += req_bytes;
			t_c++;
			tptr = tptr -> next;
		}
		q_end = 0;
		q[q_end] = node(last_occ + 1, SIZE - 1);

		cout << "PT After compaction:\n";
		print_table();
	}
	pthread_mutex_unlock(&compaction_lock);
}

void valid_ref(const int id) {
	if(id == -1) {
		cout << "## Error: Invalid reference. Aborting...\n";
		exit(-1);
	}
}

void copy_data(char* dest, char* data, short sz) {
	for(int i = 0; i < sz; ++i)
		dest[i] = data[i];
}

void print_helper1(table* tmp, int id, int val, short type, int index, bool is_arr) {
	int req_words = (tmp -> numbytes + (tmp -> numbits > 0) + WORD_SZ - 1) / WORD_SZ;
	cout << "$$ get_val: Value of varID ";
	cout << id;
	if(is_arr)
		cout << "[" << index << "]";
	cout << " (" << tmp -> addr << "," << tmp -> addr + req_words * WORD_SZ - 1 << "): ";
	if(type)
		cout <<  val;
	else
		cout << val;
	cout << '\n';
}

void print_helper2(table* tmp, int id, int val, short type, int index, bool is_arr) {
	int req_words = (tmp -> numbytes + (tmp -> numbits > 0) + WORD_SZ - 1) / WORD_SZ;
	cout << "$$ assignVar: Assigned value ";
	if(!type)
		cout << val;
	else
		cout << (char)val;
	cout << " for varID " << id;
	if(is_arr)
		cout << "[" << index << "]";
	cout << " [" << tmp -> addr << "," << tmp -> addr + req_words * WORD_SZ - 1 << "]\n";
}

int insert_stk(int info) {
	if(!gc_on) return -2;
	stk_end++;
	if(stk_end == STK_SZ) {
		if(stk_cnt < stk_end) {
			cout << "$$ insert_stk: Stack limit exceeded. Free space detected. Initiating compaction...\n";
			compact_stk();
		}
		else {
			cout << "$$ insert_stk: Stack limit exceeded. Calling realloc...\n";
			STK_SZ *= 2;
			stk = (stk_node *)realloc(stk, STK_SZ * sizeof(stk_node));
			if(stk == NULL) {
				cout << "## Error: Realloc for stk failed. Aborting...\n";
				exit(-1);
			}
		}
	}
	stk[stk_end] = stk_node(info, true);
	pthread_mutex_lock(&stk_cnt_lock);
	stk_cnt++;
	pthread_mutex_unlock(&stk_cnt_lock);
	return stk_end;
}

void gc_run() {
	if(!gc_on) return;
	cout << "$$ gc_run: Memory clean_up requested. Freeing up heap...\n";
	while(stk_end >= 0 and stk[stk_end].addr != -1) {
		pthread_mutex_lock(&stk[stk_end].lock);
		if(stk[stk_end].addr >= 0) {
			freeElem(stk[stk_end].addr);
			stk[stk_end].alive = false;
			stk[stk_end].addr = -2; // Indicate this node is freed
			pthread_mutex_lock(&stk_cnt_lock);
			stk_cnt--;
			pthread_mutex_unlock(&stk_cnt_lock);
		}
		pthread_mutex_unlock(&stk[stk_end].lock);
		stk_end--;
	}

	if(stk_end >= 0)
		stk_end--;

	// Check if the stack can be furthur contracted (may be freed by GC)
	while(stk_end >= 0) {
		if(stk[stk_end].addr == -2)
			stk_end--;
		else
			break;
	}
}

void compact_stk() {
	if(!gc_on) return;
	cout << "$$ compact_stk: Stack compaction initiated...\n";
	cout << "$$ compact_stk: Stack end before compaction: " << stk_end << '\n';
	// Layout -> Alive ..... Free .... Alive
	pthread_mutex_lock(&stk_lock);
	stk_end--;
	while(stk_end >= 0 and stk[stk_end].addr == -2) stk_end--;
	int i = 0;
	for(; i <= stk_end; ++i) {
		if(stk[i].addr == -2) break; // First free slot
	}

	if(stk_end >= 0 and i < stk_end) {
		if(stk[stk_end].addr == -2) {
			stk_end = i - 1;
		}
		else {
			int j = stk_end;
			while(stk[j].addr != -1) j--;
			// Compact the stack
			while(j <= stk_end)
				stk[i++] = stk[j++];
			stk_end = i - 1;
		}
	}
	cout << "$$ compact_stk: Stack end after compaction: " << stk_end << '\n';
	pthread_mutex_unlock(&stk_lock);
}

void gc_init() {
	cout << "$$ gc_init: Initializing Garbage Collector...\n";
	insert_stk(-1);
}

void* GC(void *arg) {
	if(!gc_on) return 0;
	cout << "$$ GC: Garbage Collector thread started...\n";
	while(true) {
		msleep(10);
		pthread_mutex_lock(&stk_lock);
		for(int i = 0; i <= stk_end; ++i) {
			clock_t start = clock();
			if(!stk[i].alive and stk[i].addr >= 0) {
				// free this region
				pthread_mutex_lock(&stk[i].lock);
				if(!stk[i].alive and stk[i].addr >= 0 and i <= stk_end) {
					if(i > 0 and stk[i - 1].addr == -1){
						stk[i - 1].addr = -2;
						stk[i - 1].alive = false;
					}
					freeElem(stk[i].addr);
					stk[i].addr = -2; // Indicate this node is freed
					cout << "$$ GC Collector Thread: Freeing up space...\n";
					pthread_mutex_lock(&stk_cnt_lock);
					stk_cnt--;
					pthread_mutex_unlock(&stk_cnt_lock);
				}
				pthread_mutex_unlock(&stk[i].lock);
			}
			clock_t end = clock();
			gc_timer += double(end - start)/CLOCKS_PER_SEC;
		}

		pthread_mutex_unlock(&stk_lock);
	}
	return 0;
}

int msleep(long msec)
{
    struct timespec ts;
    int res;

    if (msec < 0)
    {
        errno = EINVAL;
        return -1;
    }

    ts.tv_sec = msec / 1000;
    ts.tv_nsec = (msec % 1000) * 1000000;

    do {
        res = nanosleep(&ts, &ts);
    } while (res && errno == EINTR);

    return res;
}