#include "capnp_json_reader.hpp"

namespace uda::plugins::netcdf
{
    template<typename T>
    std::vector<T> CapnpJsonReader::get_node_data(NodeReader* node, NodeMetaData node_meta)
    {
        if (!node_meta.eos)
        {
            throw std::runtime_error("UDA does not currently handle streamed capnp data");
        }
        T data[node_meta.size];
        auto buffer = reinterpret_cast<char*>(data);
        size_t offset = 0;
        for (size_t i=0; i<node_meta.num_slices; ++i)
        {
            size_t slice_size = uda_capnp_read_slice_size(node, i);
            uda_capnp_read_data(node, i, buffer + offset);
            offset += slice_size;
        }

        if (offset != node_meta.size * sizeof(T))
        {
            throw std::runtime_error("Sum of slice sizes not equal to provided data count");
        }
        return std::vector<T> (data, data + node_meta.size);
    }

    template<>
    std::vector<std::string> CapnpJsonReader::get_node_data(NodeReader* node, NodeMetaData node_meta)
    {
        if (!node_meta.eos)
        {
            throw std::runtime_error("UDA does not currently handle streamed capnp data");
        }
        char data[node_meta.size];
        // auto buffer = reinterpret_cast<char*>(data);
        size_t offset = 0;
        for (size_t i=0; i<node_meta.num_slices; ++i)
        {
            size_t slice_size = uda_capnp_read_slice_size(node, i);
            uda_capnp_read_data(node, i, data + offset);
            offset += slice_size;
        }

        if (offset != node_meta.size * sizeof(char))
        {
            throw std::runtime_error("Sum of slice sizes not equal to provided data count");
        }

        std::vector<std::string> result;
        char* ptr = &data[0];
        for (unsigned int i = 0; i< node_meta.shape[0]; ++i)
        {
            std::string entry(ptr);
            result.emplace_back(entry.c_str());
            ptr += strlen(ptr) + 1;
        }
    
        return result;
    }

    std::string CapnpJsonReader::get_node_data_string (NodeReader* node, NodeMetaData node_meta)
    {
        if (!node_meta.eos)
        {
            throw std::runtime_error("UDA does not currently handle streamed capnp data");
        }
        char data[node_meta.size];
        size_t offset = 0;
        for (size_t i=0; i<node_meta.num_slices; ++i)
        {
            size_t slice_size = uda_capnp_read_slice_size(node, i);
            uda_capnp_read_data(node, i, data + offset);
            offset += slice_size;
        }

        if (offset != node_meta.size * sizeof(char))
        {
            throw std::runtime_error("Sum of slice sizes not equal to provided data count");
        }
        return std::string(data, node_meta.size);
    }

    typedef enum UdaType 
    {
        UDA_TYPE_UNKNOWN = 0,
        UDA_TYPE_CHAR = 1,
        UDA_TYPE_SHORT = 2,
        UDA_TYPE_INT = 3,
        UDA_TYPE_UNSIGNED_INT = 4,
        UDA_TYPE_LONG = 5,
        UDA_TYPE_FLOAT = 6,
        UDA_TYPE_DOUBLE = 7,
        UDA_TYPE_UNSIGNED_CHAR = 8,
        UDA_TYPE_UNSIGNED_SHORT = 9,
        UDA_TYPE_UNSIGNED_LONG = 10,
        UDA_TYPE_LONG64 = 11,
        UDA_TYPE_UNSIGNED_LONG64 = 12,
        UDA_TYPE_COMPLEX = 13,
        UDA_TYPE_DCOMPLEX = 14,
        UDA_TYPE_UNDEFINED = 15,
        UDA_TYPE_VLEN = 16,
        UDA_TYPE_STRING = 17,
        UDA_TYPE_COMPOUND = 18,
        UDA_TYPE_OPAQUE = 19,
        UDA_TYPE_ENUM = 20,
        UDA_TYPE_VOID  = 21,
        UDA_TYPE_CAPNP = 22,
        UDA_TYPE_STRING2 = 99
    } UDA_TYPE;

    std::string uda_type_to_string(int type)
    {
        switch (type)
        {
            case UDA_TYPE_CHAR: return "i8";
            case UDA_TYPE_SHORT: return "i16";
            case UDA_TYPE_INT: return "i32";
            case UDA_TYPE_LONG64: return "i64";
            case UDA_TYPE_UNSIGNED_CHAR: return "u8";
            case UDA_TYPE_UNSIGNED_SHORT: return "u16";
            case UDA_TYPE_UNSIGNED_INT: return "u32";
            case UDA_TYPE_UNSIGNED_LONG64: return "u64";
            case UDA_TYPE_FLOAT: return "f32";
            case UDA_TYPE_DOUBLE: return "f64";
            case UDA_TYPE_STRING: return "string";
            default: return "";
        }
    }

