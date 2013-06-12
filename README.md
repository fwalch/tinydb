# tinydb

Query Optimization Course

## Quickstart

 1. Run `make` to compile the source code.
 2. Run `make db` to create a sample database inside the `data` directory.

## Query execution

The query execution logic can be found in `src`; test it using e.g.

    bin/query data/uni "SELECT v.titel
      FROM studenten s, hoeren h, vorlesungen v
      WHERE s.matrnr=h.matrnr
      AND h.vorlnr=v.vorlnr
      AND s.name='Carnap'"

This should give the following output:

    Ethik
    Wissenschaftstheorie
    Bioethik
    Der Wiener Kreis

## Query graph & join tree

Basically, query graph and join tree will be generated and shown when using `EXPLAIN` instead of `SELECT` in SQL queries.

To generate an image file, the `circo` and `dot` executables provided by graphviz are needed. Also ImageMagick's `display` command is required.

Test it using e.g.

    bin/query data/uni "EXPLAIN * FROM studenten s, hoeren h, vorlesungen v WHERE s.matrnr=h.matrnr AND v.vorlnr=h.vorlnr"

# Exercises

## Exercise 1

The source code is located in `exercises/ex01`. Execute the code using `bin/ex01-query1` and
`bin/ex01-query2`.

## Exercise 5

  1. Execute `make tpch-db` to fetch TPC-H data (7zip is required) and create a database.
  2. Run `bin/ex05-query.sh` to execute the exercise query. It runs the query using `EXPLAIN`, so query graph and join tree will be shown.

## Exercise 7

To generate Dyck words, execute e.g. `bin/dyck 7 56`. This will generate the Dyck word from the exercise session 7 example, "(((())())(()))".
