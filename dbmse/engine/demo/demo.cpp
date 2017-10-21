// Toy DBMS, v0.4
// George Chernishev (c) 2016-2017, chernishev<at>google mail
// A task framework for undergraduate students at Saint-Petersburg Academic University, DBMS development course
// More details regarding the course can be found here: www.math.spbu.ru/user/chernishev/
// CHANGELOG:
// 0.4: no chance for efficiency competition, so, this year I reoriented task towards ideas:
//      1) tried to remove C code, now we are using lots of std:: stuff
//      2) implemented support for multiple attributes in the DBMS
//      3) code clean-up and restructurization
// 0.3: added:
//      1) support for restricting physical join node size
//      2) support for deduplication node, LUniqueNode
//      3) print methods for Predicate and BaseTable
//      updated:
//      1) new format for data files: third line is the sort status now
//      2) added projection code
//      3) contract contains print methods for physical and logical nodes
// 0.2: first public release

#include <stdio.h>
#include <typeinfo>
#include <iostream>
#include <memory>
#include <utility>
#include <vector>
#include <tuple>
#include "../interface/interface.h"
#include "../interface/basics.h"
#include "pselectnode.h"
#include "pjoinnode.h"

// Here be rewriter and optimizer
std::unique_ptr<PResultNode> QueryFactory(LAbstractNode* node) {
  // As of now, we handle only SELECTs with 0 predicates
  // Implementing conjunctive predicates is your homework
  if (dynamic_cast<LSelectNode*>(node) != nullptr) {
    LSelectNode* tmp = (LSelectNode*)node;
    std::vector<Predicate> p;
    return std::unique_ptr<PResultNode>(new PSelectNode(tmp, p));
  } else
    // Also, only one join is possible
    // Supporting more joins is also your (future) homework
    if (dynamic_cast<LJoinNode*>(node) != nullptr) {

      LSelectNode* tmp = (LSelectNode*)(node->GetRight());
      std::vector<Predicate> p;
      std::unique_ptr<PSelectNode> rres(new PSelectNode(tmp, p));

      LSelectNode* tmp2 = (LSelectNode*)(node->GetLeft());
      std::unique_ptr<PSelectNode> lres(new PSelectNode(tmp2, p));

      return std::unique_ptr<PResultNode>(std::unique_ptr<PJoinNode>(new PJoinNode(std::move(lres), std::move(rres), node)));
    } else
      return nullptr;

}

void ExecuteQuery(PResultNode* query) {
  std::tuple<ErrCode, std::vector<Value>> res;
  res = query->GetRecord();
  ErrCode ec = std::get<0>(res);
  std::vector<Value> vals = std::get<1>(res);
  while (ec == EC_OK) {
    for (int i = 0; i < query->GetAttrNum(); i++) {
      if (vals[i].vtype == VT_INT)
        std::cout << vals[i].vint << " ";
      else if (vals[i].vtype == VT_STRING)
        std::cout << vals[i].vstr << " ";
    }
    printf("\n");
    res = query->GetRecord();
    ec = std::get<0>(res);
    vals = std::get<1>(res);
  }

}

int main() {
  {
    std::cout << "Starting demo" << std::endl;
    std::cout << "Query1: plain select" << std::endl;
    BaseTable bt1 = BaseTable("table1");
    std::cout << bt1;
    std::unique_ptr<LAbstractNode> n1(new LSelectNode(bt1, {}));
    std::unique_ptr<PResultNode> q1 = QueryFactory(n1.get());
    q1->Print(0);
    ExecuteQuery(q1.get());
  }

  {
    std::cout << std::endl << "Query2: simple equi-join" << std::endl;
    BaseTable bt1 = BaseTable("table1");
    BaseTable bt2 = BaseTable("table2");
    std::cout << bt1;
    std::cout << bt2;
    std::unique_ptr<LAbstractNode> n1(new LSelectNode(bt1, {}));
    std::unique_ptr<LAbstractNode> n2(new LSelectNode(bt2, {}));
    std::unique_ptr<LJoinNode> n3(new LJoinNode(std::move(n1), std::move(n2), "table1.id", "table2.id2", 666));
    std::unique_ptr<PResultNode> q1 = QueryFactory(n3.get());
    q1->Print(0);
    ExecuteQuery(q1.get());
  }

}
