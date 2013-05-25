#ifndef H_GraphWriter
#define H_GraphWriter

#include "Exception.hpp"
#include "QueryGraph.hpp"
#include <ostream>

class GraphWriter {
  private:
    QueryGraph& graph;

  public:
    GraphWriter(QueryGraph& graph)
      : graph(graph) { }

    void writeQueryGraph(std::ostream& output);
};

#endif
