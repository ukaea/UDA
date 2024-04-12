#pragma once
#include "capnp_reader.hpp"
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace uda::plugins::netcdf
{

    class CapnpJsonReader : public UdaCapnpReader
    {
        public:
        inline explicit CapnpJsonReader(TreeReader* tree)
            : UdaCapnpReader(tree)
        {}

        inline explicit CapnpJsonReader(Buffer capnp_buffer)
            : UdaCapnpReader(capnp_buffer)
        {}

        template<typename T>
        inline std::vector<T> get_node_data(std::string path)
        {
            NodeReader* node = get_node(path);
            return get_node_data<T>(node);
        }

        inline std::string get_node_data_string(std::string path)
        {
            NodeReader* node = get_node(path);
            return get_node_data_string(node);
        }

        void save_to_json(std::string filename);

        void save_to_json(NodeReader* node,std::string filename);

        void save_to_json(std::string node_name, std::string filename);

        inline void print_tree_reader()
        {
            uda_capnp_print_tree_reader(_tree);
        }

        protected:
        template<typename T>
        std::vector<T> get_node_data(NodeReader* node, NodeMetaData node_meta);

        template<typename T>
        inline std::vector<T> get_node_data(NodeReader* node)
        {
            auto node_meta = get_node_metadata(node);
            return get_node_data<T>(node, node_meta);
        }

        std::string get_node_data_string (NodeReader* node, NodeMetaData node_meta);

        inline std::string get_node_data_string (NodeReader* node)
        {
            auto node_meta = get_node_metadata(node);
            return get_node_data_string(node, node_meta);
        }

        void walk(NodeReader* capnp_node, TreeReader* tree, json& j_node);

        template<typename T>
        void data_to_json(NodeReader* node, NodeMetaData node_meta, json& json);

        template<>
        void data_to_json<std::string>(NodeReader* node, NodeMetaData meta, json& json);

        void string_data_to_json(NodeReader* node, NodeMetaData node_meta, json& json);

    };

    template std::vector<char> CapnpJsonReader::get_node_data<char>(NodeReader* node);
    template std::vector<short> CapnpJsonReader::get_node_data<short>(NodeReader* node);
    template std::vector<int> CapnpJsonReader::get_node_data<int>(NodeReader* node);
    template std::vector<long long> CapnpJsonReader::get_node_data<long long>(NodeReader* node);
    template std::vector<float> CapnpJsonReader::get_node_data<float>(NodeReader* node);
    template std::vector<double> CapnpJsonReader::get_node_data<double>(NodeReader* node);
    template std::vector<unsigned char> CapnpJsonReader::get_node_data<unsigned char>(NodeReader* node);
    template std::vector<unsigned short> CapnpJsonReader::get_node_data<unsigned short>(NodeReader* node);
    template std::vector<unsigned int> CapnpJsonReader::get_node_data<unsigned int>(NodeReader* node);
    template std::vector<unsigned long long> CapnpJsonReader::get_node_data<unsigned long long>(NodeReader* node);

    template std::vector<char> CapnpJsonReader::get_node_data<char>(std::string path);
    template std::vector<short> CapnpJsonReader::get_node_data<short>(std::string path);
    template std::vector<int> CapnpJsonReader::get_node_data<int>(std::string path);
    template std::vector<long long> CapnpJsonReader::get_node_data<long long>(std::string path);
    template std::vector<float> CapnpJsonReader::get_node_data<float>(std::string path);
    template std::vector<double> CapnpJsonReader::get_node_data<double>(std::string path);
    template std::vector<unsigned char> CapnpJsonReader::get_node_data<unsigned char>(std::string path);
    template std::vector<unsigned short> CapnpJsonReader::get_node_data<unsigned short>(std::string path);
    template std::vector<unsigned int> CapnpJsonReader::get_node_data<unsigned int>(std::string path);
    template std::vector<unsigned long long> CapnpJsonReader::get_node_data<unsigned long long>(std::string path);

    // template std::vector<int8_t> CapnpJsonReader::get_node_data<int8_t>(NodeReader* node);
    // template std::vector<int16_t> CapnpJsonReader::get_node_data<int16_t>(NodeReader* node);
    // template std::vector<int32_t> CapnpJsonReader::get_node_data<int32_t>(NodeReader* node);
    // template std::vector<int64_t> CapnpJsonReader::get_node_data<int64_t>(NodeReader* node);
    // template std::vector<uint8_t> CapnpJsonReader::get_node_data<uint8_t>(NodeReader* node);
    // template std::vector<uint16_t> CapnpJsonReader::get_node_data<uint16_t>(NodeReader* node);
    // template std::vector<uint32_t> CapnpJsonReader::get_node_data<uint32_t>(NodeReader* node);
    // template std::vector<uint64_t> CapnpJsonReader::get_node_data<uint64_t>(NodeReader* node);

    // template std::vector<int8_t> CapnpJsonReader::get_node_data<int8_t>(std::string path);
    // template std::vector<int16_t> CapnpJsonReader::get_node_data<int16_t>(std::string path);
    // template std::vector<int32_t> CapnpJsonReader::get_node_data<int32_t>(std::string path);
    // template std::vector<int64_t> CapnpJsonReader::get_node_data<int64_t>(std::string path);
    // template std::vector<uint8_t> CapnpJsonReader::get_node_data<uint8_t>(std::string path);
    // template std::vector<uint16_t> CapnpJsonReader::get_node_data<uint16_t>(std::string path);
    // template std::vector<uint32_t> CapnpJsonReader::get_node_data<uint32_t>(std::string path);
    // template std::vector<uint64_t> CapnpJsonReader::get_node_data<uint64_t>(std::string path);


} // namespace uda::plugins::netcdf
