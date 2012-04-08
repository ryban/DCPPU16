LIBFLAGS = -lsfml-graphics -lsfml-window -lsfml-system -lGLU -lGL


dcppu: dcpu.o emu.cpp
	g++ -o dcppu emu.cpp dcpu.o $(LIBFLAGS)

dcpu.o: dcpu.cpp
	g++ -c dcpu.cpp

clean:
	rm -rf dcpu.o dcppu
