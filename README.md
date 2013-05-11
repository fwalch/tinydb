# tinydb

Query Optimization Course

## Quickstart

 1. Run `make` to compile the source code.
 2. Run `make db` to create a sample database inside the `data` directory.

## Exercise 1

The source code is located in `exercises/ex01`. Execute the code using `bin/ex01-query1` and
`bin/ex01-query2`.

The second part of the exercise can be found in `src`; test it using e.g.

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
