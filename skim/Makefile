COMPILER=$(shell root-config --cxx)
FLAGS=$(shell root-config --cflags --libs)

tnptreeSkim: tnptreeSkim.cxx
		$(COMPILER) -g -O3 -Wall -Wextra -Wpedantic -o tnptreeSkim tnptreeSkim.cxx $(FLAGS)

clean:
		rm -rf core  *.o tnptreeSkim
