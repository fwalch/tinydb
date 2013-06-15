#include "PlanGen.hpp"
#include <sstream>
#include <algorithm>
#include <vector>
#include <iostream>
#include <cassert>
#include <cmath>

using namespace std;

std::ostream& operator<<(std::ostream &stream, const set<string>& set);
std::ostream& operator<<(std::ostream &stream, const set<string>& set) {
  for (auto str : set) {
    stream << str << " ";
  }
  return stream;
}

set<string> merge(set<string> left, set<string> right) {
  set<string> s(left);
  for (string str : right) {
    s.insert(str);
  }
  return s;
}

void PlanGen::loadJoinRegisters() {
  for (auto join : result.joinConditions) {
    registers[join.first.getName()] = tablescans.at(join.first.relation)->getOutput(join.first.name);
    registers[join.second.getName()] = tablescans.at(join.second.relation)->getOutput(join.second.name);
  }
}

void PlanGen::loadTables() {
  for (Parser::Relation& relation : result.relations) {
    string name = relation.getName();
    tables[name] = &database.getTable(relation.relation);
    tablescans[name] = unique_ptr<Tablescan>(new Tablescan(*tables[name]));
  }
}

//TODO: this is ugly
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
        for (auto tableIt : tables) {
          Table* table = tableIt.second;
          for (unsigned i = 0; i < table->getAttributeCount(); i++) {
            string attrName = table->getAttribute(i).getName();
            if (attribute.name == attrName) {
              const Register* attrRegister = tablescans[tableIt.first]->getOutput(attrName);
              registers[attribute.getName()] = attrRegister;
              projection.push_back(attrRegister);
              goto next;
            }
          }
        }
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
next:;
  }
}

void PlanGen::loadSelections() {
  for (auto selection : result.selections) {
    registers[selection.first.getName()] = tablescans.at(selection.first.relation)->getOutput(selection.first.name);
    Register c;
    switch (selection.second.type) {
      case Attribute::Type::Bool: {
        string lowerWord;
        lowerWord.resize(selection.second.value.size());
        transform(selection.second.value.begin(), selection.second.value.end(), lowerWord.begin(), ::tolower);
        c.setBool(lowerWord == "true");
        break;
      }

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
    selections.push_back(make_tuple(
      selection.first.relation,
      registers.at(selection.first.getName()),
      &constants.at(index)));
  }
}

unique_ptr<Operator> PlanGen::addSelections(string relation, unique_ptr<Operator> op) {
  vector<SelectionsType::iterator> toErase;
  // If there are appropriate selections, add them now
  for (auto selIt = selections.begin(); selIt != selections.end(); selIt++) {
    if (get<0>(*selIt) == relation) {
      op = unique_ptr<Selection>(new Selection(move(op), get<1>(*selIt), get<2>(*selIt)));
      toErase.push_back(selIt);
    }
  }

  for (auto sel : toErase) {
    selections.erase(sel);
  }
  return op;
}

bool PlanGen::canJoin(std::set<std::string> left, std::set<std::string> right, std::vector<QueryGraph::Edge*> edges, float& cost, QueryGraph::Edge** edge) {
  for (auto it = edges.begin(); it != edges.end(); it++) {
    string rel1 = (*it)->node1->relation;
    string rel2 = (*it)->node2->relation;

    bool leftHas1 = left.find(rel1) != left.end();
    bool leftHas2 = left.find(rel2) != left.end();

    bool rightHas1 = right.find(rel1) != right.end();
    bool rightHas2 = right.find(rel2) != right.end();

    if (((leftHas1 && rightHas2) || (leftHas2 && rightHas1))
        && !((leftHas1 && rightHas1) || (leftHas2 && rightHas2))) {
      *edge = *it;
      edges.erase(it);
      cost = getCost(left, right, *edge);
      return true;
    }
  }
  return false;
}

float PlanGen::lookupCost(set<string> relations) {
  auto it = dpLookup.find(relations);
  if (it != dpLookup.end()) {
    return it->second;
  }
  return numeric_limits<float>::infinity();
}

float PlanGen::getCost(set<string> left, set<string> right, QueryGraph::Edge* edge) {
  return dpLookup[left] + dpLookup[right] + dpLookup[left]*dpLookup[right]*edge->selectivity;
}

unique_ptr<Operator> PlanGen::generate(ostream& explain) {
  loadTables();
  loadProjections();
  loadSelections();
  loadJoinRegisters();

  vector<QueryGraph::Edge*> edges(queryGraph.edges);

  dpTable.resize(queryGraph.nodes.size());

  // First, add all relations into map
  for (auto nodeIt : queryGraph.nodes) {
    string relation = nodeIt.first;

    unique_ptr<Operator> op = move(tablescans.at(relation));
    op = move(addSelections(relation, move(op)));
    unsigned cardinality = tables.at(relation)->getCardinality();

    set<string> set { relation };

    // Prepare dpTable for DPSub
    dpTable[0].insert(make_pair(set, op.release()));
    dpLookup.insert(make_pair(set, cardinality));
    dpDebug.insert(make_pair(set, relation));
  }

  // Execute DPsub
  for (size_t i = 1; i < dpTable.size(); i++) {
    for (size_t j = 0; j < i; j++) {
      DpTableType::value_type& leftMap = dpTable[i-1];
      for (auto leftIt = leftMap.begin(); leftIt != leftMap.end(); leftIt++) {
        set<string> left = leftIt->first;

        DpTableType::value_type& rightMap = dpTable[i-j-1];
        for (auto rightIt = rightMap.begin(); rightIt != rightMap.end(); rightIt++) {
          set<string> right = rightIt->first;
          set<string> joined = merge(left, right);

          float cost;
          QueryGraph::Edge* edge = nullptr;

          if (canJoin(left, right, edges, cost, &edge) && lookupCost(joined) > cost) {
            DpTableType::value_type& dpMap = dpTable[i];
            dpDebug.insert(make_pair(joined, "(" + dpDebug[left] + ") join (" + dpDebug[right] + ")"));

            auto it = dpMap.find(joined);
            if (it != dpMap.end()) {
              HashJoin* join = reinterpret_cast<HashJoin*>(it->second);
              join->left.release();
              join->right.release();
              dpMap.erase(it);
            }

            auto leftOp = unique_ptr<Operator>(leftIt->second);
            auto rightOp = unique_ptr<Operator>(rightIt->second);
            auto leftValue = registers.at(edge->fullPredicate1);
            auto rightValue = registers.at(edge->fullPredicate2);

            HashJoin* hashJoin = new HashJoin(move(leftOp), move(rightOp), leftValue, rightValue);
            dpMap.insert(make_pair(joined, hashJoin));
            dpLookup[joined] = cost;
          }
        }
      }
    }
  }

  for (DpTableType::value_type& map : dpTable) {
    for (DpTableType::value_type::value_type& pair : map) {
      // Print DP table of smaller size (already finished)
      explain << pair.first << "| " << dpDebug[pair.first] << endl;
    }
  }

  if (selections.size() > 0) {
    throw GenError("Internal error: could not push down all selections.");
  }

  DpTableType::value_type& finalDpEntry = dpTable[dpTable.size()-1];
  if (finalDpEntry.size() != 1) {
    throw GenError("Could not execute DPSub; only connected relations are supported (no cross-products)");
  }

  unique_ptr<Operator> op = unique_ptr<Operator>(finalDpEntry.begin()->second);

  // Apply final projection and print output
  op = unique_ptr<Projection>(new Projection(move(op), projection));
  return unique_ptr<Printer>(new Printer(move(op)));
}
