#include "capnp_reader.hpp"
#include <algorithm>
#include <cstring>
#include "clientserver/udaTypes.h"

namespace uda::plugins::netcdf {

size_t size_of_datatype(unsigned int uda_type) {
    switch (uda_type) {
        case UDA_TYPE_CHAR:
            return sizeof(char);
        case UDA_TYPE_SHORT:
            return sizeof(int16_t);
        case UDA_TYPE_INT:
            return sizeof(int32_t);
        case UDA_TYPE_LONG64:
            return sizeof(int64_t);
        case UDA_TYPE_UNSIGNED_CHAR:
            return sizeof(unsigned char);
        case UDA_TYPE_UNSIGNED_SHORT:
            return sizeof(uint16_t);
        case UDA_TYPE_UNSIGNED_INT:
            return sizeof(uint32_t);
        case UDA_TYPE_UNSIGNED_LONG64:
            return sizeof(uint64_t);
        case UDA_TYPE_FLOAT:
            return sizeof(float);
        case UDA_TYPE_DOUBLE:
            return sizeof(double);
        case UDA_TYPE_STRING:
            return sizeof(char);
        default:
            throw std::runtime_error("unknown uda data type requested for bytes_size method");
    }
}

size_t bytes_size(unsigned int uda_type, size_t array_len) {
    return size_of_datatype(uda_type) * array_len;
}

void UdaCapnpReader::walk(NodeReader* node, TreeReader* tree, const std::string& path) {
    _path_to_node_map[path] = node;

    Optional_Size_t maybe_rank = uda_capnp_read_rank(node);
    bool has_data = maybe_rank.has_value;

    // nodes can either have children or data (or neither), not both
    if (!has_data) {
        size_t n_children = uda_capnp_num_children(node);
        for (size_t i = 0; i < n_children; ++i) {
            auto child_node = uda_capnp_read_child_n(tree, node, i);
            auto name = uda_capnp_read_name(child_node);
            std::string child_path = (path == "/") ? (path + name) : path + "/" + name;
            walk(child_node, tree, child_path);
        }

        if (n_children == 0) {} // void node, do nothing ;
    }
}

NodeReader* UdaCapnpReader::get_node(const std::string& path) {
    try {
        return _path_to_node_map.at(path);
    }
    catch (const std::out_of_range& e) {
        std::string err = "Requested path " + path + " does not exist";
        throw std::runtime_error(err);
    }
}

NodeMetaData UdaCapnpReader::get_node_metadata(NodeReader* node) {
    NodeMetaData result;
    result.name = uda_capnp_read_name(node);

    Optional_Size_t maybe_rank = uda_capnp_read_rank(node);

    if (maybe_rank.has_value) {
        result.node_type = NodeType::DATA;

        size_t rank = maybe_rank.value;
        std::vector<size_t> shape(rank);
        uda_capnp_read_shape(node, shape.data());
        result.shape = shape;

        result.size = std::accumulate(shape.begin(), shape.end(), 1, std::multiplies<>());

        result.type = uda_capnp_read_type(node);

        result.num_slices = uda_capnp_read_num_slices(node);
        result.eos = uda_capnp_read_is_eos(node);

        result.size_of_datatype = size_of_datatype(result.type);

    } else {
        size_t n_children = uda_capnp_num_children(node);
        for (size_t i = 0; i < n_children; ++i) {
            auto child_node = uda_capnp_read_child_n(_tree, node, i);
            auto name = uda_capnp_read_name(child_node);
            result.children.emplace_back(name);
        }

        if (n_children > 0) result.node_type = NodeType::PARENT;
        else result.node_type = NodeType::VOID;
    }
    return result;
}

size_t UdaCapnpReader::data_size(NodeReader* node) {
    Optional_Size_t maybe_rank = uda_capnp_read_rank(node);

    if (maybe_rank.has_value) {
        size_t rank = maybe_rank.value;
        std::vector<size_t> shape(rank);
        uda_capnp_read_shape(node, shape.data());

        return std::accumulate(shape.begin(), shape.end(), 1, std::multiplies<>());
    }

    return 0;
}

// template<typename T>
// std::vector<T> UdaCapnpReader::get_node_data(NodeReader* node, NodeMetaData node_meta)
// {
//     if (!node_meta.eos)
//     {
//         throw std::runtime_error("UDA does not currently handle streamed capnp data");  // TODO think about this
//     }
//     T data[node_meta.size];
//     auto buffer = reinterpret_cast<char*>(data);
//     size_t offset = 0;
//     for (size_t i=0; i<node_meta.num_slices; ++i)
//     {
//         size_t slice_size = uda_capnp_read_slice_size(node, i);
//         uda_capnp_read_data(node, i, buffer + offset);
//         offset += slice_size;
//     }
//
//     if (offset != node_meta.size * sizeof(T))
//     {
//         throw std::runtime_error("Sum of slice sizes not equal to provided data count");
//     }
//     return std::vector<T> (data, data + node_meta.size);
// }

// template<>
// std::vector<std::string> UdaCapnpReader::get_node_data(NodeReader* node, NodeMetaData node_meta)
// {
//     if (!node_meta.eos)
//     {
//         throw std::runtime_error("UDA does not currently handle streamed capnp data");
//     }
//     char data[node_meta.size];
//     size_t offset = 0;
//     for (size_t i=0; i<node_meta.num_slices; ++i)
//     {
//         size_t slice_size = uda_capnp_read_slice_size(node, i);
//         uda_capnp_read_data(node, i, data + offset);
//         offset += slice_size;
//     }
//
//     if (offset != node_meta.size * sizeof(char))
//     {
//         throw std::runtime_error("Sum of slice sizes not equal to provided data count");
//     }
//     // TODO look at this for deserialising array strings
//     std::vector<std::string> result;
//     char* ptr = &data[0];
//     for (unsigned int i = 0; i< node_meta.shape[0]; ++i)
//     {
//         std::string entry(ptr);
//         result.emplace_back(entry.c_str());
//         ptr += strlen(ptr) + 1;
//     }
//
//     return result;
// }

std::string UdaCapnpReader::get_node_data_string(NodeReader* node, const NodeMetaData& node_meta) {
    if (!node_meta.eos) {
        throw std::runtime_error("UDA does not currently handle streamed capnp data");
    }
    std::vector<char> data(node_meta.size);
    size_t offset = 0;
    for (size_t i = 0; i < node_meta.num_slices; ++i) {
        size_t slice_size = uda_capnp_read_slice_size(node, i);
        uda_capnp_read_data(node, i, data.data() + offset);
        offset += slice_size;
    }

    if (offset != node_meta.size * sizeof(char)) {
        throw std::runtime_error("Sum of slice sizes not equal to provided data count");
    }
    return {data.data(), node_meta.size};
}

char* UdaCapnpReader::get_node_data(NodeReader* node, const NodeMetaData& node_meta) {
    if (!node_meta.eos) {
        throw std::runtime_error("UDA does not currently handle streamed capnp data");
    }
    size_t data_type_size = size_of_datatype(node_meta.type);
    char* data = (char*) malloc(data_type_size * node_meta.size);

    size_t offset = 0;
    for (size_t i = 0; i < node_meta.num_slices; ++i) {
        size_t slice_size = uda_capnp_read_slice_size(node, i);
        uda_capnp_read_data(node, i, data + offset);
        offset += slice_size * data_type_size;
    }

    // if (offset != node_meta.size * sizeof(char))
    // {
    //     throw std::runtime_error("Sum of slice sizes not equal to provided data count");
    // }
    return data;
}

}
