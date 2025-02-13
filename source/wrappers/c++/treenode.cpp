#include "treenode.hpp"

#include <uda/client.h>
#include <uda/structured.h>

uda::TreeNode uda::TreeNode::parent()
{
    auto parent = udaGetNodeParent(node_);
    return TreeNode(handle_, parent);
}

size_t uda::TreeNode::numChildren() const
{
    return udaGetNodeChildrenCount(node_);
}

const std::vector<uda::TreeNode> uda::TreeNode::children() const
{
    int numChildren = udaGetNodeChildrenCount(node_);

    std::vector<TreeNode> vec;
    for (int i = 0; i < numChildren; ++i) {
        auto child = udaGetNodeChild(node_, i);
        vec.push_back(TreeNode{handle_, child});
    }

    return vec;
}

std::vector<uda::TreeNode> uda::TreeNode::children()
{
    int numChildren = udaGetNodeChildrenCount(node_);

    std::vector<TreeNode> vec;
    for (int i = 0; i < numChildren; ++i) {
        vec.push_back(TreeNode(handle_, udaGetNodeChild(node_, i)));
    }

    return vec;
}

uda::TreeNode uda::TreeNode::child(int num)
{
    return TreeNode(handle_, udaGetNodeChild(node_, num));
}

void uda::TreeNode::printNode()
{
    ::udaPrintNode(node_);
}

std::string uda::TreeNode::name() const
{
    char* name = udaGetNodeStructureName(node_);
    return name == nullptr ? "" : name;
}

uda::TreeNode uda::TreeNode::findStructureDefinition(const std::string& name)
{
    return {handle_, udaFindNTreeStructureDefinition(node_, name.c_str())};
}

uda::TreeNode uda::TreeNode::findStructureComponent(const std::string& name)
{
    return {handle_, udaFindNTreeStructureComponent(node_, name.c_str())};
}

int uda::TreeNode::structureCount()
{
    return udaGetNodeStructureCount(node_);
}

std::vector<std::string> uda::TreeNode::structureNames()
{
    char** names = udaGetNodeStructureNames(node_);
    int size = udaGetNodeStructureCount(node_);
    std::vector<std::string> vec(names, names + size);
    return vec;
}

std::vector<std::string> uda::TreeNode::structureTypes()
{
    char** names = udaGetNodeStructureTypes(node_);
    int size = udaGetNodeStructureCount(node_);
    std::vector<std::string> vec(names, names + size);
    return vec;
}

int uda::TreeNode::atomicCount() const
{
    return udaGetNodeAtomicCount(node_);
}

std::vector<std::string> uda::TreeNode::atomicNames() const
{
    char** names = udaGetNodeAtomicNames(node_);
    int size = udaGetNodeAtomicCount(node_);
    std::vector<std::string> vec(names, names + size);
    return vec;
}

std::vector<std::string> uda::TreeNode::atomicTypes() const
{
    char** types = udaGetNodeAtomicTypes(node_);
    int size = udaGetNodeAtomicCount(node_);
    std::vector<std::string> vec(types, types + size);
    return vec;
}

std::vector<bool> uda::TreeNode::atomicPointers() const
{
    int* isptr = udaGetNodeAtomicPointers(node_);
    int size = udaGetNodeAtomicCount(node_);
    std::vector<bool> vec(size);
    for (int i = 0; i < size; ++i) {
        vec[i] = static_cast<bool>(isptr[i]);
    }
    return vec;
}

std::vector<std::size_t> uda::TreeNode::atomicRank() const
{
    int* ranks = udaGetNodeAtomicRank(node_);
    int size = udaGetNodeAtomicCount(node_);
    std::vector<std::size_t> vec(ranks, ranks + size);
    return vec;
}

std::vector<std::vector<std::size_t>> uda::TreeNode::atomicShape() const
{
    int** shapes = udaGetNodeAtomicShape(node_);
    int* ranks = udaGetNodeAtomicRank(node_);
    int size = udaGetNodeAtomicCount(node_);
    std::vector<std::vector<std::size_t>> vec;
    for (int i = 0; i < size; ++i) {
        if (shapes[i] == nullptr) {
            std::vector<std::size_t> vec2;
            vec2.push_back(0);
            vec.push_back(vec2);
        } else if (ranks[i] == 0) {
            std::vector<std::size_t> vec2(shapes[i], shapes[i] + 1);
            vec.push_back(vec2);
        } else {
            std::vector<std::size_t> vec2(shapes[i], shapes[i] + ranks[i]);
            vec.push_back(vec2);
        }
    }
    return vec;
}

