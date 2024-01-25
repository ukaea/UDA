#include "treenode.hpp"

#include "accAPI.h"
#include "accessors.h"
#include "struct.h"

uda::TreeNode uda::TreeNode::parent()
{
    return TreeNode(handle_, node_->parent);
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

void uda::TreeNode::udaPrintNode()
{
    ::udaPrintNode(node_);
}

std::string uda::TreeNode::name() const
{
    char* name = udaGetNodeStructureName(node_);
    return name == nullptr ? "" : name;
}

void uda::TreeNode::printStructureNames()
{
    LOGMALLOCLIST* logmalloclist = udaGetLogMallocList(handle_);
    udaPrintNTreeStructureNames(logmalloclist, node_);
}

uda::TreeNode uda::TreeNode::findStructureDefinition(const std::string& name)
{
    return {handle_, findNTreeStructureDefinition(node_, (char*)name.c_str())};
}

uda::TreeNode uda::TreeNode::findStructureComponent(const std::string& name)
{
    LOGMALLOCLIST* logmalloclist = udaGetLogMallocList(handle_);
    return {handle_, findNTreeStructureComponent(logmalloclist, node_, (char*)name.c_str())};
}

void uda::TreeNode::udaPrintUserDefinedTypeTable(const std::string& name)
{
    USERDEFINEDTYPELIST* userdefinedtypelist = udaGetUserDefinedTypeList(handle_);
    USERDEFINEDTYPE* type = findUserDefinedType(userdefinedtypelist, (char*)name.c_str(), 0);
    ::udaPrintUserDefinedTypeTable(userdefinedtypelist, *type);
}

void uda::TreeNode::udaPrintUserDefinedTypeTable()
{
    USERDEFINEDTYPELIST* userdefinedtypelist = udaGetUserDefinedTypeList(handle_);
    USERDEFINEDTYPE* type = getNodeUserDefinedType(node_);
    ::udaPrintUserDefinedTypeTable(userdefinedtypelist, *type);
}

int uda::TreeNode::structureCount()
{
    return udaGetNodeStructureCount(node_);
}

std::vector<std::string> uda::TreeNode::structureNames()
{
    LOGMALLOCLIST* logmalloclist = udaGetLogMallocList(handle_);
    char** names = udaGetNodeStructureNames(logmalloclist, node_);
    int size = udaGetNodeStructureCount(node_);
    std::vector<std::string> vec(names, names + size);
    return vec;
}

std::vector<std::string> uda::TreeNode::structureTypes()
{
    LOGMALLOCLIST* logmalloclist = udaGetLogMallocList(handle_);
    char** names = udaGetNodeStructureTypes(logmalloclist, node_);
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
    LOGMALLOCLIST* logmalloclist = udaGetLogMallocList(handle_);
    char** names = udaGetNodeAtomicNames(logmalloclist, node_);
    int size = udaGetNodeAtomicCount(node_);
    std::vector<std::string> vec(names, names + size);
    return vec;
}

std::vector<std::string> uda::TreeNode::atomicTypes() const
{
    LOGMALLOCLIST* logmalloclist = udaGetLogMallocList(handle_);
    char** types = udaGetNodeAtomicTypes(logmalloclist, node_);
    int size = udaGetNodeAtomicCount(node_);
    std::vector<std::string> vec(types, types + size);
    return vec;
}

std::vector<bool> uda::TreeNode::atomicPointers() const
{
    LOGMALLOCLIST* logmalloclist = udaGetLogMallocList(handle_);
    int* isptr = udaGetNodeAtomicPointers(logmalloclist, node_);
    int size = udaGetNodeAtomicCount(node_);
    std::vector<bool> vec(size);
    for (int i = 0; i < size; ++i) {
        vec[i] = static_cast<bool>(isptr[i]);
    }
    return vec;
}

std::vector<std::size_t> uda::TreeNode::atomicRank() const
{
    LOGMALLOCLIST* logmalloclist = udaGetLogMallocList(handle_);
    int* ranks = udaGetNodeAtomicRank(logmalloclist, node_);
    int size = udaGetNodeAtomicCount(node_);
    std::vector<std::size_t> vec(ranks, ranks + size);
    return vec;
}

std::vector<std::vector<std::size_t>> uda::TreeNode::atomicShape() const
{
    LOGMALLOCLIST* logmalloclist = udaGetLogMallocList(handle_);
    int** shapes = udaGetNodeAtomicShape(logmalloclist, node_);
    int* ranks = udaGetNodeAtomicRank(logmalloclist, node_);
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
    LOGMALLOCLIST* logmalloclist = udaGetLogMallocList(handle_);
    return udaGetNodeStructureComponentData(logmalloclist, node_, (char*)name.c_str());
}

template <typename T> static uda::Scalar getScalar(LOGMALLOCLIST* logmalloclist, NTREE* node, const char* name)
{
    T* val = reinterpret_cast<T*>(udaGetNodeStructureComponentData(logmalloclist, node, (char*)name));
    return uda::Scalar(*val);
}

template <> uda::Scalar getScalar<char*>(LOGMALLOCLIST* logmalloclist, NTREE* node, const char* name)
{
    char* val = reinterpret_cast<char*>(udaGetNodeStructureComponentData(logmalloclist, node, (char*)name));
    return uda::Scalar(val);
}

template <> uda::Scalar getScalar<char**>(LOGMALLOCLIST* logmalloclist, NTREE* node, const char* name)
{
    char** val = reinterpret_cast<char**>(udaGetNodeStructureComponentData(logmalloclist, node, (char*)name));
    return uda::Scalar(val[0]);
}

uda::Scalar uda::TreeNode::atomicScalar(const std::string& target)
{
    LOGMALLOCLIST* logmalloclist = udaGetLogMallocList(handle_);
    NTREE* node =
        findNTreeStructureComponent(logmalloclist, node_, (char*)target.c_str()); // Locate the named variable target
    // NTREE * node = findNTreeStructureComponent(node_, target.c_str()); // Locate the named variable target
    if (node == nullptr) {
        return Scalar::Null;
    }

    int acount = udaGetNodeAtomicCount(node); // Number of atomic typed structure members
    if (acount == 0) {
        return Scalar::Null; // No atomic data
    }

    char** anames = udaGetNodeAtomicNames(logmalloclist, node);
    char** atypes = udaGetNodeAtomicTypes(logmalloclist, node);
    int* arank = udaGetNodeAtomicRank(logmalloclist, node);
    int** ashape = udaGetNodeAtomicShape(logmalloclist, node);

    if (anames == nullptr || atypes == nullptr || arank == nullptr || ashape == nullptr) {
        return Scalar::Null;
    }

    for (int i = 0; i < acount; i++) {
        if (target == anames[i] && std::string("STRING") == atypes[i] && (arank[i] == 0 || arank[i] == 1)) {
            return getScalar<char*>(logmalloclist, node, anames[i]);
        } else if (target == anames[i] && std::string("STRING *") == atypes[i] && arank[i] == 0) {
            return getScalar<char**>(logmalloclist, node, anames[i]);
        } else if (target == anames[i] && (arank[i] == 0 || (arank[i] == 1 && ashape[i][0] == 1))) {
            if (std::string("short") == atypes[i]) {
                return getScalar<short>(logmalloclist, node, anames[i]);
            }
            if (std::string("double") == atypes[i]) {
                return getScalar<double>(logmalloclist, node, anames[i]);
            }
            if (std::string("float") == atypes[i]) {
                return getScalar<float>(logmalloclist, node, anames[i]);
            }
            if (std::string("int") == atypes[i]) {
                return getScalar<int>(logmalloclist, node, anames[i]);
            }
            if (std::string("char") == atypes[i]) {
                return getScalar<char>(logmalloclist, node, anames[i]);
            }
            if (std::string("unsigned int") == atypes[i]) {
                return getScalar<unsigned int>(logmalloclist, node, anames[i]);
            }
            if (std::string("unsigned short") == atypes[i]) {
                return getScalar<unsigned short>(logmalloclist, node, anames[i]);
            }
            if (std::string("unsigned char") == atypes[i]) {
                return getScalar<unsigned char>(logmalloclist, node, anames[i]);
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
//                                                                       (char*)target.c_str()));
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
//                                                                         (char*)target.c_str()));
//     }
//     return uda::Vector(data, (size_t)count);
// }

template <typename T>
static uda::Vector getVector(LOGMALLOCLIST* logmalloclist, NTREE* node, const std::string& target, int count)
{
    T* data = reinterpret_cast<T*>(udaGetNodeStructureComponentData(logmalloclist, node, (char*)target.c_str()));

    return uda::Vector(data, (size_t)count);
}

uda::Vector getStringVector(LOGMALLOCLIST* logmalloclist, NTREE* node, const std::string& target, int* shape)
{
    int count = shape[1];

    auto data = static_cast<char**>(malloc(count * sizeof(char*)));
    if (data == nullptr) {
        return uda::Vector::Null;
    }

    auto val = reinterpret_cast<char*>(udaGetNodeStructureComponentData(logmalloclist, node, (char*)target.c_str()));

    for (int j = 0; j < count; j++) {
        data[j] = &val[j * shape[0]];
    }

    return uda::Vector(data, (size_t)count);
}

uda::Vector getStringVector(LOGMALLOCLIST* logmalloclist, NTREE* node, const std::string& target)
{
    auto data = static_cast<char**>(malloc(sizeof(char*)));
    if (data == nullptr) {
        return uda::Vector::Null;
    }

    auto val = reinterpret_cast<char*>(udaGetNodeStructureComponentData(logmalloclist, node, (char*)target.c_str()));
    data[0] = val;

    return uda::Vector(data, (size_t)1);
}

uda::Vector uda::TreeNode::atomicVector(const std::string& target)
{
    LOGMALLOCLIST* logmalloclist = udaGetLogMallocList(handle_);
    NTREE* node = findNTreeStructureComponent(logmalloclist, node_, (char*)target.c_str());
    // NTREE * node = findNTreeStructureComponent(node_, (char *)target.c_str()); // Locate the named variable target
    if (node == nullptr) {
        return Vector::Null;
    }

    int acount = udaGetNodeAtomicCount(node); // Number of atomic typed structure members
    if (acount == 0) {
        return Vector::Null; // No atomic data
    }

    char** anames = udaGetNodeAtomicNames(logmalloclist, node);
    char** atypes = udaGetNodeAtomicTypes(logmalloclist, node);
    int* apoint = udaGetNodeAtomicPointers(logmalloclist, node);
    int* arank = udaGetNodeAtomicRank(logmalloclist, node);
    int** ashape = udaGetNodeAtomicShape(logmalloclist, node);

    if (anames == nullptr || atypes == nullptr || apoint == nullptr || arank == nullptr || ashape == nullptr) {
        return Vector::Null;
    }

    for (int i = 0; i < acount; i++) {
        if (target == anames[i]) {
            if (std::string("STRING *") == atypes[i] &&
                ((arank[i] == 0 && apoint[i] == 1) || (arank[i] == 1 && apoint[i] == 0))) {
                // String array in a single data structure
                char** val =
                    reinterpret_cast<char**>(udaGetNodeStructureComponentData(logmalloclist, node, (char*)target.c_str()));
                return uda::Vector(val, (size_t)ashape[i][0]);
            } else if (arank[i] == 0 && apoint[i] == 1) {
                int count = udaGetNodeStructureComponentDataCount(logmalloclist, node, (char*)target.c_str());
                if (std::string("STRING *") == atypes[i]) {
                    return getVector<char*>(logmalloclist, node, target, count);
                }
                if (std::string("char *") == atypes[i]) {
                    return getVector<char>(logmalloclist, node, target, count);
                }
                if (std::string("short *") == atypes[i]) {
                    return getVector<short>(logmalloclist, node, target, count);
                }
                if (std::string("double *") == atypes[i]) {
                    return getVector<double>(logmalloclist, node, target, count);
                }
                if (std::string("float *") == atypes[i]) {
                    return getVector<float>(logmalloclist, node, target, count);
                }
                if (std::string("int *") == atypes[i]) {
                    return getVector<int>(logmalloclist, node, target, count);
                }
                if (std::string("unsigned int *") == atypes[i]) {
                    return getVector<unsigned int>(logmalloclist, node, target, count);
                }
                if (std::string("unsigned short *") == atypes[i]) {
                    return getVector<unsigned short>(logmalloclist, node, target, count);
                }
                if (std::string("unsigned char *") == atypes[i]) {
                    return getVector<unsigned char>(logmalloclist, node, target, count);
                }
            } else if (arank[i] == 1) {
                //                if (std::string("STRING") == atypes[i]) return getVector<char>(logmalloclist, node,
                //                target, ashape[i][0]); if (std::string("STRING") == atypes[i]) return
                //                getStringVector(logmalloclist, node, target);
                if (std::string("char") == atypes[i]) {
                    return getVector<char>(logmalloclist, node, target, ashape[i][0]);
                }
                if (std::string("short") == atypes[i]) {
                    return getVector<short>(logmalloclist, node, target, ashape[i][0]);
                }
                if (std::string("double") == atypes[i]) {
                    return getVector<double>(logmalloclist, node, target, ashape[i][0]);
                }
                if (std::string("float") == atypes[i]) {
                    return getVector<float>(logmalloclist, node, target, ashape[i][0]);
                }
                if (std::string("int") == atypes[i]) {
                    return getVector<int>(logmalloclist, node, target, ashape[i][0]);
                }
                if (std::string("unsigned int") == atypes[i]) {
                    return getVector<unsigned int>(logmalloclist, node, target, ashape[i][0]);
                }
                if (std::string("unsigned short") == atypes[i]) {
                    return getVector<unsigned short>(logmalloclist, node, target, ashape[i][0]);
                }
                if (std::string("unsigned char") == atypes[i]) {
                    return getVector<unsigned char>(logmalloclist, node, target, ashape[i][0]);
                }
            } else if (arank[i] == 2 && std::string("STRING") == atypes[i]) {
                return getStringVector(logmalloclist, node, target, ashape[i]);
            }
        }
    }

    return Vector::Null;
}

template <typename T>
static uda::Array getArray(LOGMALLOCLIST* logmalloclist, NTREE* node, const std::string& target, int* shape, int rank)
{
    auto data = reinterpret_cast<T*>(udaGetNodeStructureComponentData(logmalloclist, node, (char*)target.c_str()));

    std::vector<uda::Dim> dims;
    for (int i = 0; i < rank; ++i) {
        std::vector<int> dim((size_t)shape[i]);
        for (int j = 0; j < shape[i]; ++j) {
            dim[j] = j;
        }
        dims.emplace_back(uda::Dim((uda::dim_type)i, dim.data(), (size_t)shape[i], "", ""));
    }

    return uda::Array(data, dims);
}

uda::Array uda::TreeNode::atomicArray(const std::string& target)
{
    LOGMALLOCLIST* logmalloclist = udaGetLogMallocList(handle_);
    NTREE* node = findNTreeStructureComponent(logmalloclist, node_, (char*)target.c_str());
    // NTREE * node = findNTreeStructureComponent(node_, (char *)target.c_str()); // Locate the named variable target
    if (node == nullptr) {
        return Array::Null;
    }

    int acount = udaGetNodeAtomicCount(node); // Number of atomic typed structure members
    if (acount == 0) {
        return Array::Null; // No atomic data
    }

    char** anames = udaGetNodeAtomicNames(logmalloclist, node);
    char** atypes = udaGetNodeAtomicTypes(logmalloclist, node);
    int* apoint = udaGetNodeAtomicPointers(logmalloclist, node);
    int* arank = udaGetNodeAtomicRank(logmalloclist, node);
    int** ashape = udaGetNodeAtomicShape(logmalloclist, node);

    if (anames == nullptr || atypes == nullptr || apoint == nullptr || arank == nullptr || ashape == nullptr) {
        return Array::Null;
    }

    for (int i = 0; i < acount; i++) {
        if (target == anames[i]) {
            if (std::string("STRING") == atypes[i]) {
                return getArray<char*>(logmalloclist, node, target, ashape[i], arank[i]);
            }
            if (std::string("short") == atypes[i]) {
                return getArray<short>(logmalloclist, node, target, ashape[i], arank[i]);
            }
            if (std::string("double") == atypes[i]) {
                return getArray<double>(logmalloclist, node, target, ashape[i], arank[i]);
            }
            if (std::string("float") == atypes[i]) {
                return getArray<float>(logmalloclist, node, target, ashape[i], arank[i]);
            }
            if (std::string("int") == atypes[i]) {
                return getArray<int>(logmalloclist, node, target, ashape[i], arank[i]);
            }
            if (std::string("unsigned int") == atypes[i]) {
                return getArray<unsigned int>(logmalloclist, node, target, ashape[i], arank[i]);
            }
            if (std::string("unsigned short") == atypes[i]) {
                return getArray<unsigned short>(logmalloclist, node, target, ashape[i], arank[i]);
            }
            if (std::string("unsigned char") == atypes[i]) {
                return getArray<unsigned char>(logmalloclist, node, target, ashape[i], arank[i]);
            }
        }
    }

    return Array::Null;
}

uda::StructData uda::TreeNode::structData(const std::string& target)
{
    LOGMALLOCLIST* logmalloclist = udaGetLogMallocList(handle_);
    NTREE* node = findNTreeStructureComponent(logmalloclist, node_, (char*)target.c_str());
    if (node == nullptr) {
        return StructData::Null;
    }

    int count = udaGetNodeChildrenCount(node->parent);

    uda::StructData data;

    for (int j = 0; j < count; j++) {
        void* ptr = udaGetNodeData(node->parent->children[j]);
        std::string name(udaGetNodeStructureType(node->parent->children[j]));
        auto size = static_cast<std::size_t>(udaGetNodeStructureSize(node->parent->children[j]));
        data.append(name, size, ptr);
    }

    return data;
}

void* uda::TreeNode::data()
{
    return udaGetNodeData(node_);
}
