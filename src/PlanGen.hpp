#ifndef H_PlanGen
#define H_PlanGen

#include "Database.hpp"
#include "Table.hpp"
#include "Parser.hpp"
#include "Exception.hpp"
#include "operator/Operator.hpp"
#include "operator/Tablescan.hpp"
#include "operator/Projection.hpp"
#include "operator/Printer.hpp"
#include "operator/Selection.hpp"
#include "operator/CrossProduct.hpp"
#include "operator/Chi.hpp"
#include "Register.hpp"
#include <unordered_map>
#include <vector>
#include <memory>

class PlanGen {
  private:
    Parser::Result& result;
    Database& database;
    std::unordered_map<std::string, Table*> tables;
    std::unordered_map<std::string, std::unique_ptr<Tablescan>> tablescans;
    std::unordered_map<std::string, const Attribute*> attributes;
    std::unordered_map<std::string, const Register*> registers;
    std::vector<const Register*> projection;
    std::vector<std::tuple<std::string, const Register*, const Register*>> selections;
    std::vector<std::pair<const Register*, const Register*>> joins;
    std::vector<Register> constants;

    void loadTables();
    void loadProjections();
    void loadSelections();
    void loadJoins();
    std::unique_ptr<Operator> addSelections(std::string relation, std::unique_ptr<Operator> op, size_t& processedSelections);

  public:
    PlanGen(Database& database, Parser::Result& result)
      : result(result), database(database) { }
    std::unique_ptr<Operator> generate();

  class GenError : public Exception {
    public:
      GenError(const std::string message) : Exception(message) {
      }
  };
};

#endif
