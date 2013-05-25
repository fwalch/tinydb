#ifndef H_QueryGraph
#define H_QueryGraph

#include "Parser.hpp"
#include "Database.hpp"
#include <unordered_map>
#include <vector>

class QueryGraph {
  public:
    struct Edge;

    struct Node {
      unsigned cardinality;
      Table* table;
      std::string relation;

      std::unordered_map<std::string, Edge*> edges;
      std::vector<std::string> predicates;
    };

    struct Edge {
      Node* node1;
      Node* node2;
      std::string predicate;

      float selectivity;
    };

  private:
    std::unordered_map<std::string, Node*> nodes;
    std::unordered_map<std::string, Edge*> edges;

  public:
    ~QueryGraph();
    void createNode(Parser::Relation& relation, Table& table);
    void createEdge(Parser::Attribute& attr1, Parser::Attribute& attr2, Attribute& tableAttr1, Attribute& tableAttr2);
    void addSelection(Parser::Attribute& attr, Parser::Constant& c);

    friend class GraphWriter;
};

#endif
