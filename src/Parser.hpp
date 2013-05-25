#ifndef H_Parser
#define H_Parser

#include <string>
#include <vector>
#include "Attribute.hpp"
#include "Exception.hpp"

class Parser {
  public:
    struct Result;

  private:
    std::string query;

  public:
    Parser(const std::string query) : query(query) { }
    void parse(Result& result);

  struct Constant {
    ::Attribute::Type type;
    std::string value;
  };

  struct Attribute {
    static const std::string STAR;
    std::string relation;
    std::string name;
    inline std::string getName() {
      return relation + "." + name;
    }
  };

  struct Relation {
    std::string relation;
    std::string binding;
    inline std::string getName() {
      return binding == ""
        ? relation
        : binding;
    }
  };

  struct Result {
    std::vector<Relation> relations;
    std::vector<Attribute> projections;
    std::vector<std::pair<Attribute, Constant>> selections;
    std::vector<std::pair<Attribute, Attribute>> joinConditions;
    bool explain = false;
  };
  class SyntacticError : public Exception {
    public:
      SyntacticError(const std::string message) : Exception(message) {
      }
  };
};

#endif
