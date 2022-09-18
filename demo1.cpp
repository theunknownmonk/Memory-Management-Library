#include <iostream>
#include "memlab.h"
#include <climits>
using namespace std;

const int SZ = 256;
const int MAX_SZ = 50000;

void my_func1(my_int x, my_int y) {
	gc_init();
	my_int arr(MAX_SZ, 1);
	for(int i = 0; i < MAX_SZ; ++i)
		arr.assignval(i, i);
	gc_run();
}

void my_func2(my_medint x, my_medint y) {
	gc_init();
	my_medint arr(MAX_SZ, 1);
	for(int i = 0; i < MAX_SZ; ++i)
		arr.assignval(i, i);
	gc_run();
}

void my_func3(my_char x, my_char y) {
	gc_init();
	my_char arr(MAX_SZ,1);
	for(int i = 0; i < MAX_SZ; ++i)
		arr.assignval('a' + i % 26, i);
	gc_run();
}

void my_func4(my_bool x, my_bool y) {
	gc_init();
	my_bool arr(MAX_SZ,1);
	for(int i = 0; i < MAX_SZ; ++i)
		arr.assignval(i & 1, i);
	gc_run();
}

void my_func5(my_int x, my_int y) {
	gc_init();
	my_int arr(MAX_SZ, 1);
	for(int i = 0; i < MAX_SZ; ++i)
		arr.assignval(i, i);
	gc_run();
}

void my_func6(my_medint x, my_medint y) {
	gc_init();
	my_medint arr(MAX_SZ, 1);
	for(int i = 0; i < MAX_SZ; ++i)
		arr.assignval(i, i);
	gc_run();
}

void my_func7(my_char x, my_char y) {
	gc_init();
	my_char arr(MAX_SZ, 1);
	for(int i = 0; i < MAX_SZ; ++i)
		arr.assignval('a' + i % 26, i);
	gc_run();
}

void my_func8(my_bool x, my_bool y) {
	gc_init();
	my_bool arr(MAX_SZ,1);
	for(int i = 0; i < MAX_SZ; ++i)
		arr.assignval(i & 1, i);
	gc_run();
}

void my_func9(my_int x, my_int y) {
	gc_init();
	my_int arr(MAX_SZ, 1);
	for(int i = 0; i < MAX_SZ; ++i)
		arr.assignval(i, i);
	gc_run();
}

void my_func10(my_medint x, my_medint y) {
	gc_init();
	my_medint arr(MAX_SZ, 1);
	for(int i = 0; i < MAX_SZ; ++i)
		arr.assignval(i, i);
	gc_run();
}

int main() {
	freopen("output_1.txt", "w", stdout);
	freopen("error_1.txt", "w", stderr);
	gc_on = true;
	if(createMem(SZ, "MB") == -1)
		exit(-1);
	gc_init();
	my_func1(5, 6);
	my_func2(10, 11);
	my_func3('a', 'b');
	my_func4(1&1, 1&0);
	my_func5(15, 16);
	my_func6(2, 3);
	my_func7('c', 'd');
	my_func8(1&0, 1&1);
	my_func9(25, 26);
	my_func10(10, 11);
	gc_run();
	cerr << "GC thread runtime: " << gc_timer << endl;
}



