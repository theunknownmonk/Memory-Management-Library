#ifndef __MEMLAB_H
#define __MEMLAB_H

#include <iostream>
#include <pthread.h>
#include <unistd.h>
using namespace std;

class table;
class node;
class stk_node;

extern char* user_mem; // points to allocated mem
extern char* info_mem; // Additional data for VM e.g page_table, variable info
extern bool gc_on; // GC on or off boolean

const int MAX_T_SZ = 1 << 12;
const int MAX_ST_SZ = 1 << 12;
const int MAX_Q_SZ = 1 << 12;
extern int Q_SZ; // Max number of free mem segments allowed i.e fragmentation limit
extern int TABLE_SIZE; // Max number of allocated mem slots
const int WORD_SZ = 4;	// Word size
const short NAME_SZ = 5;

// types
const short TYPE_INT = 0;
const short TYPE_MED_INT = 1;
const short TYPE_BOOLEAN = 2;
const short TYPE_CHAR = 3;

// Array of free mem segments
extern node* q;
extern int q_end;
extern pthread_mutex_t q_end_lock;

// Page table
extern table* t;
extern table* table_st;
extern table* table_end;
extern int table_cnt;

extern double gc_timer;
// Stack
extern stk_node* stk;
extern int stk_end;
extern int STK_SZ;
extern int stk_cnt;
extern pthread_mutex_t stk_lock; // For compaction of stack
extern pthread_mutex_t stk_cnt_lock; // For actual stack cnt
extern pthread_mutex_t compaction_lock; // Lock for compaction

// Thread id for GC
extern pthread_t tid;
const int sleep_t = 1;

extern int free_space; // Total free-space avl.

class table {
public:
	int addr;
	char type;
	int numbytes; // No of bytes
	short numbits; // No of bits
	table* next;
	table* prev;
	table() {
		addr = -1, numbytes = numbits = -1, next = prev = NULL;
	}
};

class node {
public:
	int l, r;
	node(): l(-1), r(-1) {}
	node(int l_, int r_): l(l_), r(r_) {}
};

class stk_node {
public:
	int addr;
	bool alive;
	pthread_mutex_t lock;
	stk_node(int, bool);
};

class my_type {
protected:
	int id;
	int stk_off;
public:
	my_type(short, int = 1);
	void freeElem();
	virtual ~my_type() {
		if(stk[stk_off].alive and stk[stk_off].addr == id){
			stk[stk_off].alive = false;
		}
	}
};

class my_int: public my_type {
public:
	my_int();
	my_int(int, bool = false);
	int getval(int = -1);
	void assignval(int, int = -1);
	void operator=(int);
	~my_int() {}
};

class my_medint: public my_type {
public:
	my_medint();
	my_medint(int, bool = false);
	int getval(int = -1);
	void assignval(int, int = -1);
	void operator=(int);
	~my_medint() {}
};

class my_char: public my_type {
public:
	my_char();
	my_char(int, bool = false);
	char getval(int = -1);
	void assignval(int, int = -1);
	void operator=(char c);
	~my_char() {}
};

class my_bool: public my_type {
public:
	my_bool();
	my_bool(int, bool = false);
	bool getval(int = -1);
	void assignval(int, int = -1);
	void operator=(bool);
	~my_bool() {}
};

int createMem(size_t, const string&);
int allocate_space(const short, int = 1);
int alloc_space_helper(const short, int, int, int);
void assignVar(const int, const int, int = -1);
void valid_ref(const int);
void copy_data(char*, char*, short);
void freeElem(int);
void compaction();

// Stack functions
int insert_stk(int);
void gc_run();
void gc_init();
void compact_stk(); // Fills the holes in stk

// Garbage collector
void * GC(void *arg);

// Print helper
void print_helper1(table*, int, int, short, int, bool);
void print_helper2(table*, int, int, short, int, bool);
void print_table();

int msleep(long);
#endif
