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
//#include "codegen.h"

//-----------------------------------------------------
//ENTRY POINT
int main(int argc, char **argv)
{
   assert(argc == 2 && "expected input file path");

   std::ifstream input_file(argv[1]);
   std::string   file_data((std::istreambuf_iterator<char>(input_file)), (std::istreambuf_iterator<char>()));

   //std::string file_data(argv[1]);

   //std::vector<Token> tokens = tokenize_string("{a+b;c + d; e * f > g && h;} if let abba");
   std::vector<Token> tokens = tokenize_string(file_data);
   std::cout << "tokens:\n";
   for (auto& token : tokens)
   {
      std::cout << token << " ";
   }

   std::cout << "\nparsing";
   std::shared_ptr<IAST> ast = parse(tokens, file_data);
   std::cout << "\ntype checking";
   TSContext ctx = type_system_type_check(ast);

   std::cout << pretty_print(*ast);

   return 0;
}
