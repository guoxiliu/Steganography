CC = g++
CFLAGS = -g -Wall -std=c++11

# first set up the platform dependent variables
ifeq ("$(shell uname)", "Darwin")
	LDFLAGS = -framework Foundation -framework GLUT -framework OpenGL -lOpenImageIO -lm
else
	ifeq ("$(shell uname)", "Linux")
		LDFLAGS = -L /usr/lib64/ -lglut -lGL -lGLU -lOpenImageIO -lm
	endif
endif

PROJECT = imghide

# this will compile the source file and generate the executable file
compile: ${PROJECT}.cpp
	${CC} ${CFLAGS} -o ${PROJECT}.out ${PROJECT}.cpp ${LDFLAGS}

# this will clean up all created temporary files
clean:
	rm -f *.o ${PROJECT}
