#include "capnp_serialisation.h"

#include <iostream>
#include <capnp/message.h>
#include <capnp/serialize-packed.h>
#include <memory>
#include <vector>
#include <algorithm>
#include <cassert>

#include <clientserver/udaTypes.h>

#include "schema.capnp.h"

std::ostream& operator<<(std::ostream& out, const kj::ArrayPtr<::kj::byte> bytes)
{
    for (auto byte : bytes) {
        out << byte;
    }
    return out;
}

const char* to_string(::TreeNode::Type type)
{
    switch (type) {
        case ::TreeNode::Type::INT8: return "int8";
        case ::TreeNode::Type::INT16: return "int16";
        case ::TreeNode::Type::INT32: return "int32";
        case ::TreeNode::Type::INT64: return "int64";
        case ::TreeNode::Type::UINT8: return "uint8";
        case ::TreeNode::Type::UINT16: return "uint16";
        case ::TreeNode::Type::UINT32: return "uint32";
        case ::TreeNode::Type::UINT64: return "uint64";
        case ::TreeNode::Type::FLT32: return "flt32";
        case ::TreeNode::Type::FLT64: return "flt64";
        case ::TreeNode::Type::STRING: return "string";
        case ::TreeNode::Type::VOID: return "void";
    }
    return "";
}

std::ostream& operator<<(std::ostream& out, const typename capnp::List<uint64_t, capnp::Kind::PRIMITIVE>::Reader& list)
{
    const char* delim = "";
    int count = 0;
    for (auto el : list) {
        if (count == 10) {
            out << delim << "...";
            break;
        }
        out << delim << el;
        delim = ", ";
        ++count;
    }
    return out;
}

template <typename T>
std::ostream& operator<<(std::ostream& out, const std::vector<T>& vec)
{
    const char* delim = "";
    int count = 0;
    for (auto el : vec) {
        if (count == 10) {
            out << delim << "...";
            break;
        }
        out << delim << el;
        delim = ", ";
        ++count;
    }
    return out;
}

template <typename T>
void print_data(std::ostream& out, ::TreeNode::Array::Reader& array, const std::string& indent)
{
    auto len = array.getLen();
    if (len == 0) {
        auto ptr = reinterpret_cast<const T*>(array.getData().begin());
        out << indent << "  data: " << *ptr << "\n";
    } else {
        auto ptr = reinterpret_cast<const T*>(array.getData().begin());
        std::vector<T> vec(ptr, ptr + len);
        out << indent << "  data: [" << vec << "]\n";
    }
}

template <>
void print_data<std::string>(std::ostream& out, ::TreeNode::Array::Reader& array, const std::string& indent)
{
    auto len = array.getLen();
    auto ptr = reinterpret_cast<const char*>(array.getData().begin());
    std::string str(ptr, ptr + len);
    out << indent << "  data: [" << str << "]\n";
}

void print_node(std::ostream& out, const ::TreeNode::Reader& tree, const std::string& indent = "")
{
    out << indent << "{\n  "
        << indent << "name: " << tree.getName().cStr() << "\n";

    if (tree.hasChildren()) {
        for (auto child : tree.getChildren()) {
            print_node(out, child, indent + "  ");
            out << "\n";
        }
    } else if (!tree.isEmpty()) {
        auto array = tree.getArray();

        switch (array.getType()) {
            case ::TreeNode::Type::FLT32: print_data<float>(out, array, indent); break;
            case ::TreeNode::Type::FLT64: print_data<double>(out, array, indent); break;
            case ::TreeNode::Type::INT8: print_data<int8_t>(out, array, indent); break;
            case ::TreeNode::Type::INT16: print_data<int16_t>(out, array, indent); break;
            case ::TreeNode::Type::INT32: print_data<int32_t>(out, array, indent); break;
            case ::TreeNode::Type::INT64: print_data<int64_t>(out, array, indent); break;
            case ::TreeNode::Type::UINT8: print_data<uint8_t>(out, array, indent); break;
            case ::TreeNode::Type::UINT16: print_data<uint16_t>(out, array, indent); break;
            case ::TreeNode::Type::UINT32: print_data<uint32_t>(out, array, indent); break;
            case ::TreeNode::Type::UINT64: print_data<uint64_t>(out, array, indent); break;
            case ::TreeNode::Type::STRING: print_data<std::string>(out, array, indent); break;
            case ::TreeNode::Type::VOID: out << indent << "  data: <void>\n"; break;
        }

        out << indent << "  type: " << to_string(array.getType()) << "\n";
        out << indent << "  len: " << array.getLen() << "\n";
        out << indent << "  shape: [" << array.getShape() << "]\n";
    }

    out << indent << "}";
}


