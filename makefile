#-g = gdb sumbols yadda yadda
CLANG_OBJ=clang -Werror -g -std=c++11 
CLANG_LINK=clang -lstdc++ -lm -Werror -g

link: build
	$(CLANG_LINK) -I../src/ build/main.o \
		build/tokenizer.o \
		build/file_handling.o \
		build/ast.o \
		build/type_system.o
		-o bin/achilles
	@echo "----\n"

build: src/*
	#clang -lstdc++ -std=c++11 -lm src/main.cpp  -Werror -g -c  build/main.o
	 $(CLANG_OBJ) -c src/main.cpp  -o build/main.o
	 $(CLANG_OBJ) -c src/tokenizer.cpp  -o build/tokenizer.o
	 $(CLANG_OBJ) -c src/file_handling.cpp  -o build/file_handling.o
	 $(CLANG_OBJ) -c src/ast.cpp  -o build/ast.o
	 $(CLANG_OBJ) -c src/type_system.cpp  -o build/type_system.o
	 #$(CLANG_OBJ) -c src/pretty_print.cpp  -o build/pretty_print.o