void* uda::TreeNode::structureComponentData(const std::string& name) const
{
    return udaGetNodeStructureComponentData(node_, name.c_str());
}

template <typename T> static uda::Scalar getScalar(NTREE* node, const char* name)
{
    T* val = reinterpret_cast<T*>(udaGetNodeStructureComponentData(node, name));
    return uda::Scalar(*val);
}

template <> uda::Scalar getScalar<char*>(NTREE* node, const char* name)
{
    char* val = reinterpret_cast<char*>(udaGetNodeStructureComponentData(node, name));
    return uda::Scalar(val);
}

template <> uda::Scalar getScalar<char**>(NTREE* node, const char* name)
{
    char** val = reinterpret_cast<char**>(udaGetNodeStructureComponentData(node, name));
    return uda::Scalar(val[0]);
}

uda::Scalar uda::TreeNode::atomicScalar(const std::string& target)
{
    NTREE* node =
        udaFindNTreeStructureComponent(node_, target.c_str()); // Locate the named variable target
    // NTREE * node = udaFindNTreeStructureComponent(node_, target.c_str()); // Locate the named variable target
    if (node == nullptr) {
        return Scalar::Null;
    }

    int acount = udaGetNodeAtomicCount(node); // Number of atomic typed structure members
    if (acount == 0) {
        return Scalar::Null; // No atomic data
    }

    char** anames = udaGetNodeAtomicNames(node);
    char** atypes = udaGetNodeAtomicTypes(node);
    int* arank = udaGetNodeAtomicRank(node);
    int** ashape = udaGetNodeAtomicShape(node);

    if (anames == nullptr || atypes == nullptr || arank == nullptr || ashape == nullptr) {
        return Scalar::Null;
    }

    for (int i = 0; i < acount; i++) {
        if (target == anames[i] && std::string("STRING") == atypes[i] && (arank[i] == 0 || arank[i] == 1)) {
            return getScalar<char*>(node, anames[i]);
        } else if (target == anames[i] && std::string("STRING *") == atypes[i] && arank[i] == 0) {
            return getScalar<char**>(node, anames[i]);
        } else if (target == anames[i] && (arank[i] == 0 || (arank[i] == 1 && ashape[i][0] == 1))) {
            if (std::string("short") == atypes[i]) {
                return getScalar<short>(node, anames[i]);
            }
            if (std::string("double") == atypes[i]) {
                return getScalar<double>(node, anames[i]);
            }
            if (std::string("float") == atypes[i]) {
                return getScalar<float>(node, anames[i]);
            }
            if (std::string("int") == atypes[i]) {
                return getScalar<int>(node, anames[i]);
            }
            if (std::string("char") == atypes[i]) {
                return getScalar<char>(node, anames[i]);
            }
            if (std::string("unsigned int") == atypes[i]) {
                return getScalar<unsigned int>(node, anames[i]);
            }
            if (std::string("unsigned short") == atypes[i]) {
                return getScalar<unsigned short>(node, anames[i]);
            }
            if (std::string("unsigned char") == atypes[i]) {
                return getScalar<unsigned char>(node, anames[i]);
            }
        }
    }

    return Scalar::Null;
}

// template <typename T>
// static uda::Vector getVectorOverSiblings(NTREE* node, const std::string& target)
//{
//     int count = udaGetNodeChildrenCount(node->parent);
//     T* data = static_cast<T*>(malloc(count * sizeof(T)));
//     if (data == nullptr) {
//         return uda::Vector::Null;
//     }
//     for (int j = 0; j < count; j++) {
//         data[j] = *reinterpret_cast<T*>(udaGetNodeStructureComponentData(node->parent->children[j],
//                                                                       target.c_str()));
//     }
//     return uda::Vector(data, (size_t)count);
// }
//
// template <>
// uda::Vector getVectorOverSiblings<char*>(NTREE* node, const std::string& target)
//{
//     // Scalar String in an array of data structures
//     int count = udaGetNodeChildrenCount(node->parent);
//     char** data = static_cast<char**>(malloc(count * sizeof(char*))); // Managed by IDAM
//     if (data == nullptr) {
//         return uda::Vector::Null;
//     }
//     udaAddMalloc(data, count, sizeof(char*), (char*)"char *");
//     for (int j = 0; j < count; j++) {
//         data[j] = reinterpret_cast<char*>(udaGetNodeStructureComponentData(node->parent->children[j],
//                                                                         target.c_str()));
//     }
//     return uda::Vector(data, (size_t)count);
// }