    void CapnpJsonReader::walk(NodeReader* capnp_node, TreeReader* tree, json& j_node)
    {
        auto node_meta = get_node_metadata(capnp_node);

        if (node_meta.has_data())
        {
            j_node["shape"] = node_meta.shape;
            j_node["size"] = node_meta.size;

            auto data_type = uda_capnp_read_type(capnp_node);
            j_node["type"] = uda_type_to_string(data_type);

            // std::cout << uda_capnp_read_name(capnp_node) << ": " << j_node["type"] << std::endl;

            switch (data_type)
            {
                case UDA_TYPE_CHAR: data_to_json<int8_t>(capnp_node, node_meta, j_node); break;
                case UDA_TYPE_SHORT: data_to_json<int16_t>(capnp_node, node_meta, j_node); break;
                case UDA_TYPE_INT: data_to_json<int32_t>(capnp_node, node_meta, j_node); break;
                case UDA_TYPE_LONG64: data_to_json<int64_t>(capnp_node, node_meta, j_node); break;
                case UDA_TYPE_UNSIGNED_CHAR: data_to_json<uint8_t>(capnp_node, node_meta, j_node); break;
                case UDA_TYPE_UNSIGNED_SHORT: data_to_json<uint16_t>(capnp_node, node_meta, j_node); break;
                case UDA_TYPE_UNSIGNED_INT: data_to_json<uint32_t>(capnp_node, node_meta, j_node); break;
                case UDA_TYPE_UNSIGNED_LONG64: data_to_json<uint64_t>(capnp_node, node_meta, j_node); break;
                case UDA_TYPE_FLOAT: data_to_json<float>(capnp_node, node_meta, j_node); break;
                case UDA_TYPE_DOUBLE: data_to_json<double>(capnp_node, node_meta, j_node); break;
                case UDA_TYPE_STRING: 
                {
                    if (node_meta.shape.size() == 2) data_to_json<std::string>(capnp_node, node_meta, j_node);
                    else string_data_to_json(capnp_node, node_meta, j_node);
                    break;
                }
                // case ::TreeNode::Type::VOID: out << indent << "  data: <void>\n"; break;
                default: j_node["data"] = "UDA type unknown";
            }
        }
        else
        {
            // try
            // {
            //     // should fail
            //     auto data_type = uda_capnp_read_type(capnp_node);
            //     j_node = "node has no data but has type";
            //     return;
            // }
            // catch(const std::exception& e){}
            

            // std::cout << uda_capnp_read_name(capnp_node) << ": no data" << std::endl;
            try
            {          
                size_t n_children = uda_capnp_num_children(capnp_node);
                for (size_t i=0; i<n_children; ++i)
                {
                    auto child_node = uda_capnp_read_child_n(tree, capnp_node, i);
                    auto name = uda_capnp_read_name(child_node);
                    walk(child_node, tree, j_node[name]);
                }
                if (n_children == 0) j_node = "null";
            }
            catch(const std::exception& e)
            {
                j_node = "capnp parsing error";
                std::cout << "error parsing non-data node" << std::endl;
                std::cout << e.what() << std::endl;
            }
        }
    }

    template<typename T>
    void CapnpJsonReader::data_to_json(NodeReader* node, NodeMetaData meta, json& json)
    {
        if (meta.size > 0)
        {
            std::vector<T> data_vec = get_node_data<T>(node, meta);
            try
            {
                json["data"] = data_vec;
            }
            catch(const nlohmann::json::exception& e)
            {
                std::cout << "data: ";
                for (const auto& val: data_vec) std::cout << val << ", ";
                std::cout << std::endl;
                json["data"] = std::string(e.what());
            }
        }
        else
        {
            T data;
            uda_capnp_read_data(node, 0, (char*) &data);
            json["data"] = data;
        }
    }

    template<>
    void CapnpJsonReader::data_to_json<std::string>(NodeReader* node, NodeMetaData meta, json& json)
    {
        if (meta.shape.size() == 2)
        {
            std::vector<std::string> data_vec = get_node_data<std::string>(node, meta);
            try
            {
                json["data"] = data_vec;
            }
            catch(const nlohmann::json::exception& e)
            {
                std::cout << "data: ";
                for (const auto& val: data_vec) std::cout << val << ", ";
                std::cout << std::endl;
                json["data"] = e.what();
            }
        }
        else
        {
            string_data_to_json(node, meta, json);
        }
    }

    void CapnpJsonReader::string_data_to_json(NodeReader* node, NodeMetaData meta, json& json)
    {
        std::string  data_vec = get_node_data_string(node, meta);
        try
        {
            // sometimes weird things happening with string conversions (e.g. \u0000 or \0 appended to std::string representation)
            // could this be capnp slicing with strings being weird?

            // std::cout << uda_capnp_read_name(node) << ": " << data_vec <<std::endl;
            json["data"] = data_vec.c_str();
        }
        catch(const nlohmann::json::exception& e)
        {
            std::cout << "data: " << data_vec << std::endl;
            json["data"] = e.what();
        }
    }

    void CapnpJsonReader::save_to_json(NodeReader* node, std::string filename)
    {
        json j;
        walk(node, _tree, j);
        std::ofstream file(filename);
        // file << std::setw(4) << j.dump(4, ' ', false, json::error_handler_t::ignore) << std::endl;
        file << std::setw(4) << j << std::endl;
    }

    void CapnpJsonReader::save_to_json(std::string filename)
    {
        save_to_json(_root, filename);
    }

    void CapnpJsonReader::save_to_json(std::string node_name, std::string filename)
    {
        auto node = get_node(node_name);
        save_to_json(node, filename);
    }

} // namespace uda::plugins::netcdf