std::ostream& operator<<(std::ostream& out, const ::TreeNode::Reader& tree)
{
    print_node(out, tree);
    return out;
}

struct NodeReader {
    TreeNode::Reader node;
};

struct TreeReader {
    std::shared_ptr<capnp::PackedMessageReader> message_reader;
    NodeReader* root = nullptr;
    std::vector<std::unique_ptr<NodeReader>> nodes= {};
};

struct NodeBuilder {
    TreeNode::Builder node;
};

struct TreeBuilder {
    std::shared_ptr<capnp::MallocMessageBuilder> message_builder = {};
    NodeBuilder* root = nullptr;
    std::vector<std::unique_ptr<NodeBuilder>> nodes= {};
};

Buffer uda_capnp_serialise(TreeBuilder* tree)
{
    kj::VectorOutputStream out;
    capnp::writePackedMessage(out, *tree->message_builder);

    auto arr = out.getArray();
    char* buffer = (char*)malloc(arr.size());
    std::copy(arr.begin(), arr.end(), buffer);

    return Buffer{ buffer, arr.size() };
}

TreeReader* uda_capnp_deserialise(char* bytes, size_t size)
{
    kj::ArrayPtr<kj::byte> buffer(reinterpret_cast<kj::byte*>(bytes), size);
    kj::ArrayInputStream in(buffer);

    auto message_reader = std::make_shared<capnp::PackedMessageReader>(in);
    auto root = message_reader->getRoot<TreeNode>();
    auto tree = new TreeReader{ message_reader, nullptr };
    tree->nodes.emplace_back(std::make_unique<NodeReader>(NodeReader{ root }));
    tree->root = tree->nodes.back().get();

    return tree;
}

TreeBuilder* uda_capnp_new_tree()
{
    auto message_builder = std::make_shared<capnp::MallocMessageBuilder>();
    return new TreeBuilder{message_builder, {} };
}

void uda_capnp_free_tree_builder(TreeBuilder* tree)
{
    delete tree;
}

void uda_capnp_free_tree_reader(TreeReader* tree)
{
    delete tree;
}

NodeBuilder* uda_capnp_get_root(TreeBuilder* tree)
{
    if (tree->root == nullptr) {
        assert(tree->nodes.empty());
        auto node = tree->message_builder->initRoot<TreeNode>();
        tree->nodes.emplace_back(std::make_unique<NodeBuilder>(NodeBuilder{node }));
        tree->root = tree->nodes.back().get();
    }

    return tree->root;
}

void uda_capnp_set_node_name(NodeBuilder* node, const char* name)
{
    node->node.setName(name);
}

void uda_capnp_add_children(NodeBuilder* node, size_t num_children)
{
    if (node->node.isArray()) {
        return;
    }
    node->node.initChildren(num_children);
}

NodeBuilder* uda_capnp_get_child(TreeBuilder* tree, NodeBuilder* node, size_t index)
{
    if (!node->node.isChildren()) {
        return nullptr;
    }
    auto children = node->node.getChildren();
    if (index > children.size() - 1) {
        return nullptr;
    }
    auto child = children[index];
    tree->nodes.emplace_back(std::make_unique<NodeBuilder>(NodeBuilder{child }));
    return tree->nodes.back().get();
}

template <typename T>
struct TreeNodeTypeConverter {
    static TreeNode::Type type;
};

