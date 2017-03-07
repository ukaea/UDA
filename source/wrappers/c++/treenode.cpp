//
// Created by jholloc on 08/03/16.
//

#include "treenode.hpp"

#include <structures/struct.h>
#include <structures/accessors.h>

uda::TreeNode uda::TreeNode::parent()
{
    return TreeNode(node_->parent);
}

size_t uda::TreeNode::numChildren()
{
    return getNodeChildrenCount(node_);
}

std::vector<uda::TreeNode> uda::TreeNode::children()
{
    int numChildren = getNodeChildrenCount(node_);

    std::vector<TreeNode> vec;
    for (int i = 0; i < numChildren; ++i) {
        vec.push_back(TreeNode(getNodeChild(node_, i)));
    }

    return vec;
}

uda::TreeNode uda::TreeNode::child(int num)
{
    return TreeNode(getNodeChild(node_, num));
}

void uda::TreeNode::print()
{
    printNode(node_);
}

std::string uda::TreeNode::name()
{
    char* name = getNodeStructureName(node_);
    return name == NULL ? "" : name;
}

void uda::TreeNode::printStructureNames()
{
    printNTreeStructureNames(node_);
}

uda::TreeNode uda::TreeNode::findStructureDefinition(const std::string& name)
{
    return TreeNode(findNTreeStructureDefinition(node_, (char*) name.c_str()));
}

uda::TreeNode uda::TreeNode::findStructureComponent(const std::string& name)
{
    return TreeNode(findNTreeStructureComponent(node_, (char*) name.c_str()));
}

void uda::TreeNode::printUserDefinedTypeTable(const std::string& name)
{
    USERDEFINEDTYPE* type = findUserDefinedType((char*) name.c_str(), 0);
    ::printUserDefinedTypeTable(*type);
}

void uda::TreeNode::printUserDefinedTypeTable()
{
    USERDEFINEDTYPE* type = getNodeUserDefinedType(node_);
    ::printUserDefinedTypeTable(*type);
}

int uda::TreeNode::structureCount()
{
    return getNodeStructureCount(node_);
}

std::vector<std::string> uda::TreeNode::structureNames()
{
    char** names = getNodeStructureNames(node_);
    int size = getNodeStructureCount(node_);
    std::vector<std::string> vec(names, names + size);
    return vec;
}

std::vector<std::string> uda::TreeNode::structureTypes()
{
    char** names = getNodeStructureTypes(node_);
    int size = getNodeStructureCount(node_);
    std::vector<std::string> vec(names, names + size);
    return vec;
}

int uda::TreeNode::atomicCount()
{
    return getNodeAtomicCount(node_);
}

std::vector<std::string> uda::TreeNode::atomicNames()
{
    char** names = getNodeAtomicNames(node_);
    int size = getNodeAtomicCount(node_);
    std::vector<std::string> vec(names, names + size);
    return vec;
}

std::vector<std::string> uda::TreeNode::atomicTypes()
{
    char** types = getNodeAtomicTypes(node_);
    int size = getNodeAtomicCount(node_);
    std::vector<std::string> vec(types, types + size);
    return vec;
}

std::vector<bool> uda::TreeNode::atomicPointers()
{
    int* isptr = getNodeAtomicPointers(node_);
    int size = getNodeAtomicCount(node_);
    std::vector<bool> vec(isptr, isptr + size);
    return vec;
}

std::vector<std::size_t> uda::TreeNode::atomicRank()
{
    int* ranks = getNodeAtomicRank(node_);
    int size = getNodeAtomicCount(node_);
    std::vector<std::size_t> vec(ranks, ranks + size);
    return vec;
}

std::vector<std::vector<std::size_t> > uda::TreeNode::atomicShape()
{
    int** shapes = getNodeAtomicShape(node_);
    int* ranks = getNodeAtomicRank(node_);
    int size = getNodeAtomicCount(node_);
    std::vector<std::vector<std::size_t> > vec;
    for (int i = 0; i < size; ++i) {
        if (shapes[i] == NULL || ranks[i] == 0) {
            std::vector<std::size_t> vec2;
            vec2.push_back(0);
            vec.push_back(vec2);
        } else {
            std::vector<std::size_t> vec2(shapes[i], shapes[i] + ranks[i]);
            vec.push_back(vec2);
        }
    }
    return vec;
}

void* uda::TreeNode::structureComponentData(const std::string& name)
{
    return getNodeStructureComponentData(node_, (char*) name.c_str());
}

template<typename T>
static uda::Scalar getScalar(NTREE* node, const char* name)
{
    T* val = reinterpret_cast<T*>(getNodeStructureComponentData(node, (char*) name));
    return uda::Scalar(*val);
}

template<>
uda::Scalar getScalar<char*>(NTREE* node, const char* name)
{
    char* val = reinterpret_cast<char*>(getNodeStructureComponentData(node, (char*) name));
    return uda::Scalar(val);
}

template<>
uda::Scalar getScalar<char**>(NTREE* node, const char* name)
{
    char** val = reinterpret_cast<char**>(getNodeStructureComponentData(node, (char*) name));
    return uda::Scalar(val[0]);
}

