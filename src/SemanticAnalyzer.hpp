#ifndef H_SemanticAnalyzer
#define H_SemanticAnalyzer

#include "Exception.hpp"
#include "Database.hpp"
#include "Parser.hpp"
#include "QueryGraph.hpp"
#include <unordered_map>
#include <string>

class SemanticAnalyzer {
  private:
    Database& database;
    Parser::Result& result;
    std::unordered_map<std::string, Parser::Relation> relations;

    void init();
    void analyzeAttribute(Parser::Attribute& attr, std::string context, ::Attribute& tableAttr);

  public:
    SemanticAnalyzer(Database& database, Parser::Result& result)
      : database(database), result(result) { }

    void execute(QueryGraph& queryGraph);

    class SemanticError : public Exception {
      public:
        SemanticError(const std::string message) : Exception(message) {
        }
    };
};

#endif
