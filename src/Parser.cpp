#include <unordered_set>
#include <algorithm>
#include <cstring>
#include <cctype>
#include "Parser.hpp"

using namespace std;

void select(const char** query, Parser::Result& result);
void from(const char** query, Parser::Result& result);
void where(const char** query, Parser::Result& result);
bool constant(const char** query, Parser::Constant& constant);
void attribute(const char** query, Parser::Attribute& attribute);
void relation(const char** query, Parser::Relation& relation);

void Parser::parse(Result& result) {
  const char* queryPtr = query.c_str();

  select(&queryPtr, result);

  if (queryPtr != query.c_str() + query.length()) {
    throw SyntacticError("Could not parse trailing query text: '" + string(queryPtr) + "'");
  }
}

const string Parser::Attribute::STAR = "*";
const char STAR = '*';
const string SELECT = "select";
const string EXPLAIN = "explain";
const string FROM = "from";
const string WHERE = "where";
const string AND = "and";
const string TRUE = "true";
const string FALSE = "false";
const char DOT = '.';
const char COMMA = ',';
const char WHITESPACE = ' ';
const char EOS = '\0';
const char APOSTROPHE = '\'';
const char EQUAL = '=';

inline bool isWhitespace(char character) {
  return character == WHITESPACE;
}

inline bool isEndOfWord(char character) {
  return isWhitespace(character)
    || character == EQUAL
    || character == COMMA
    || character == DOT;
}

inline bool isEndOfString(char character) {
  return character == EOS;
}

inline void skipLeadingWhitespace(const char** text) {
  while (isWhitespace(**text) && !isEndOfString(**text)) (*text)++;
}

inline void readUntilEndOfWord(const char** text) {
  while (!isEndOfWord(**text) && !isEndOfString(**text)) (*text)++;
}

string peekWord(const char* text) {
  const char* wordStart = text;

  skipLeadingWhitespace(&wordStart);
  text = wordStart;
  readUntilEndOfWord(&text);

  return string(wordStart, text - wordStart);
}

char peek(const char* text) {
  skipLeadingWhitespace(&text);
  return *text;
}

string readWord(const char** text) {
  skipLeadingWhitespace(text);

  string word = peekWord(*text);
  *text += word.size();
  return word;
}

char read(const char** text) {
  skipLeadingWhitespace(text);

  char c = peek(*text);
  if (!isEndOfString(c)) *text += 1;
  return c;
}

bool assume(string assumed, const char* actual) {
  string word = peekWord(actual);

  string lowerWord;
  lowerWord.resize(word.size());
  std::transform(word.begin(), word.end(), lowerWord.begin(), ::tolower);

  return lowerWord == assumed;
}

void expect(string expected, const char** actual, string context) {
  string word = readWord(actual);

  string lowerWord;
  lowerWord.resize(word.size());
  std::transform(word.begin(), word.end(), lowerWord.begin(), ::tolower);

  if (lowerWord != expected) {
    if (isEndOfString(peek(*actual))) {
      throw Parser::SyntacticError("Expected " + expected + " in " + context);
    }
    throw Parser::SyntacticError("Expected " + expected + ", but got " + string(*actual) + " in " + context);
  }
}

void expect(char expected, const char** actual, string context) {
  if (tolower(read(actual)) != expected) {
    if (isEndOfString(peek(*actual))) {
      throw Parser::SyntacticError("Expected " + string(&expected, 1) + " in " + context);
    }
    throw Parser::SyntacticError("Expected " + string(&expected, 1) + ", but got " + string(*actual) + " in " + context);
  }
}

inline bool isString(const char* query) {
  return peek(query) == APOSTROPHE;
}

inline bool isBoolean(const char* query) {
  string word = peekWord(query);
  return word == TRUE || word == FALSE;
}

inline bool isDouble(const char* query) {
  string word = peekWord(query);
  for (char c : word) {
    if (isdigit(c)) continue;
    if (c == DOT) return true;
    return false;
  }
  return false;
}

