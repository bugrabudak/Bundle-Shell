all: hw1.cpp 
	g++ *.c hw1.cpp -o hw1 -g

  clean: 
	  $(RM) hw1
