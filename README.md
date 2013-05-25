# tinydb

Query Optimization Course

## Quickstart

 1. Run `make` to compile the source code.
 2. Run `make db` to create a sample database inside the `data` directory.

## Exercise 1

The source code is located in `exercises/ex01`. Execute the code using `bin/ex01-query1` and
`bin/ex01-query2`.

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

## Query graph

To generate an image file, the `circo` executable provided by graphviz is needed. When using the helper script `bin/graph.sh`, ImageMagick's `display` command is utilized.

Test it using e.g.

    bin/query data/uni "EXPLAIN * FROM studenten s, hoeren h, vorlesungen v WHERE s.matrnr=h.matrnr AND v.vorlnr=h.vorlnr" | circo -Tpng -oquery.png

or use the provided helper script

    bin/graph.sh data/uni "EXPLAIN * FROM studenten s, hoeren h, vorlesungen v WHERE s.matrnr=h.matrnr AND v.vorlnr=h.vorlnr"

