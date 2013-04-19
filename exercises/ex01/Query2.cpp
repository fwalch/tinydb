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
  cout << "# Problem" << endl
       << "Find all professors whose lectures attended at least two students." << endl;

  cout << "# SQL" << endl
       << "SELECT p.name" << endl
       << "FROM professoren p, vorlesungen v, hoeren h" << endl
       << "WHERE p.persnr = v.gelesenvon" << endl
       << "  AND v.vorlnr = h.vorlnr" << endl
       << "GROUP BY p.name" << endl
       << "HAVING COUNT(p.name) > 2" << endl;

  cout << "# Query output" << endl;

  Database db;
  db.open("data/uni");

  Table& p = db.getTable("professoren");
  Table& v = db.getTable("vorlesungen");
  Table& h = db.getTable("hoeren");

  unique_ptr<Tablescan> p_scan(new Tablescan(p));
  unique_ptr<Tablescan> v_scan(new Tablescan(v));
  unique_ptr<Tablescan> h_scan(new Tablescan(h));

  const Register* p_name = p_scan->getOutput("name");
  const Register* p_persnr = p_scan->getOutput("persnr");
  const Register* v_gelesenvon = v_scan->getOutput("gelesenvon");
  const Register* v_vorlnr = v_scan->getOutput("vorlnr");
  const Register* h_vorlnr = h_scan->getOutput("vorlnr");
  Register two; two.setInt(2);
}
