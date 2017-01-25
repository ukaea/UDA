//
// Created by jholloc on 08/03/16.
//

#ifndef IDAM_WRAPPERS_CPP_TREENODE_H
#define IDAM_WRAPPERS_CPP_TREENODE_H

#include <string>
#include <vector>

#include <include/idamgenstructpublic.h>

#include "scalar.hpp"
#include "vector.hpp"
#include "structdata.hpp"

namespace Idam {

class TreeNode
{
public:
    TreeNode findStructureDefinition(const std::string& name);
    TreeNode findStructureComponent(const std::string& name);
    TreeNode parent();
    size_t numChildren();
    std::vector<Idam::TreeNode> children();
    Idam::TreeNode child(int num);
    void print();
    void printStructureNames();
    void printUserDefinedTypeTable();
    void printUserDefinedTypeTable(const std::string& name);

    int structureCount();
    std::vector<std::string> structureNames();
    std::vector<std::string> structureTypes();

    std::string name();

    int atomicCount();
    std::vector<std::string> atomicNames();
    std::vector<std::string> atomicTypes();
    std::vector<bool> atomicPointers();
    std::vector<size_t> atomicRank();
    std::vector<std::vector<size_t> > atomicShape();
    void * structureComponentData(const std::string& name);
    Scalar atomicScalar(const std::string& target);
    Vector atomicVector(const std::string& target);

    StructData structData(const std::string& target);
    void * data();
private:
    TreeNode(NTREE * node) : node_(node) {}
    friend class Result;
    NTREE * node_;
};


}

#endif //IDAM_WRAPPERS_CPP_TREENODE_H