template <typename T>
static uda::Vector getVector(NTREE* node, const std::string& target, int count)
{
    T* data = reinterpret_cast<T*>(udaGetNodeStructureComponentData(node, target.c_str()));

    return uda::Vector(data, (size_t)count);
}

uda::Vector getStringVector(NTREE* node, const std::string& target, int* shape)
{
    int count = shape[1];

    auto data = static_cast<char**>(malloc(count * sizeof(char*)));
    if (data == nullptr) {
        return uda::Vector::Null;
    }

    auto val = reinterpret_cast<char*>(udaGetNodeStructureComponentData(node, target.c_str()));

    for (int j = 0; j < count; j++) {
        data[j] = &val[j * shape[0]];
    }

    return uda::Vector(data, (size_t)count);
}

uda::Vector getStringVector(NTREE* node, const std::string& target)
{
    auto data = static_cast<char**>(malloc(sizeof(char*)));
    if (data == nullptr) {
        return uda::Vector::Null;
    }

    auto val = reinterpret_cast<char*>(udaGetNodeStructureComponentData(node, target.c_str()));
    data[0] = val;

    return uda::Vector(data, (size_t)1);
}

uda::Vector uda::TreeNode::atomicVector(const std::string& target)
{
    NTREE* node = udaFindNTreeStructureComponent(node_, target.c_str());
    if (node == nullptr) {
        return Vector::Null;
    }

    int acount = udaGetNodeAtomicCount(node); // Number of atomic typed structure members
    if (acount == 0) {
        return Vector::Null; // No atomic data
    }

    char** anames = udaGetNodeAtomicNames(node);
    char** atypes = udaGetNodeAtomicTypes(node);
    int* apoint = udaGetNodeAtomicPointers(node);
    int* arank = udaGetNodeAtomicRank(node);
    int** ashape = udaGetNodeAtomicShape(node);

    if (anames == nullptr || atypes == nullptr || apoint == nullptr || arank == nullptr || ashape == nullptr) {
        return Vector::Null;
    }

    for (int i = 0; i < acount; i++) {
        if (target == anames[i]) {
            if (std::string("STRING *") == atypes[i] &&
                ((arank[i] == 0 && apoint[i] == 1) || (arank[i] == 1 && apoint[i] == 0))) {
                // String array in a single data structure
                char** val = reinterpret_cast<char**>(
                    udaGetNodeStructureComponentData(node, target.c_str()));
                return uda::Vector(val, (size_t)ashape[i][0]);
            } else if (arank[i] == 0 && apoint[i] == 1) {
                int count = udaGetNodeStructureComponentDataCount(node, target.c_str());
                if (std::string("STRING *") == atypes[i]) {
                    return getVector<char*>(node, target, count);
                }
                if (std::string("char *") == atypes[i]) {
                    return getVector<char>(node, target, count);
                }
                if (std::string("short *") == atypes[i]) {
                    return getVector<short>(node, target, count);
                }
                if (std::string("double *") == atypes[i]) {
                    return getVector<double>(node, target, count);
                }
                if (std::string("float *") == atypes[i]) {
                    return getVector<float>(node, target, count);
                }
                if (std::string("int *") == atypes[i]) {
                    return getVector<int>(node, target, count);
                }
                if (std::string("unsigned int *") == atypes[i]) {
                    return getVector<unsigned int>(node, target, count);
                }
                if (std::string("unsigned short *") == atypes[i]) {
                    return getVector<unsigned short>(node, target, count);
                }
                if (std::string("unsigned char *") == atypes[i]) {
                    return getVector<unsigned char>(node, target, count);
                }
            } else if (arank[i] == 1) {
                //                if (std::string("STRING") == atypes[i]) return getVector<char>(node,
                //                target, ashape[i][0]); if (std::string("STRING") == atypes[i]) return
                //                getStringVector(node, target);
                if (std::string("char") == atypes[i]) {
                    return getVector<char>(node, target, ashape[i][0]);
                }
                if (std::string("short") == atypes[i]) {
                    return getVector<short>(node, target, ashape[i][0]);
                }
                if (std::string("double") == atypes[i]) {
                    return getVector<double>(node, target, ashape[i][0]);
                }
                if (std::string("float") == atypes[i]) {
                    return getVector<float>(node, target, ashape[i][0]);
                }
                if (std::string("int") == atypes[i]) {
                    return getVector<int>(node, target, ashape[i][0]);
                }
                if (std::string("unsigned int") == atypes[i]) {
                    return getVector<unsigned int>(node, target, ashape[i][0]);
                }
                if (std::string("unsigned short") == atypes[i]) {
                    return getVector<unsigned short>(node, target, ashape[i][0]);
                }
                if (std::string("unsigned char") == atypes[i]) {
                    return getVector<unsigned char>(node, target, ashape[i][0]);
                }
            } else if (arank[i] == 2 && std::string("STRING") == atypes[i]) {
                return getStringVector(node, target, ashape[i]);
            }
        }
    }

    return Vector::Null;
}

