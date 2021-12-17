build:
	gcc process_generator.c -o bin/process_generator.out
	gcc clk.c -o bin/clk.out
	gcc scheduler.c -o bin/scheduler.out -lm
	gcc process.c -o bin/process.out
	gcc test_generator.c -o bin/test_generator.out

clean:
	rm -f bin/*.out  processes.txt

all: clean build

run:
	./bin/process_generator.out
