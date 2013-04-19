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

void SemanticAnalyzer::analyzeAttribute(Parser::Attribute& attr, ::Attribute::Type& type, string context) {
  if (attr.name == Parser::Attribute::STAR) return;

  if (attr.relation == "") {
    // Search for attribute
    bool found = false;

    for (auto relIt : relations) {
      Table& table = database.getTable(relIt.second.relation);
      for (unsigned i = 0; i < table.getAttributeCount(); i++) {
        ::Attribute tableAttr = table.getAttribute(i);
        if (tableAttr.getName() == attr.name) {
          type = tableAttr.getType();
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

    type = table.getAttribute(attrIndex).getType();
  }
}

void SemanticAnalyzer::execute() {
  init();

  ::Attribute::Type projectionType;
  for (Parser::Attribute& attr : result.projections) {
    analyzeAttribute(attr, projectionType, "SELECT");
  }

  // Check that all selections are valid
  for (auto selection : result.selections) {
    Parser::Attribute& attr = selection.first;
    Parser::Constant& c = selection.second;

    ::Attribute::Type type;
    analyzeAttribute(attr, type, "WHERE");

    if (type != c.type) {
      throw SemanticError("Attribute '" + attr.name + "' and constant '" + c.value + "' do not have the same type (WHERE clause)");
    }
  }

  // Check that all joins are valid
  for (auto joinCondition : result.joinConditions) {
    Parser::Attribute& attr1 = joinCondition.first;
    Parser::Attribute& attr2 = joinCondition.second;

    ::Attribute::Type type1;
    ::Attribute::Type type2;
    analyzeAttribute(attr1, type1, "WHERE");
    analyzeAttribute(attr2, type2, "WHERE");

    if (type1 != type2) {
      throw SemanticError("Attributes '" + attr1.name + "' and '" + attr2.name + "' do not have the same type (WHERE clause");
    }
  }
}
