#include "PlanGen.hpp"
#include <sstream>
#include <algorithm>
#include <vector>
#include <iostream>
#include <cassert>

using namespace std;

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

PlanGen::WaitingJoinsType::iterator PlanGen::findWaitingJoin(std::string str) {
  for (auto joinIt = waitingJoins.begin(); joinIt != waitingJoins.end(); ++joinIt) {
    if (joinIt->first.find(str) != joinIt->first.end()) {
      return joinIt;
    }
  }
  return waitingJoins.end();
}

unique_ptr<Operator> PlanGen::generate(ostream& explain) {
  explain << "graph {" << endl;

  loadTables();
  loadProjections();
  loadSelections();
  loadJoinRegisters();

  vector<QueryGraph::Edge*> edges(queryGraph.edges);

  // First, add all relations into waiting map
  for (auto nodeIt : queryGraph.nodes) {
    std::string relation = nodeIt.first;

    unique_ptr<Operator> op = move(tablescans.at(relation));
    op = move(addSelections(relation, move(op)));
    unsigned cardinality = tables.at(relation)->getCardinality();

    explain << relation << ";" << endl;

    set<string> set { relation };

    waitingJoins[set] = make_tuple(relation, cardinality, move(op), 0);
  }

  // GOO on join edges
  unsigned joinId = 0;
  while (edges.size() > 0) {
    // select edge with lowest selectivity
    size_t edgeIndex = -1;
    double edgeCost = numeric_limits<double>::infinity();

    for (size_t i = 0; i < edges.size(); i++) {
      double value = edges[i]->selectivity*edges[i]->node1->cardinality*edges[i]->node2->cardinality;
      if (value < edgeCost) {
        edgeCost = value;
        edgeIndex = i;
      }
    }
    QueryGraph::Edge* edge = edges.at(edgeIndex);

    std::string& relation1 = edge->node1->relation;
    std::string& relation2 = edge->node2->relation;

    // Process join of given edge
    unique_ptr<Operator> left = nullptr;
    unique_ptr<Operator> right = nullptr;

    set<string> leftSet{relation1};
    set<string> rightSet{relation2};

    std::string leftId;
    std::string rightId;

    unsigned leftCardinality;
    unsigned rightCardinality;

    float leftCost;
    float rightCost;

    auto joinedRelation1 = findWaitingJoin(relation1);
    auto joinedRelation2 = findWaitingJoin(relation2);

    if (joinedRelation1 != waitingJoins.end()) {
      left = move(get<2>(joinedRelation1->second));
      leftSet = joinedRelation1->first;
      leftId = get<0>(joinedRelation1->second);
      leftCardinality = get<1>(joinedRelation1->second);
      leftCost = get<3>(joinedRelation1->second);
      waitingJoins.erase(joinedRelation1);
    }
    else {
      throw GenError("Internal error: relation not found");
    }
    auto leftValue = registers.at(edge->fullPredicate1);

    if (joinedRelation2 != waitingJoins.end()) {
      right = move(get<2>(joinedRelation2->second));
      rightSet = joinedRelation2->first;
      rightId = get<0>(joinedRelation2->second);
      rightCardinality = get<1>(joinedRelation2->second);
      rightCost = get<3>(joinedRelation2->second);
      waitingJoins.erase(joinedRelation2);
    }
    else {
      throw GenError("Internal error: relation not found");
    }
    auto rightValue = registers.at(edge->fullPredicate2);

    set<string> set(leftSet);
    for (auto str : rightSet) {
      set.insert(str);
    }

    float cost = leftCost + rightCost + leftCardinality*rightCardinality*edge->selectivity;
    auto joinIdStr = "J" + to_string(joinId);

    unique_ptr<HashJoin> join(new HashJoin(move(left), move(right), leftValue, rightValue));
    waitingJoins[set] = make_tuple(joinIdStr, cost, move(join), cost);

    cout << "Joining " << joinIdStr << " = " << leftId << " and " << rightId << " (total estimated cost up to this point: " << cost << ")" << endl;

    explain << joinIdStr << ";" << endl;
    explain << joinIdStr << " -- " << leftId << ";" << endl;
    explain << joinIdStr << " -- " << rightId << ";" << endl;

    edges.erase(edges.begin()+edgeIndex);
    joinId++;
  }

  if (selections.size() > 0) {
    throw GenError("Internal error: could not push down all selections.");
  }

  unique_ptr<Operator> op = nullptr;
  std::string previousId;

  // Cross-product remaining joins and relations
  for (auto joinIt = waitingJoins.begin(); joinIt != waitingJoins.end(); ++joinIt) {
    if (op == nullptr) {
      op = move(get<2>(joinIt->second));
      previousId = get<0>(joinIt->second);
      continue;
    }
    op = unique_ptr<CrossProduct>(new CrossProduct(move(op), move(get<2>(joinIt->second))));

    explain << previousId << " -- " << get<0>(joinIt->second) << ";" << endl;
    previousId = get<0>(joinIt->second);
  }

  explain << "}" << endl;

  // Apply final projection and print output
  op = unique_ptr<Projection>(new Projection(move(op), projection));
  return unique_ptr<Printer>(new Printer(move(op)));
}
