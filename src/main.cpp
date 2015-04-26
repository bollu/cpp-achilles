#include <iostream>
#include <vector>
#include <string>
#include <memory>
#include <sstream>
#include <assert.h>
#include <map>
#include <fstream>

#include "tokenizer.h"
#include "file_handling.h"
#include "ast.h"
#include "pretty_print.h"
#include "type_system.h"
#include "llvm_codegen.h"

// #include "codegen.h"

// -----------------------------------------------------
// ENTRY POINT
int main(int argc, char **argv) {
    assert(argc == 2
           && "expected input file path");

    std::ifstream input_file(argv[1]);
    std::string   file_data((std::istreambuf_iterator<char>(
                                 input_file)),
                            (std::istreambuf_iterator<char>()));

    std::vector<Token>tokens = tokenize_string(file_data);
    std::cout << "tokens:\n";

    for (auto& token : tokens) {
        std::cout << token << " ";
    }

    std::cout << "\n-------\n\nparse tree:\n";


    std::shared_ptr<IAST>ast = parse(tokens, file_data);
    TSContext ctx = type_system_type_check(ast);
    ctx.get_root_scope();

    std::cout << pretty_print(*ast);

    // generate_llvm_code(*ast, ctx);

    return 0;
}
