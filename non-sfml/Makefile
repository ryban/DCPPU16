dcppu: dcpu.o emu.cpp
	g++ -o dcppu emu.cpp dcpu.o

dcpu.o: dcpu.cpp dcpu.h
	g++ -c dcpu.cpp

clean:
	rm -rf dcpu.o dcppu
