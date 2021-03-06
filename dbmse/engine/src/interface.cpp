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

#include <algorithm>
#include <tuple>
#include <utility>
#include <string.h>
#include "interface.h"


LAbstractNode::LAbstractNode(std::unique_ptr<LAbstractNode> left_, std::unique_ptr<LAbstractNode> right_)
  : left(std::move(left_)), right(std::move(right_)) {
}

LAbstractNode::~LAbstractNode() {
}

LAbstractNode* LAbstractNode::GetLeft() {
  return left.get();
}

LAbstractNode* LAbstractNode::GetRight() {
  return right.get();
}

LCrossProductNode::LCrossProductNode(std::unique_ptr<LAbstractNode> left_, std::unique_ptr<LAbstractNode> right_) 
  : LAbstractNode(std::move(left_), std::move(right_)) {
  fieldNames = left->fieldNames;
  fieldTypes = left->fieldTypes;
  fieldOrders = left->fieldOrders;
  for (int i = 0; i < right->fieldNames.size(); i++) {
    fieldNames.push_back(right->fieldNames[i]);
    fieldTypes.push_back(right->fieldTypes[i]);
    fieldOrders.push_back(CS_UNKNOWN);
  }
}

LJoinNode::LJoinNode(std::unique_ptr<LAbstractNode> left_, std::unique_ptr<LAbstractNode> right_,
                     std::string offset1, std::string offset2, int memorylimit, LJoinType type)
  : LAbstractNode(std::move(left_), std::move(right_)) {
  this->offset1 = offset1;
  this->offset2 = offset2;
  this->memorylimit = memorylimit;
  this->type = type;

  // TODO: disgusting, fix this
  std::vector<std::string> match;
  ValueType vt;
  COLUMN_SORT cs;
  for (int i = 0; i < left->fieldNames.size(); i++) {
    for (int j = 0; j < right->fieldNames.size(); j++) {
      std::vector<std::string> l = left->fieldNames[i];
      std::vector<std::string> r = right->fieldNames[j];

      if (std::find(l.begin(), l.end(), offset1) != l.end()) {
        if (std::find(r.begin(), r.end(), offset2) != r.end()) {
          match = l;
          match.insert(std::end(match), std::begin(r), std::end(r));
          vt = left->fieldTypes[i];
          cs = left->fieldOrders[i];
        }
      } else if (std::find(l.begin(), l.end(), offset2) != l.end()) {
        if (std::find(r.begin(), r.end(), offset1) != r.end()) {
          match = l;
          match.insert(std::end(match), std::begin(r), std::end(r));
          vt = left->fieldTypes[i];
          cs = left->fieldOrders[i];
        }
      }
    }
  }

  for (int i = 0; i < left->fieldNames.size(); i++) {
    std::vector<std::string> l = left->fieldNames[i];
    if (std::find(l.begin(), l.end(), offset1) == l.end())
      if (std::find(l.begin(), l.end(), offset2) == l.end()) {
        fieldNames.push_back(l);
        fieldTypes.push_back(left->fieldTypes[i]);
        fieldOrders.push_back(left->fieldOrders[i]);
      }
  }

  for (int i = 0; i < right->fieldNames.size(); i++) {
    std::vector<std::string> r = right->fieldNames[i];
    if (std::find(r.begin(), r.end(), offset1) == r.end())
      if (std::find(r.begin(), r.end(), offset2) == r.end()) {
        fieldNames.push_back(r);
        fieldTypes.push_back(right->fieldTypes[i]);
        fieldOrders.push_back(CS_UNKNOWN);

      }

  }
  fieldNames.push_back(match);
  fieldTypes.push_back(vt);
  fieldOrders.push_back(cs);

}

LUnionNode::LUnionNode(std::unique_ptr<LAbstractNode> left_, std::unique_ptr<LAbstractNode> right_) 
  : LAbstractNode(std::move(left_), std::move(right_)) {
  assert(left->fieldNames.size() == right->fieldNames.size());
  assert(left->fieldTypes == right->fieldTypes);

  fieldNames = left->fieldNames;
  fieldTypes = left->fieldTypes;
  fieldOrders = left->fieldOrders;
  fill(fieldOrders.begin(), fieldOrders.end(), CS_UNKNOWN);
  for (int i = 0; i < right->fieldNames.size(); i++) {
    fieldNames[i].insert(fieldNames[i].end(), right->fieldNames[i].begin(), right->fieldNames[i].end());
  }
}

LProjectNode::LProjectNode(std::unique_ptr<LAbstractNode> child_, std::vector<std::string> tokeep)
  : LAbstractNode(std::move(child_), nullptr) {
  for (int i = 0; i < left->fieldNames.size(); i++) {
    for (int j = 0; j < tokeep.size(); j++) {
      std::vector<std::string> source = left->fieldNames[i];
      std::string candidate = tokeep[j];
      if (std::find(source.begin(), source.end(), candidate) != source.end()) {
        fieldNames.push_back(source);
        fieldTypes.push_back(left->fieldTypes[i]);
        fieldOrders.push_back(left->fieldOrders[i]);
        offsets.push_back(source[0]);
        continue;
      }
    }
  }
}

LSelectNode::LSelectNode(BaseTable& table,
                         const Predicate *predicate): LAbstractNode(nullptr, nullptr) {
  this->table = table;
  this->predicate = predicate;
  for (int i = 0; i < table.nbAttr; i++) {
    std::string tmp = table.relpath + "." + table.vnames[i];
    std::vector<std::string> tmp2;
    tmp2.push_back(tmp);
    fieldNames.push_back(tmp2);
  }
  fieldTypes = table.vtypes;
  fieldOrders = table.vorders;
}

BaseTable& LSelectNode::GetBaseTable() {
  return table;
}

LUniqueNode::LUniqueNode(std::unique_ptr<LAbstractNode> child_): LAbstractNode(std::move(child_), nullptr) {
  fieldNames = left->fieldNames;
  fieldTypes = left->fieldTypes;
  fieldOrders = left->fieldOrders;
}

LSortNode::LSortNode(std::unique_ptr<LAbstractNode> child_, std::string name): LAbstractNode(std::move(child_), nullptr) {
  fieldNames = left->fieldNames;
  fieldTypes = left->fieldTypes;
  fieldOrders = left->fieldOrders;
  fill(fieldOrders.begin(), fieldOrders.end(), CS_UNKNOWN);
  offset = -1;
  for (int i = 0; i < fieldNames.size(); i++) {
    if (std::find(fieldNames[i].begin(), fieldNames[i].end(), name) != fieldNames[i].end()) {
      offset = i;
      break;
    }
  }
  assert(offset != -1);
  fieldOrders[offset] = CS_ASCENDING;
}

/* Physical nodes*/

std::ostream& operator<<(std::ostream &stream, const PStats &s) {
  return stream << "{rewound=" << s.rewound
                << ",output_blocks=" << s.output_blocks
                << ",non_empty_output_blocks=" << s.non_empty_output_blocks
                << ",output_rows=" << s.output_rows
                << "}";
}

PResultNode::PResultNode(std::unique_ptr<PResultNode> left_, std::unique_ptr<PResultNode> right_, LAbstractNode* p)
  : left(std::move(left_)), right(std::move(right_)), prototype(p) {
}

PResultNode::~PResultNode() {
}