inline bool isInteger(const char* query) {
  return isdigit(peek(query));
}

inline bool isConstant(const char* query) {
  return isString(query)
      || isBoolean(query)
      || isDouble(query)
      || isInteger(query);
}

bool constant(const char** query, Parser::Constant& constant) {
  ::Attribute::Type type;
  if (isString(*query)) {
    type = ::Attribute::Type::String;
    expect(APOSTROPHE, query, "string constant");
    string word = readWord(query);
    (*query)--;
    expect(APOSTROPHE, query, "string constant");
    constant = {
      type,
      string(word.c_str(), word.length() - 1)
    };
    return true;
  }
  else if (isBoolean(*query)) {
    type = ::Attribute::Type::Bool;
  }
  else if (isDouble(*query)) {
    type = ::Attribute::Type::Double;
  }
  else if (isInteger(*query)) {
    type = ::Attribute::Type::Int;
  }
  else {
    return false;
  }

  constant = {
    type,
    readWord(query)
  };

  return true;
}

void attribute(const char** query, Parser::Attribute& attribute) {
  string word1 = readWord(query);
  if (peek(*query) == DOT) {
    expect(DOT, query, "attribute parsing");
    string word2 = readWord(query);
    attribute = {
      word1,
      word2
    };
  }
  else {
    attribute = {
      "",
      word1
    };
  }
}

void relation(const char** query, Parser::Relation& relation) {
  string word1 = readWord(query);
  char nextChar = peek(*query);
  if (nextChar != COMMA && !isEndOfString(nextChar) && !assume(WHERE, *query)) {
    if (nextChar == STAR) {
      // SELECT relation.*
      expect(STAR, query, "SELECT clause");
      relation = {
        word1,
        "*"
      };
    }
    else {
      // SELECT relation.field
      string word2 = readWord(query);
      relation = {
        word1,
        word2
      };
    }
  }
  else {
    // SELECT field
    relation = {
      word1,
      ""
    };
  }
}

void select(const char** query, Parser::Result& result) {
  if (assume(EXPLAIN, *query)) {
    expect(EXPLAIN, query, "SELECT clause");
    result.explain = true;
  }
  else {
    expect(SELECT, query, "SELECT clause");
  }

  if (peek(*query) == STAR) {
    // SELECT *
    expect(STAR, query, "SELECT clause");
    Parser::Attribute attr = {
      "",
      Parser::Attribute::STAR
    };
    result.projections.push_back(attr);
  }
  else {
    while (!assume(FROM, *query)) {
      Parser::Attribute attr;
      attribute(query, attr);
      result.projections.push_back(attr);
      if (!assume(FROM, *query)) expect(COMMA, query, "SELECT clause");
    }
  }

  from(query, result);
}

void from(const char** query, Parser::Result& result) {
  expect(FROM, query, "FROM clause");

  while (!assume(WHERE, *query)) {
    Parser::Relation rel;
    relation(query, rel);
    result.relations.push_back(rel);

    if (isEndOfString(peek(*query))) return;
    if (!assume(WHERE, *query)) expect(COMMA, query, "FROM clause");
  }
  where(query, result);
}

void where(const char** query, Parser::Result& result) {
  expect(WHERE, query, "WHERE clause");

  if (isEndOfString(peek(*query))) {
    throw Parser::SyntacticError("Expected WHERE clause");
  }

  do {
    Parser::Attribute attr1;
    attribute(query, attr1);

    expect(EQUAL, query, "WHERE clause");

    if (isConstant(*query)) {
      Parser::Constant c;
      constant(query, c);
      result.selections.push_back(make_pair(attr1, c));
    }
    else {
      Parser::Attribute attr2;
      attribute(query, attr2);
      result.joinConditions.push_back(make_pair(attr1, attr2));
    }

    if (!isEndOfString(peek(*query))) expect(AND, query, "WHERE clause");
  }
  while (!isEndOfString(peek(*query)));
}

