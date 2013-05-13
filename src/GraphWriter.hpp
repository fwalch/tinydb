#ifndef H_GraphWriter
#define H_GraphWriter

#include "Exception.hpp"
#include "Parser.hpp"
#include <ostream>

class GraphWriter {
  private:
    Parser::Result& result;

  public:
    GraphWriter(Parser::Result& result)
      : result(result) { }

    void writeQueryGraph(std::ostream& output);
};

#endif
