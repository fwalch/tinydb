#include "Database.hpp"
#include "Parser.hpp"
#include "PlanGen.hpp"
#include "SemanticAnalyzer.hpp"
#include "GraphWriter.hpp"
#include <iostream>
#include <fstream>
#include <memory>
//---------------------------------------------------------------------------
using namespace std;
//---------------------------------------------------------------------------
static void showHelp(const char* argv0)
   // Display a short help
{
   cout << "usage: " << argv0 << " [db] [query]" << endl
        << "supported query types" << endl
        << "select|explain (*|attribute(,attribute)*)" << endl
        << "from relation binding(,relation binding)*" << endl
        << "where binding.attribute=(binding.attribute|constant)" << endl
        << "(and binding.attribute=(binding.attribute|constant))*" << endl;
}
//---------------------------------------------------------------------------
int main(int argc, char* argv[])
   // Entry point
{
   if (argc != 3) { showHelp(argv[0]); return 1; }

   Database db;
   db.open(argv[1]);

   const string query = argv[2];
   Parser parser(query);

   Parser::Result result;
   try {
     parser.parse(result);
   }
   catch (Parser::SyntacticError e) {
     cerr << "Syntactic error: " << e.what() << endl;
     return 1;
   }

   SemanticAnalyzer analyzer(db, result);

   QueryGraph queryGraph;
   try {
     analyzer.execute(queryGraph);
   }
   catch (SemanticAnalyzer::SemanticError e) {
     cerr << "Semantic error: " << e.what() << endl;
     return 1;
   }

   // Output query result
   PlanGen planGen(db, queryGraph, result);

   unique_ptr<Operator> output;
   try {
     fstream joinTreeFile;
     output = planGen.generate(cout);
   }
   catch (PlanGen::GenError e) {
     cerr << "Plan generation error: " << e.what() << endl;
     return 1;
   }

   output->open();
   while (output->next());
   output->close();

   db.close();

   return 0;
}
//---------------------------------------------------------------------------