template <typename T> TreeNode::Type TreeNodeTypeConverter<T>::type = TreeNode::Type::VOID;
template <> TreeNode::Type TreeNodeTypeConverter<int8_t>::type = TreeNode::Type::INT8;
template <> TreeNode::Type TreeNodeTypeConverter<int16_t>::type = TreeNode::Type::INT16;
template <> TreeNode::Type TreeNodeTypeConverter<int32_t>::type = TreeNode::Type::INT32;
template <> TreeNode::Type TreeNodeTypeConverter<int64_t>::type = TreeNode::Type::INT64;
template <> TreeNode::Type TreeNodeTypeConverter<uint8_t>::type = TreeNode::Type::UINT8;
template <> TreeNode::Type TreeNodeTypeConverter<uint16_t>::type = TreeNode::Type::UINT16;
template <> TreeNode::Type TreeNodeTypeConverter<uint32_t>::type = TreeNode::Type::UINT32;
template <> TreeNode::Type TreeNodeTypeConverter<uint64_t>::type = TreeNode::Type::UINT64;
template <> TreeNode::Type TreeNodeTypeConverter<float>::type = TreeNode::Type::FLT32;
template <> TreeNode::Type TreeNodeTypeConverter<double>::type = TreeNode::Type::FLT64;

template <typename T>
void uda_capnp_add_array(NodeBuilder* node, T* data, size_t size)
{
    assert(!node->node.isChildren());
    auto array = node->node.initArray();
    array.setType(TreeNodeTypeConverter<T>::type);
    array.setLen(size);
    auto shape = array.initShape(1);
    shape.set(0, size);
    kj::ArrayPtr<kj::byte> ptr(reinterpret_cast<kj::byte*>(data), size * sizeof(T));
    array.setData(ptr);
}

void uda_capnp_add_array_f32(NodeBuilder* node, float* data, size_t size)
{
    return uda_capnp_add_array(node, data, size);
}

void uda_capnp_add_array_f64(NodeBuilder* node, double* data, size_t size)
{
    return uda_capnp_add_array(node, data, size);
}

void uda_capnp_add_array_i8(NodeBuilder* node, int8_t* data, size_t size)
{
    return uda_capnp_add_array(node, data, size);
}

void uda_capnp_add_array_i16(NodeBuilder* node, int16_t* data, size_t size)
{
    return uda_capnp_add_array(node, data, size);
}

void uda_capnp_add_array_i32(NodeBuilder* node, int32_t* data, size_t size)
{
    return uda_capnp_add_array(node, data, size);
}

void uda_capnp_add_array_i64(NodeBuilder* node, int64_t* data, size_t size)
{
    return uda_capnp_add_array(node, data, size);
}

void uda_capnp_add_array_u8(NodeBuilder* node, uint8_t* data, size_t size)
{
    return uda_capnp_add_array(node, data, size);
}

void uda_capnp_add_array_u16(NodeBuilder* node, uint16_t* data, size_t size)
{
    return uda_capnp_add_array(node, data, size);
}

void uda_capnp_add_array_u32(NodeBuilder* node, uint32_t* data, size_t size)
{
    return uda_capnp_add_array(node, data, size);
}

void uda_capnp_add_array_u64(NodeBuilder* node, uint64_t* data, size_t size)
{
    return uda_capnp_add_array(node, data, size);
}

template <typename T>
void uda_capnp_add_scalar(NodeBuilder* node, T data)
{
    assert(!node->node.isChildren());
    auto array = node->node.initArray();
    array.setType(TreeNodeTypeConverter<T>::type);
    array.setLen(0);
    array.initShape(0);
    kj::ArrayPtr<kj::byte> ptr(reinterpret_cast<kj::byte*>(&data), sizeof(T));
    array.setData(ptr);
}

void uda_capnp_add_f32(NodeBuilder* node, float data)
{
    uda_capnp_add_scalar(node, data);
}

void uda_capnp_add_f64(NodeBuilder* node, double data)
{
    uda_capnp_add_scalar(node, data);
}

void uda_capnp_add_i8(NodeBuilder* node, int8_t data)
{
    uda_capnp_add_scalar(node, data);
}

void uda_capnp_add_i16(NodeBuilder* node, int16_t data)
{
    uda_capnp_add_scalar(node, data);
}

void uda_capnp_add_i32(NodeBuilder* node, int32_t data)
{
    uda_capnp_add_scalar(node, data);
}

