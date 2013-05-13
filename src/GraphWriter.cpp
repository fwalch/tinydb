#include "GraphWriter.hpp"
#include "Exception.hpp"

using namespace std;

void GraphWriter::writeQueryGraph(std::ostream& output) {
  if (!output.good()) {
    throw Exception("Cannot write to stream.");
  }
  output << "graph {" << endl;

  output << "  // Relations" << endl;
  for (auto rel : result.relations) {
    output << "  " << rel.getName() << ";" << endl;
  }

  output << "  // Join edges" << endl;
  for (auto join : result.joinConditions) {
    output << "  " << join.first.relation <<
      " -- " << join.second.relation << ";" << endl;
  }

  output << "}" << endl;
}
