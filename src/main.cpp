#include <iostream>
#include <vector>
#include <string>
#include <memory>
#include <sstream>
#include <assert.h>
#include <map>

#include "tokenizer.h"
#include "file_handling.h"
#include "ast.h"
#include "pretty_print.h"
#include "type_system.h"
//#include "codegen.h"

//-----------------------------------------------------
//ENTRY POINT
int main(int argc, char **argv) {
    assert(argc == 2 && "expected input tokens");
    //std::vector<Token> tokens = tokenize_string("{a+b;c + d; e * f > g && h;} if let abba");
    std::vector<Token> tokens = tokenize_string(argv[1]);
    std::cout<<"tokens:\n";
    for (auto &token : tokens) {
        std::cout<<token<<" ";
    }

    std::cout<<"\nparsing:\n";
    std::shared_ptr<IAST> ast = parse(tokens);
    std::cout<<pretty_print(*ast);
    //std::cout<<ast->stringify();
    return 0;
}