void uda_capnp_add_i64(NodeBuilder* node, int64_t data)
{
    uda_capnp_add_scalar(node, data);
}

void uda_capnp_add_u8(NodeBuilder* node, uint8_t data)
{
    uda_capnp_add_scalar(node, data);
}

void uda_capnp_add_u16(NodeBuilder* node, uint16_t data)
{
    uda_capnp_add_scalar(node, data);
}

void uda_capnp_add_u32(NodeBuilder* node, uint32_t data)
{
    uda_capnp_add_scalar(node, data);
}

void uda_capnp_add_u64(NodeBuilder* node, uint64_t data)
{
    uda_capnp_add_scalar(node, data);
}

void uda_capnp_print_tree_builder(TreeBuilder* tree)
{
    std::cout << tree->root->node << std::endl;
}

void uda_capnp_print_tree_reader(TreeReader* tree)
{
    std::cout << tree->root->node << std::endl;
}

NodeReader* uda_capnp_read_root(TreeReader* tree)
{
    return tree->root;
}

size_t uda_capnp_num_children(NodeReader* node)
{
    if (node->node.hasChildren()) {
        return node->node.getChildren().size();
    }
    return 0;
}

NodeReader* uda_capnp_read_child(TreeReader* tree, NodeReader* node, const char* name)
{
    for (auto child : node->node.getChildren()) {
        std::string s_name = child.getName().cStr();
        if (s_name == name) {
            tree->nodes.emplace_back(std::make_unique<NodeReader>(NodeReader{ child }));
            return tree->nodes.back().get();
        }
    }
    return nullptr;
}

NodeReader* uda_capnp_read_child_n(TreeReader* tree, NodeReader* node, size_t index)
{
    auto children = node->node.getChildren();
    if (index >= children.size()) {
        return nullptr;
    }
    auto child = children[index];
    tree->nodes.emplace_back(std::make_unique<NodeReader>(NodeReader{ child }));
    return tree->nodes.back().get();
}

const char* uda_capnp_read_name(NodeReader* node)
{
    return node->node.getName().cStr();
}

int uda_capnp_read_type(NodeReader* node)
{
    if (!node->node.isArray()) {
        return UDA_TYPE_UNKNOWN;
    }
    auto type = node->node.getArray().getType();
    switch (type) {
        case TreeNode::Type::INT8: return UDA_TYPE_CHAR;
        case TreeNode::Type::INT16: return UDA_TYPE_SHORT;
        case TreeNode::Type::INT32: return UDA_TYPE_INT;
        case TreeNode::Type::INT64: return UDA_TYPE_LONG64;
        case TreeNode::Type::UINT8: return UDA_TYPE_UNSIGNED_CHAR;
        case TreeNode::Type::UINT16: return UDA_TYPE_UNSIGNED_SHORT;
        case TreeNode::Type::UINT32: return UDA_TYPE_UNSIGNED_INT;
        case TreeNode::Type::UINT64: return UDA_TYPE_UNSIGNED_LONG64;
        case TreeNode::Type::FLT32: return UDA_TYPE_FLOAT;
        case TreeNode::Type::FLT64: return UDA_TYPE_DOUBLE;
        case TreeNode::Type::STRING: return UDA_TYPE_STRING;
        default: return UDA_TYPE_UNKNOWN;
    }
}

Optional_Size_t uda_capnp_read_rank(NodeReader* node)
{
    if (!node->node.isArray()) {
        return { false, 0 };
    }
    auto array = node->node.getArray();
    return { true, array.getShape().size() };
}

bool uda_capnp_read_shape(NodeReader* node, size_t* shape)
{
    if (!node->node.isArray()) {
        return false;
    }
    auto array = node->node.getArray();
    auto shape_array = array.getShape();
    std::copy(shape_array.begin(), shape_array.end(), shape);
    return true;
}

bool uda_capnp_read_data(NodeReader* node, char* ptr)
{
    if (!node->node.isArray()) {
        return false;
    }
    auto array = node->node.getArray();
    auto data = array.getData();
    std::copy(data.begin(), data.end(), ptr);
    return true;
}