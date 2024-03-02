.PHONY: build
build:
	g++ -ggdb main.cpp -o main.tsk

.PHONY: run
run: build
	./main.tsk
