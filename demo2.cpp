#include <iostream>
#include "memlab.h"
#include <climits>
using namespace std;
#define MAX_SZ 50
#define SZ 500

void fibonacci(my_int n,my_int& arr){
    my_int t1 = 0, t2 = 1, nextTerm = 0;

    for (int i = 1; i <= n.getval(); ++i) {
        if(i == 1) {
            arr.assignval(0, i);
            continue;
        }
        if(i == 2) {
            arr.assignval(1, i);
            continue;
        }
        nextTerm = t1.getval() + t2.getval();
        t1 = t2.getval();
        t2 = nextTerm.getval();
        if(nextTerm.getval()<0){
            perror("k");
            break;
        }
        arr.assignval(nextTerm.getval(), i);
    }
}

my_int fibonacciProduct(my_int k, my_int& arr){
    gc_init();
    fibonacci(k,arr);
    gc_run();
    my_int p = 1;
    for(int i = 2; i <= k.getval(); i++){
        p = p.getval() * arr.getval(i);
        // cout<<arr.getval(i)<<endl;
    }
    return p;
}

int main() {
    freopen("output_2.txt", "w", stdout);
	freopen("error_2.txt", "w", stderr);
	gc_on = true;
    if(createMem(SZ, "MB") == -1)
		exit(-1);
    gc_init();
    my_int arr(MAX_SZ, 1);
    arr.assignval(100, 30);
    my_int k = 5;
    gc_init();
    cout << "Product of fibonacci number: " << fibonacciProduct(k.getval(), arr).getval() << endl;
    gc_run();
    sleep(1);
    gc_run();
    cerr << "GC thread runtime: " << gc_timer << endl;
    return 0;
}