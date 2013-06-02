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

   if (result.explain) {
     // Output query graph
     GraphWriter graphWriter(queryGraph);

     try {
       fstream queryGraphFile("queryGraph.dot", ios_base::out);
       graphWriter.writeQueryGraph(queryGraphFile);
       queryGraphFile.close();

       cout << "Query graph should be shown now; otherwise try to use bin/query-graph.sh." << endl;
       system("((cat queryGraph.dot | circo -Tpng | display -) && rm queryGraph.dot) &");
     }
     catch (Exception e) {
       cerr << "An error occured: " << e.what() << endl;
       return 1;
     }
   }

   // Output query result
   PlanGen planGen(db, queryGraph, result);

   unique_ptr<Operator> output;
   try {
     fstream joinTreeFile;

     if (result.explain) {
       joinTreeFile.open("joinTree.dot", ios_base::out);
     }
     else {
       joinTreeFile.open("/dev/null", ios_base::out);
     }

     output = planGen.generate(joinTreeFile);
     joinTreeFile.close();

     if (result.explain) {
       cout << "Join tree should be shown now; otherwise try to use bin/join-tree.sh" << endl;
       system("((cat joinTree.dot | dot -Tpng | display -) && rm joinTree.dot) &");
     }
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
