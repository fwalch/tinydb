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
       << "FROM professoren p, vorlesungen v, hoeren h1, hoeren h2" << endl
       << "WHERE p.persnr = v.gelesenvon" << endl
       << "  AND v.vorlnr = h1.vorlnr" << endl
       << "  AND v.vorlnr = h2.vorlnr" << endl
       << "  AND h1.matrnr <> h2.matrnr" << endl;

  cout << "# Query output" << endl;

  Database db;
  db.open("data/uni");

  Table& p = db.getTable("professoren");
  Table& v = db.getTable("vorlesungen");
  Table& h1 = db.getTable("hoeren");
  Table& h2 = db.getTable("hoeren");

  unique_ptr<Tablescan> p_scan(new Tablescan(p));
  unique_ptr<Tablescan> v_scan(new Tablescan(v));
  unique_ptr<Tablescan> h2_scan(new Tablescan(h1));
  unique_ptr<Tablescan> h1_scan(new Tablescan(h2));

  const Register* p_name = p_scan->getOutput("name");
  const Register* p_persnr = p_scan->getOutput("persnr");
  const Register* v_gelesenvon = v_scan->getOutput("gelesenvon");
  const Register* v_vorlnr = v_scan->getOutput("vorlnr");
  const Register* h1_vorlnr = h1_scan->getOutput("vorlnr");
  const Register* h2_vorlnr = h2_scan->getOutput("vorlnr");
  const Register* h1_matrnr = h1_scan->getOutput("matrnr");
  const Register* h2_matrnr = h2_scan->getOutput("matrnr");

  unique_ptr<HashJoin> p_v_join(new HashJoin(move(p_scan), move(v_scan), p_persnr, v_gelesenvon));
  unique_ptr<HashJoin> v_h1_join(new HashJoin(move(p_v_join), move(h1_scan), v_vorlnr, h1_vorlnr));
  unique_ptr<HashJoin> v_h2_join(new HashJoin(move(v_h1_join), move(h2_scan), v_vorlnr, h2_vorlnr));
  unique_ptr<Chi> h_chi(new Chi(move(v_h2_join), Chi::NotEqual, h1_matrnr, h2_matrnr));
  const Register* h_chi_result = h_chi->getResult();
  unique_ptr<Selection> h_select(new Selection(move(h_chi), h_chi_result));
  unique_ptr<Projection> project(new Projection(move(h_select), {p_name}));

  Printer out(move(project));

  out.open();
  while (out.next());
  out.close();
}
