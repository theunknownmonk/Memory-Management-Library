#include <iostream>
#include "memlab.h"
#include <climits>
using namespace std;

const int SZ = 20;

int main() {
	freopen("output_3.txt", "w", stdout);
	freopen("error_3.txt", "w", stderr);
	gc_on = true;
	if(createMem(SZ, "B") == -1)
		exit(-1);
	gc_init();
	my_int x = 5;
	my_int y = 2;
	my_int z = 1;
	my_medint p = 3;
	my_char l = 'a';

	// free two non-consecutive mem segments
	y.freeElem();
	p.freeElem();

	// Declare an array to invoke compaction
	my_int arr(2, 1);
	gc_run();
}



