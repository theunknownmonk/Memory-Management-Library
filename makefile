CFLAGS= -pthread
REQS = memlab.h memlab.cpp
demo1: $(REQS) demo1.cpp
	g++ demo1.cpp $(REQS) -o demo1 $(CFLAGS) 
	./demo1
demo2: $(REQS) demo2.cpp
	g++ demo2.cpp $(REQS) -o demo2 $(CFLAGS) 
	./demo2
demo3: $(REQS) demo3.cpp
	g++ demo3.cpp $(REQS) -o demo3 $(CFLAGS) 
	./demo3
clean:
	$(RM) error_1.txt output_1.txt error_3.txt output_3.txt error_2.txt output_2.txt demo1 demo2
help:
	echo "Execute Demo 1: make demo1\nExecute Demo 2: make demo2\nTo clean: make clean"