template <typename T>
static uda::Array getArray(NTREE* node, const std::string& target, int* shape, int rank)
{
    auto data = static_cast<T*>(udaGetNodeStructureComponentData(node, target.c_str()));

    std::vector<uda::Dim> dims;
    for (int i = 0; i < rank; ++i) {
        std::vector<int> dim((size_t)shape[i]);
        for (int j = 0; j < shape[i]; ++j) {
            dim[j] = j;
        }
        dims.emplace_back(uda::Dim(static_cast<uda::dim_type>(i), dim.data(), static_cast<size_t>(shape[i]), "", ""));
    }

    return uda::Array(data, dims);
}

uda::Array uda::TreeNode::atomicArray(const std::string& target)
{
    NTREE* node = udaFindNTreeStructureComponent(node_, target.c_str());
    if (node == nullptr) {
        return Array::Null;
    }

    int acount = udaGetNodeAtomicCount(node); // Number of atomic typed structure members
    if (acount == 0) {
        return Array::Null; // No atomic data
    }

    char** anames = udaGetNodeAtomicNames(node);
    char** atypes = udaGetNodeAtomicTypes(node);
    int* apoint = udaGetNodeAtomicPointers(node);
    int* arank = udaGetNodeAtomicRank(node);
    int** ashape = udaGetNodeAtomicShape(node);

    if (anames == nullptr || atypes == nullptr || apoint == nullptr || arank == nullptr || ashape == nullptr) {
        return Array::Null;
    }

    for (int i = 0; i < acount; i++) {
        if (target == anames[i]) {
            if (std::string("STRING") == atypes[i]) {
                return getArray<char*>(node, target, ashape[i], arank[i]);
            }
            if (std::string("short") == atypes[i]) {
                return getArray<short>(node, target, ashape[i], arank[i]);
            }
            if (std::string("double") == atypes[i]) {
                return getArray<double>(node, target, ashape[i], arank[i]);
            }
            if (std::string("float") == atypes[i]) {
                return getArray<float>(node, target, ashape[i], arank[i]);
            }
            if (std::string("int") == atypes[i]) {
                return getArray<int>(node, target, ashape[i], arank[i]);
            }
            if (std::string("unsigned int") == atypes[i]) {
                return getArray<unsigned int>(node, target, ashape[i], arank[i]);
            }
            if (std::string("unsigned short") == atypes[i]) {
                return getArray<unsigned short>(node, target, ashape[i], arank[i]);
            }
            if (std::string("unsigned char") == atypes[i]) {
                return getArray<unsigned char>(node, target, ashape[i], arank[i]);
            }
        }
    }

    return Array::Null;
}

uda::StructData uda::TreeNode::structData(const std::string& target)
{
    NTREE* node = udaFindNTreeStructureComponent(node_, target.c_str());
    if (node == nullptr) {
        return StructData::Null;
    }

    const auto parent = udaGetNodeParent(node);
    const int count = udaGetNodeChildrenCount(parent);

    StructData data;

    for (int j = 0; j < count; j++) {
        const auto child = udaGetNodeChild(parent, j);
        void* ptr = udaGetNodeData(child);
        const std::string name(udaGetNodeStructureType(child));
        const auto size = static_cast<std::size_t>(udaGetNodeStructureSize(child));
        data.append(name, size, ptr);
    }

    return data;
}

void* uda::TreeNode::data()
{
    return udaGetNodeData(node_);
}
