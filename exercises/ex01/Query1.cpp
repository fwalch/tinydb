#include "Database.hpp"
#include "operator/Tablescan.hpp"
#include "operator/HashJoin.hpp"
#include "operator/Printer.hpp"
#include "operator/Selection.hpp"
#include "operator/Projection.hpp"
#include "operator/Chi.hpp"
#include <iostream>

using namespace std;

int main()
{
  cout << "# Problem" << endl;
  cout << "Find all students that attended the lectures together with Schopenhauer, excluding Schopenhauer himself." << endl;

  cout << "# SQL" << endl;
  cout << "SELECT s1.name" << endl;
  cout << "FROM studenten s1, hoeren h1," << endl;
  cout << "     studenten s2, hoeren h2" << endl;
  cout << "WHERE s1.name = 'Schopenhauer'" << endl;
  cout << "  AND s1.matrnr <> s2.matrnr" << endl;
  cout << "  AND s1.matrnr = h1.matrnr" << endl;
  cout << "  AND h1.vorlnr = h2.vorlnr" << endl;
  cout << "  AND h2.matrnr = s2.matrnr" << endl;

  Database db;
  db.open("data/uni");

  Table& s1 = db.getTable("studenten");
  Table& s2 = db.getTable("studenten");
  Table& h1 = db.getTable("hoeren");
  Table& h2 = db.getTable("hoeren");

  unique_ptr<Tablescan> s1_scan(new Tablescan(s1));
  unique_ptr<Tablescan> s2_scan(new Tablescan(s2));
  unique_ptr<Tablescan> h1_scan(new Tablescan(h1));
  unique_ptr<Tablescan> h2_scan(new Tablescan(h2));

  const Register* s1_name = s1_scan->getOutput("name");
  const Register* s1_matrnr = s1_scan->getOutput("matrnr");
  const Register* s2_name = s2_scan->getOutput("name");
  const Register* s2_matrnr = s2_scan->getOutput("matrnr");
  const Register* h1_vorlnr = h1_scan->getOutput("vorlnr");
  const Register* h1_matrnr = h1_scan->getOutput("matrnr");
  const Register* h2_vorlnr = h2_scan->getOutput("vorlnr");
  const Register* h2_matrnr = h2_scan->getOutput("matrnr");

  // Find Schopenhauer
  Register schopenhauer; schopenhauer.setString("Schopenhauer");
  unique_ptr<Chi> s1_chi(new Chi(move(s1_scan), Chi::Equal, s1_name, &schopenhauer));
  const Register* s1_chiResult = s1_chi->getResult();
  unique_ptr<Selection> s1_select(new Selection(move(s1_chi), s1_chiResult));

  // Join with Schopenhauer's lectures
  unique_ptr<HashJoin> s1_h1_join(new HashJoin(move(s1_select), move(h1_scan), s1_matrnr, h1_matrnr));

  // Join with other student's lectures
  unique_ptr<HashJoin> h1_h2_join(new HashJoin(move(s1_h1_join), move(h2_scan), h1_vorlnr, h2_vorlnr));

  // Join with students that are not Schopenhauer
  unique_ptr<Chi> s2_chi(new Chi(move(s2_scan), Chi::NotEqual, s1_matrnr, s2_matrnr));
  const Register* s2_chiResult = s2_chi->getResult();
  unique_ptr<Selection> s2_select(new Selection(move(s2_chi), s2_chiResult));

  unique_ptr<HashJoin> h2_s2_join(new HashJoin(move(h1_h2_join), move(s2_select), h2_matrnr, s2_matrnr));

  // Print the output
  unique_ptr<Projection> s2_project(new Projection(move(h2_s2_join), {s2_name}));
  Printer out(move(s2_project));

  out.open();
  while (out.next());
  out.close();
}
