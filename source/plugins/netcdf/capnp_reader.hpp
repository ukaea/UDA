#pragma once

#include <string>
#include <vector>
#include <fstream>
#include <iostream>
#include "serialisation/capnp_serialisation.h"
#include <random>
#include <type_traits>
#include <map>
#include <numeric>

namespace uda::plugins::netcdf {

enum class NodeType : uint {
    VOID,
    DATA,
    PARENT
};

struct NodeMetaData {
    std::string name;
    std::vector<std::string> children;
    unsigned int type;
    size_t size_of_datatype;
    size_t size;
    std::vector<size_t> shape;
    size_t num_slices;
    bool eos;

    NodeType node_type;

    [[nodiscard]] inline bool has_data() const { return node_type == NodeType::DATA; }

    [[nodiscard]] inline bool is_void() const { return node_type == NodeType::VOID; }

    [[nodiscard]] inline bool has_children() const { return node_type == NodeType::PARENT; }
};

class UdaCapnpReader {
public:
    inline explicit UdaCapnpReader(TreeReader* tree) : _tree(tree) {
        _root = uda_capnp_read_root(_tree);
        // populate path->node map
        walk(_root, _tree, "/");
    }

    inline explicit UdaCapnpReader(Buffer capnp_buffer) :
            _tree(uda_capnp_deserialise(capnp_buffer.data, capnp_buffer.size)) {
        _root = uda_capnp_read_root(_tree);
        walk(_root, _tree, "/");
    }

    char* get_node_data(const std::string& path) {
        NodeReader* node = get_node(path);
        auto node_meta = get_node_metadata(node);
        return get_node_data(node, node_meta);
    }

    // template<typename T>
    // inline std::vector<T> get_node_data(std::string path)
    // {
    //     NodeReader* node = get_node(path);
    //     return get_node_data<T>(node);
    // }

    inline std::string get_node_data_string(const std::string& path) {
        NodeReader* node = get_node(path);
        return get_node_data_string(node);
    }


    inline NodeMetaData get_node_metadata(const std::string& path) {
        NodeReader* node = get_node(path);
        return get_node_metadata(node);
    }

    inline std::vector<std::string> get_all_node_paths() {
        std::vector<std::string> results;
        results.reserve(_path_to_node_map.size());
        for (const auto& [path, node_ptr]: _path_to_node_map) {
            results.emplace_back(path);
        }
        return results;
    }

    inline bool node_exists(const std::string& path) {
        return _path_to_node_map.find(path) != _path_to_node_map.end();
    }

protected:
    void walk(NodeReader* node, TreeReader* tree, const std::string& path);

    size_t data_size(NodeReader* node);

    NodeReader* get_node(const std::string& path);

    NodeMetaData get_node_metadata(NodeReader* node);

    // template<typename T>
    // std::vector<T> get_node_data(NodeReader* node, NodeMetaData node_meta);
    //
    // template<typename T>
    // inline std::vector<T> get_node_data(NodeReader* node)
    // {
    //     auto node_meta = get_node_metadata(node);
    //     return get_node_data<T>(node, node_meta);
    // }

    std::string get_node_data_string(NodeReader* node, const NodeMetaData& node_meta);

    inline std::string get_node_data_string(NodeReader* node) {
        auto node_meta = get_node_metadata(node);
        return get_node_data_string(node, node_meta);
    }

    char* get_node_data(NodeReader* node, const NodeMetaData& node_meta);

    TreeReader* _tree;
    NodeReader* _root;
    std::map<std::string, NodeReader*> _path_to_node_map;
};

// template std::vector<signed char> UdaCapnpReader::get_node_data<signed char>(NodeReader* node);
// template std::vector<char> UdaCapnpReader::get_node_data<char>(NodeReader* node);
// template std::vector<int16_t> UdaCapnpReader::get_node_data<int16_t>(NodeReader* node);
// template std::vector<int32_t> UdaCapnpReader::get_node_data<int32_t>(NodeReader* node);
// template std::vector<int64_t> UdaCapnpReader::get_node_data<int64_t>(NodeReader* node);
// template std::vector<float> UdaCapnpReader::get_node_data<float>(NodeReader* node);
// template std::vector<double> UdaCapnpReader::get_node_data<double>(NodeReader* node);
// template std::vector<unsigned char> UdaCapnpReader::get_node_data<unsigned char>(NodeReader* node);
// template std::vector<uint16_t> UdaCapnpReader::get_node_data<uint16_t>(NodeReader* node);
// template std::vector<uint32_t> UdaCapnpReader::get_node_data<uint32_t>(NodeReader* node);
// template std::vector<uint64_t> UdaCapnpReader::get_node_data<uint64_t>(NodeReader* node);
//
// template std::vector<char> UdaCapnpReader::get_node_data<char>(std::string path);
// template std::vector<int16_t> UdaCapnpReader::get_node_data<int16_t>(std::string path);
// template std::vector<int32_t> UdaCapnpReader::get_node_data<int32_t>(std::string path);
// template std::vector<int64_t> UdaCapnpReader::get_node_data<int64_t>(std::string path);
// template std::vector<float> UdaCapnpReader::get_node_data<float>(std::string path);
// template std::vector<double> UdaCapnpReader::get_node_data<double>(std::string path);
// template std::vector<unsigned char> UdaCapnpReader::get_node_data<unsigned char>(std::string path);
// template std::vector<uint16_t> UdaCapnpReader::get_node_data<uint16_t>(std::string path);
// template std::vector<uint32_t> UdaCapnpReader::get_node_data<uint32_t>(std::string path);
// template std::vector<uint64_t> UdaCapnpReader::get_node_data<uint64_t>(std::string path);
}
