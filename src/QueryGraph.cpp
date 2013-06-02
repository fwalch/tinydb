#include "QueryGraph.hpp"

using namespace std;

QueryGraph::~QueryGraph() {
  for (auto node : nodes) {
    delete node.second;
  }

  for (auto edge : edges) {
    delete edge;
  }
}

void QueryGraph::createNode(Parser::Relation& relation, Table& table) {
  Node* node = new Node();
  node->cardinality = table.getCardinality();
  node->relation = relation.getName();
  node->table = &table;

  nodes.insert(make_pair(relation.getName(), node));
}

inline unsigned max(unsigned a, unsigned b) {
  return a > b ? a : b;
}

float estimateSelectivity(Attribute& attr1, Attribute& attr2, QueryGraph::Node* node1, QueryGraph::Node* node2) {
  if (attr1.getKey() && attr2.getKey()) {
    return 1.0/max(node1->cardinality, node2->cardinality);
  }

  if (!attr1.getKey() && attr2.getKey()) {
    return 1.0/node2->cardinality;
  }

  if (attr1.getKey() && !attr2.getKey()) {
    return 1.0/node1->cardinality;
  }

  return 1.0/max(attr1.getUniqueValues(), attr2.getUniqueValues());
}

//TODO: store proper information
void QueryGraph::addSelection(Parser::Attribute& attr, Parser::Constant& c) {
  nodes.at(attr.relation)->predicates.push_back(attr.getName() + " = " + c.value);
}

void QueryGraph::createEdge(Parser::Attribute& attr1, Parser::Attribute& attr2, Attribute& tattr1, Attribute& tattr2) {
  Node* node1 = nodes.at(attr1.relation);
  Node* node2 = nodes.at(attr2.relation);

  Edge* edge = new Edge {
    node1,
    node2,
    attr1.name,
    attr2.name,
    attr1.getName(),
    attr2.getName(),
    estimateSelectivity(tattr1, tattr2, node1, node2)
  };
  edges.push_back(edge);
  node1->edges.push_back(edge);
  node2->edges.push_back(edge);
}