uda::Scalar uda::TreeNode::atomicScalar(const std::string& target)
{
    NTREE* node = findNTreeStructureComponent(node_, (char*) target.c_str()); // Locate the named variable target
    //NTREE * node = findNTreeStructureComponent(node_, target.c_str()); // Locate the named variable target
    if (node == NULL) return Scalar::Null;

    int acount = getNodeAtomicCount(node); // Number of atomic typed structure members
    if (acount == 0) return Scalar::Null; // No atomic data

    char** anames = getNodeAtomicNames(node);
    char** atypes = getNodeAtomicTypes(node);
    int* arank = getNodeAtomicRank(node);
    int** ashape = getNodeAtomicShape(node);

    if (anames == NULL || atypes == NULL || arank == NULL || ashape == NULL) {
        return Scalar::Null;
    }

    for (int i = 0; i < acount; i++) {
        if (target == anames[i]
            && std::string("STRING") == atypes[i]
            && (arank[i] == 0 || arank[i] == 1)) {
            return getScalar<char*>(node, anames[i]);
        } else if (target == anames[i]
                   && std::string("STRING *") == atypes[i]
                   && (arank[i] == 0 || arank[i] == 1)) {
            return getScalar<char**>(node, anames[i]);
        } else if (target == anames[i]
                   && (arank[i] == 0
                       || (arank[i] == 1 && ashape[i][0] == 1))) {
            if (std::string("short") == atypes[i]) return getScalar<short>(node, anames[i]);
            if (std::string("double") == atypes[i]) return getScalar<double>(node, anames[i]);
            if (std::string("float") == atypes[i]) return getScalar<float>(node, anames[i]);
            if (std::string("int") == atypes[i]) return getScalar<int>(node, anames[i]);
            if (std::string("unsigned int") == atypes[i]) return getScalar<unsigned int>(node, anames[i]);
            if (std::string("unsigned short") == atypes[i]) return getScalar<unsigned short>(node, anames[i]);
        }
    }

    return Scalar::Null;
}

template<typename T>
static uda::Vector getVector(NTREE* node, const std::string& target)
{
    int count = getNodeChildrenCount(node->parent);
    T* data = static_cast<T*>(malloc(count * sizeof(T)));
    if (data == NULL) {
        return uda::Vector::Null;
    }
    for (int j = 0; j < count; j++) {
        data[j] = *reinterpret_cast<T*>(getNodeStructureComponentData(node->parent->children[j],
                                                                      (char*) target.c_str()));
    }
    return uda::Vector(data, count);
}

template<>
uda::Vector getVector<char*>(NTREE* node, const std::string& target)
{
    // Scalar String in an array of data structures
    int count = getNodeChildrenCount(node->parent);
    char** data = static_cast<char**>(malloc(count * sizeof(char*))); // Managed by IDAM
    if (data == NULL) {
        return uda::Vector::Null;
    }
    addMalloc(data, count, sizeof(char*), (char*) "char *");
    for (int j = 0; j < count; j++) {
        data[j] = reinterpret_cast<char*>(getNodeStructureComponentData(node->parent->children[j],
                                                                        (char*) target.c_str()));
    }
    return uda::Vector(data, count);
}

uda::Vector uda::TreeNode::atomicVector(const std::string& target)
{
    NTREE* node = findNTreeStructureComponent(node_, (char*) target.c_str());
    //NTREE * node = findNTreeStructureComponent(node_, (char *)target.c_str()); // Locate the named variable target
    if (node == NULL) return Vector::Null;

    int acount = getNodeAtomicCount(node); // Number of atomic typed structure members
    if (acount == 0) return Vector::Null; // No atomic data

    char** anames = getNodeAtomicNames(node);
    char** atypes = getNodeAtomicTypes(node);
    int* apoint = getNodeAtomicPointers(node);
    int* arank = getNodeAtomicRank(node);
    int** ashape = getNodeAtomicShape(node);

    if (anames == NULL || atypes == NULL || apoint == NULL || arank == NULL || ashape == NULL) {
        return Vector::Null;
    }

    for (int i = 0; i < acount; i++) {
        if (target == anames[i]) {
            if (std::string("STRING *") == atypes[i]
                && arank[i] == 0
                && apoint[i] == 1) {
                // String array in a single data structure
                char** val = reinterpret_cast<char**>(getNodeStructureComponentData(node, (char*) target.c_str()));
                return uda::Vector(val, ashape[i][0]);
            } else if (arank[i] == 0 || arank[i] == 1) {
                if (std::string("STRING") == atypes[i]) return getVector<char*>(node, target);
                if (std::string("short") == atypes[i]) return getVector<short>(node, target);
                if (std::string("double") == atypes[i]) return getVector<double>(node, target);
                if (std::string("float") == atypes[i]) return getVector<float>(node, target);
                if (std::string("int") == atypes[i]) return getVector<int>(node, target);
                if (std::string("unsigned int") == atypes[i]) return getVector<unsigned int>(node, target);
                if (std::string("unsigned short") == atypes[i]) return getVector<unsigned short>(node, target);
            }
        }
    }

    return Vector::Null;
}

uda::StructData uda::TreeNode::structData(const std::string& target)
{
    NTREE* node = findNTreeStructureComponent(node_, (char*) target.c_str());
    if (node == NULL) return StructData::Null;

    int count = getNodeChildrenCount(node->parent);
    //void ** data = static_cast<void **>(malloc(count * sizeof(void *)));

    uda::StructData data;

    //addMalloc(data, count, sizeof(void *), (char *)"void *");
    for (int j = 0; j < count; j++) {
        void* ptr = getNodeData(node->parent->children[j]);
        std::string name(getNodeStructureType(node->parent->children[j]));
        std::size_t size = static_cast<std::size_t>(getNodeStructureSize(node->parent->children[j]));
        data.append(name, size, ptr);
    }

    return data;
}

void* uda::TreeNode::data()
{
    return getNodeData(node_);
}


