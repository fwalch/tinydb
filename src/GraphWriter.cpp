#include "GraphWriter.hpp"
#include "Exception.hpp"

using namespace std;

void GraphWriter::writeQueryGraph(std::ostream& output) {
  if (!output.good()) {
    throw Exception("Cannot write to stream.");
  }
  output << "graph {" << endl;

  output << "  // Relations" << endl;
  for (auto node : graph.nodes) {
    output << "  " << node.first << " [label=\"" + node.first + ": " + to_string(node.second->cardinality) + "\"];" << endl;
  }

  output << "  // Join edges" << endl;
  for (auto edge : graph.edges) {
    output << "  " << edge->node1->relation <<
      " -- " << edge->node2->relation <<
      "[label=\"" + to_string(edge->selectivity) + "\"]"<< ";" << endl;
  }

  output << "}" << endl;
}

