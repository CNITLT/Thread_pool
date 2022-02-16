test:test.cpp Thread.cpp Task.cpp Thread_pool.cpp debug.h
	g++ -g -o test test.cpp Task.cpp Thread_pool.cpp Thread.cpp -lpthread
test_debug:test.cpp Thread.cpp Task.cpp Thread_pool.cpp debug.h
	g++ -g -o test test.cpp Task.cpp Thread_pool.cpp Thread.cpp -lpthread -DDEBUG
clear:
	rm -f ./test