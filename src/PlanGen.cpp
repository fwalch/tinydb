#include "PlanGen.hpp"
#include <sstream>

using namespace std;

void PlanGen::loadTables() {
  for (Parser::Relation& relation : result.relations) {
    string name = relation.getName();
    tables[name] = &database.getTable(relation.relation);
    tablescans[name] = unique_ptr<Tablescan>(new Tablescan(*tables[name]));
  }
}

void PlanGen::loadProjections() {
  for (Parser::Attribute& attribute : result.projections) {
    if (attribute.name == Parser::Attribute::STAR) {
      if (attribute.relation == "") {
        // SELECT *
        for (auto tableIt : tables) {
          Table* table = tableIt.second;
          for (unsigned i = 0; i < table->getAttributeCount(); i++) {
            string attrName = table->getAttribute(i).getName();
            const Register* attrRegister = tablescans[tableIt.first]->getOutput(attrName);
            registers[attribute.getName()] = attrRegister;
            projection.push_back(attrRegister);
          }
        }
      }
      else {
        // SELECT relation.*
        Table* table = tables[attribute.relation];
        for (unsigned i = 0; i < table->getAttributeCount(); i++) {
          string attrName = table->getAttribute(i).getName();
          const Register* attrRegister = tablescans[attribute.relation]->getOutput(attrName);
          registers[attribute.getName()] = attrRegister;
          projection.push_back(attrRegister);
        }
      }
    }
    else {
      if (attribute.relation == "") {
        // SELECT field
        throw "Not implemented";
      }
      else {
        // SELECT relation.field
        int attrIndex = tables[attribute.relation]->findAttribute(attribute.name);
        string attrName = attribute.getName();
        attributes[attrName] = &tables[attribute.relation]->getAttribute(attrIndex);
        registers[attrName] = tablescans[attribute.relation]->getOutput(attribute.name);
        projection.push_back(registers[attrName]);
      }
    }
  }
}

void PlanGen::loadSelections() {
  for (auto join : result.joinConditions) {
    registers[join.first.getName()] = tablescans.at(join.first.relation)->getOutput(join.first.name);
    registers[join.second.getName()] = tablescans.at(join.second.relation)->getOutput(join.second.name);
    selections.push_back(make_pair(
      registers.at(join.first.getName()),
      registers.at(join.second.getName())));
  }

  for (auto selection : result.selections) {
    registers[selection.first.getName()] = tablescans.at(selection.first.relation)->getOutput(selection.first.name);
    Register c;
    switch (selection.second.type) {
      case Attribute::Type::Bool:
        c.setBool(selection.second.value == "true");
        break;

      case Attribute::Type::Int:
        int intValue;
        istringstream(selection.second.value) >> intValue;
        c.setInt(intValue);
        break;

      case Attribute::Type::String:
        c.setString(selection.second.value);
        break;

      case Attribute::Type::Double:
        double doubleValue;
        istringstream(selection.second.value) >> doubleValue;
        c.setDouble(doubleValue);
        break;
    }
    int index = constants.size();
    constants.push_back(c);
    selections.push_back(make_pair(
      registers.at(selection.first.getName()),
      &constants.at(index)));
  }
}

unique_ptr<Operator> PlanGen::generate() {
  loadTables();
  loadProjections();
  loadSelections();

  unique_ptr<Operator> op = move(tablescans.begin()->second);

  // Cross-product over all tables
  for (auto tsIt = ++tablescans.begin(); tsIt != tablescans.end(); tsIt++) {
    op = unique_ptr<CrossProduct>(new CrossProduct(move(op), move(tsIt->second)));
  }

  // Add selections
  for (auto selIt : selections) {
    unique_ptr<Chi> chi(new Chi(move(op), Chi::Equal, selIt.first, selIt.second));
    const Register* chiResult = chi->getResult();
    op = unique_ptr<Selection>(new Selection(move(chi), chiResult));
  }

  // Apply final projection and print output
  op = unique_ptr<Projection>(new Projection(move(op), projection));
  return unique_ptr<Printer>(new Printer(move(op)));
}
