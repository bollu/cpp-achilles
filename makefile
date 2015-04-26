#-g = gdb sumbols yadda yadda
CLANG_OBJ=clang -Werror -g -std=c++11

#LLVM_LINKS=-I/usr/lib/llvm-3.5/include -DNDEBUG -D_GNU_SOURCE -D__STDC_CONSTANT_MACROS -D__STDC_FORMAT_MACROS -D__STDC_LIMIT_MACROS -g -O2 -fomit-frame-pointer -std=c++11 -fvisibility-inlines-hidden -fno-exceptions -fPIC -Woverloaded-virtual -ffunction-sections -fdata-sections -Wcast-qual -L/usr/lib/llvm-3.5/lib -lLLVMCore -lLLVMSupport -lz -lpthread -lffi -ledit -ltinfo -ldl -lm
LLVM_LIBS=`llvm-config --cxxflags --ldflags --system-libs --libs core`
LIBRARIES= -lstdc++ -lm  $(LLVM_LIBS) 
CLANG_LINKER=clang  -Werror -g

link: build
	$(CLANG_LINKER) -I../src/ build/main.o \
		build/tokenizer.o \
		build/file_handling.o \
		build/ast.o \
		build/type_system.o \
		$(LIBRARIES) \
		-o bin/achilles
	@echo "----\n"

dummy:

build: dummy uncrustify src/*
	 $(CLANG_OBJ) -c src/main.cpp  -o build/main.o
	 $(CLANG_OBJ) -c src/tokenizer.cpp  -o build/tokenizer.o
	 $(CLANG_OBJ) -c src/file_handling.cpp  -o build/file_handling.o
	 $(CLANG_OBJ) -c src/ast.cpp  -o build/ast.o
	 $(CLANG_OBJ) -c src/type_system.cpp  -o build/type_system.o
	 #$(CLANG_OBJ) -c src/intermediate.cpp -o build/intermediate.o
	 #$(CLANG_OBJ) -c src/pretty_print.cpp  -o build/pretty_print.o

uncrustify: dummy src/*
	uncrustify -c uncrustify/neovim.cfg --replace --no-backup  src/*
