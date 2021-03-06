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

#ifndef INTERFACE_H
#define INTERFACE_H
#include <string.h>
#include <vector>
#include <string>
#include <iostream>
#include <sstream>
#include <fstream>
#include <memory>

#include "basics.h"

/* Logical nodes (query) */

class LAbstractNode {
  public:
    LAbstractNode(std::unique_ptr<LAbstractNode> left, std::unique_ptr<LAbstractNode> right);
    virtual ~LAbstractNode();
    LAbstractNode* GetLeft();
    LAbstractNode* GetRight();
    // schema-related info
    std::vector<std::vector<std::string>> fieldNames;
    std::vector<ValueType> fieldTypes;
    std::vector<COLUMN_SORT> fieldOrders;
  protected:
    std::unique_ptr<LAbstractNode> left;
    std::unique_ptr<LAbstractNode> right;
};

class LCrossProductNode : public LAbstractNode {
  public:
    LCrossProductNode(std::unique_ptr<LAbstractNode> left, std::unique_ptr<LAbstractNode> right);
};

enum class LJoinType { NESTED_LOOP, SORTED_MERGE, HASH_JOIN, DE_HASH_JOIN };

class LJoinNode : public LAbstractNode {
  public:
    // offsets are defined as "TableName.AttributeName" so, ensure there is no duplicates
    LJoinNode(std::unique_ptr<LAbstractNode> left, std::unique_ptr<LAbstractNode> right, std::string offset1, std::string offset2, int memorylimit, LJoinType type);
    // attributes to perform equi-join on
    std::string offset1, offset2;
    // maximum number of records permitted to present inside physical node
    int memorylimit;
    LJoinType type;
};

class LUnionNode : public LAbstractNode {
  public:
    // offsets are defined as "TableName.AttributeName" so, ensure there is no duplicates
    LUnionNode(std::unique_ptr<LAbstractNode> left, std::unique_ptr<LAbstractNode> right);
};

class LProjectNode : public LAbstractNode {
  public:
    // offsets to keep
    LProjectNode(std::unique_ptr<LAbstractNode> child, std::vector<std::string> tokeep);
    // offsets are defined as "TableName.AttributeName" so, ensure there is no duplicates
    std::vector<std::string> offsets;
};

class LSelectNode : public LAbstractNode {
  public:
    LSelectNode(BaseTable& table, const Predicate *predicate);
    // returns a reference to BaseTable
    BaseTable& GetBaseTable();

    const Predicate *predicate;
  private:
    BaseTable table;
};

class LUniqueNode : public LAbstractNode {
  public:
    LUniqueNode(std::unique_ptr<LAbstractNode> child);
};

class LSortNode : public LAbstractNode {
  public:
    LSortNode(std::unique_ptr<LAbstractNode> child, std::string name);
    int offset;
};

// Physical node interface (result), should be used for automatic testing

struct PStats {
  int rewound = 0;
  int output_blocks = 0;
  int non_empty_output_blocks = 0;
  int output_rows = 0;
};

std::ostream& operator<<(std::ostream&, const PStats&);

class PResultNode {
  public:
    PResultNode(std::unique_ptr<PResultNode> left, std::unique_ptr<PResultNode> right, LAbstractNode* p);
    virtual ~PResultNode();
    // returns number of attributes
    virtual int GetAttrNum() = 0;
    // prints tree
    virtual void Print(int indent, bool print_stats = false) = 0;
    const PStats& stats() { return stats_; }
  protected:
    std::unique_ptr<PResultNode> left;
    std::unique_ptr<PResultNode> right;
    PStats stats_;
  public:
    // used to get attribute info
    LAbstractNode* prototype;
};

#endif // INTERFACE_H
