#include<cstddef>
#include<algorithm>
#include<utility>
#include<iostream>

#include "pprojectnode.h"

PProjectNode::PProjectNode(std::unique_ptr<PGetNextNode> child_, LAbstractNode* p)
  : PGetNextNode(std::move(child_), nullptr, p) {
  LProjectNode* pn = (LProjectNode*)p;

  const auto& childFieldNames = p->GetLeft()->fieldNames;
  for (const auto &field : pn->offsets) {
    for (int i = 0; i < childFieldNames.size(); i++) {
      const auto& curNames = childFieldNames[i];
      if (std::find(curNames.begin(), curNames.end(), field) != curNames.end()) {
        indices.push_back(i);
      }
    }
  }
  Initialize();
}

std::vector<std::vector<Value>> PProjectNode::GetNext() {
  auto result = std::move(data);
  data.clear();
  return result;
}

void PProjectNode::Initialize() {
  PGetNextNode* l = (PGetNextNode*)left.get();
  std::vector<std::vector<Value>> lres = l->GetNext();
  for (const auto &lrow : lres) {
    std::vector<Value> result;
    for (int idx : indices)
      result.push_back(lrow[idx]);
    data.push_back(result);
  }
}

void PProjectNode::Print(int indent) {
  for (int i = 0; i < indent; i++) {
    std::cout << " ";
  }
  std::cout << "PROJECT:";
  for (int idx : indices)
    std::cout << " " << idx << "(" << prototype->GetLeft()->fieldNames[idx][0] << ")";
  std::cout << std::endl;
  left->Print(indent + 2);
}
