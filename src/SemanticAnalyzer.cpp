#include "SemanticAnalyzer.hpp"
#include "Database.hpp"

using namespace std;

void SemanticAnalyzer::init() {
  // Build helper map
  for (Parser::Relation& rel : result.relations) {
    string key = rel.getName();

    if (relations.find(key) != relations.end()) {
      throw SemanticError("Duplicate binding in FROM clause: '" + key + "'");
    }

    try {
      database.getTable(rel.relation);
    }
    catch (...) {
      throw SemanticError("Table '" + rel.relation + "' does not exist");
    }

    relations[key] = rel;
  }
}

void SemanticAnalyzer::analyzeAttribute(Parser::Attribute& attr, string context, ::Attribute& tableAttr) {
  if (attr.name == Parser::Attribute::STAR) return;

  if (attr.relation == "") {
    // Search for attribute
    bool found = false;

    for (auto relIt : relations) {
      Table& table = database.getTable(relIt.second.relation);
      for (unsigned i = 0; i < table.getAttributeCount(); i++) {
        tableAttr = table.getAttribute(i);
        if (tableAttr.getName() == attr.name) {
          if (found) {
            throw SemanticError("Attribute '" + attr.name + "' used in " + context + " is ambiguous");
          }
          found = true;
          break;
        }
      }
    }

    if (!found) {
      throw SemanticError("Attribute '" + attr.name + "' used in " + context + " clause not found in any relation in FROM clause.");
    }
  }
  else {
    auto rIt = relations.find(attr.relation);
    if (rIt == relations.end()) {
      throw SemanticError("Relation or binding '" + attr.relation + "' used in " + context + " clause is not contained in FROM clause.");
    }

    Table& table = database.getTable(rIt->second.relation);
    int attrIndex;
    if ((attrIndex = table.findAttribute(attr.name)) == -1) {
      throw SemanticError("Attribute '" + attr.name + "' does not exist in relation '" + rIt->second.relation + "'");
    }

    tableAttr = table.getAttribute(attrIndex);
  }
}

void SemanticAnalyzer::execute(QueryGraph& queryGraph) {
  init();

  ::Attribute tableAttr;
  for (Parser::Attribute& attr : result.projections) {
    analyzeAttribute(attr, "SELECT", tableAttr);
  }

  for (auto relation : result.relations) {
    queryGraph.createNode(relation, database.getTable(relation.relation));
  }

  // Check that all selections are valid
  for (auto selection : result.selections) {
    Parser::Attribute& attr = selection.first;
    Parser::Constant& c = selection.second;

    ::Attribute tableAttr;
    analyzeAttribute(attr, "WHERE", tableAttr);

    if (tableAttr.getType() != c.type) {
      throw SemanticError("Attribute '" + attr.name + "' and constant '" + c.value + "' do not have the same type (WHERE clause)");
    }

    queryGraph.addSelection(attr, c);
  }

  // Check that all joins are valid
  for (auto joinCondition : result.joinConditions) {
    Parser::Attribute& attr1 = joinCondition.first;
    Parser::Attribute& attr2 = joinCondition.second;

    ::Attribute tableAttr1;
    ::Attribute tableAttr2;
    analyzeAttribute(attr1, "WHERE", tableAttr1);
    analyzeAttribute(attr2, "WHERE", tableAttr2);

    if (tableAttr1.getType() != tableAttr2.getType()) {
      throw SemanticError("Attributes '" + attr1.name + "' and '" + attr2.name + "' do not have the same type (WHERE clause");
    }

    queryGraph.createEdge(attr1, attr2, tableAttr1, tableAttr2);
  }
}
