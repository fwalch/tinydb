#include "Parser.hpp"
#include "GraphWriter.hpp"
#include <iostream>
#include <fstream>
//---------------------------------------------------------------------------
using namespace std;
//---------------------------------------------------------------------------
static void showHelp(const char* argv0)
   // Display a short help
{
   cout << "usage: " << argv0 << " [query] [outfile]" << endl;
   cout << "If outfile isn't specified, stdout is used" << endl;
}
//---------------------------------------------------------------------------
int main(int argc, char* argv[])
   // Entry point
{
   if (argc < 2 || argc > 3) { showHelp(argv[0]); return 1; }

   const string query = argv[1];
   Parser parser(query);

   Parser::Result result;
   try {
     parser.parse(result);
   }
   catch (Parser::SyntacticError e) {
     cerr << "Syntactic error: " << e.what() << endl;
     return 1;
   }

   GraphWriter graphWriter(result);
   try {
     if (argc == 3) {
       fstream output(argv[2], ios_base::out);
       graphWriter.writeQueryGraph(output);
       output.close();
     }
     else {
       graphWriter.writeQueryGraph(cout);
     }
   }
   catch (Exception e) {
     cerr << "An error occured: " << e.what() << endl;
     return 1;
   }

   return 0;
}
//---------------------------------------------------------------------------
