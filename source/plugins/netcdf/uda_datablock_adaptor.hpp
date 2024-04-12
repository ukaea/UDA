#include "capnp_reader.hpp"
#include "serialisation/capnp_serialisation.h"
#include "netcdf_reader.hpp"

#include "clientserver/udaTypes.h"
#include "clientserver/udaStructs.h"
#include "clientserver/initStructs.h"

#include <cstring>

namespace uda::plugins::netcdf {

const static std::string signal_data_path = std::string("/") + CAPNP_REQUEST_NODE_NAME + "/" + CAPNP_DATA_NODE_NAME;
const static std::string signal_dim_path = std::string("/") + CAPNP_REQUEST_NODE_NAME + "/" + CAPNP_DIM_NODE_NAME;
const static std::string capnp_coordinate_path = std::string("/") + CAPNP_DIM_NODE_NAME;
const static std::string analysed_signal_label_path = std::string("/") + CAPNP_REQUEST_NODE_NAME + "/label";
const static std::string raw_signal_label_path = std::string("/") + CAPNP_REQUEST_NODE_NAME + "/title";
const static std::string signal_units_path = std::string("/") + CAPNP_REQUEST_NODE_NAME + "/units";

class DatablockAdaptor {
public:
    inline DatablockAdaptor(Buffer capnp_buffer) :
            _buffer(capnp_buffer),
            _capnp_interface(std::make_shared<UdaCapnpReader>(capnp_buffer))
    // _structured(structured)
    {}

    // inline void write_buffer_to_datablock(DATA_BLOCK* data_block)
    // {
    //     if (_structured) write_structured_data_to_datablock(data_block);
    //     else write_signal_data_to_datablock(data_block);
    // }
    inline void write_structured_data_to_data_block(DATA_BLOCK* data_block) const {
        initDataBlock(data_block);
        data_block->data_n = static_cast<int>(_buffer.size);
        data_block->data = _buffer.data;
        data_block->dims = nullptr;
        data_block->data_type = UDA_TYPE_CAPNP;
    }

    inline void write_signal_data_to_data_block(DATA_BLOCK* data_block) {
        initDataBlock(data_block);

        // UDA_LOG(UDA_LOG_DEBUG, "data block initialised.\n");
        NodeMetaData data_capnp_meta = _capnp_interface->get_node_metadata(signal_data_path);
        if (!data_capnp_meta.has_data()) {
            std::cout << "path: " << signal_data_path;
            throw std::runtime_error("expected data node for signal data in capnp buffer");
        }
        data_block->data_n = data_capnp_meta.size;
        data_block->data_type = data_capnp_meta.type;
        data_block->data = _capnp_interface->get_node_data(signal_data_path);

        UDA_LOG(UDA_LOG_DEBUG, "netcdf_reader: datablock writer, data len is : %d\n", data_block->data_n);
        UDA_LOG(UDA_LOG_DEBUG, "netcdf_reader: datablock writer, data type is : %d\n", data_block->data_type);
        if (data_block->data_type == UDA_TYPE_FLOAT)
            UDA_LOG(UDA_LOG_DEBUG, "netcdf_reader: datablock writer, data value 0 : %f\n",
                    ((float*) data_block->data)[0]);

        // signals in analysed files have a "label" attribute, in raw files this is "title" instead
        if (_capnp_interface->node_exists(analysed_signal_label_path)) {
            strcpy(data_block->data_label, _capnp_interface->get_node_data_string(analysed_signal_label_path).c_str());
        } else if (_capnp_interface->node_exists(raw_signal_label_path)) {
            strcpy(data_block->data_label, _capnp_interface->get_node_data_string(raw_signal_label_path).c_str());
        }
        strcpy(data_block->data_units, _capnp_interface->get_node_data_string(signal_units_path).c_str());

        UDA_LOG(UDA_LOG_DEBUG, "netcdf_reader: datablock writer, data LABEL is: %s\n", data_block->data_label);
        UDA_LOG(UDA_LOG_DEBUG, "netcdf_reader: datablock writer, data UNITS is : %s\n", data_block->data_units);

        size_t rank = data_capnp_meta.shape.size();
        data_block->rank = rank;
        UDA_LOG(UDA_LOG_DEBUG, "netcdf_reader: datablock writer, rank is : %d\n", rank);
        // !to do: meta (from top-level)
        if (!rank) return;

        NodeMetaData dim_capnp_meta = _capnp_interface->get_node_metadata(signal_dim_path);
        if (!dim_capnp_meta.has_children()) {
            std::cout << "path: " << signal_dim_path;
            throw std::runtime_error("expected parent node for signal dim in capnp buffer");
        }

        data_block->dims = (DIMS*) malloc(rank * sizeof(DIMS));
        for (unsigned int i = 0; i < rank; ++i) {
            auto dim = &data_block->dims[rank - 1 - i];
            initDimBlock(dim);
            dim->dim_n = data_capnp_meta.shape[i];
            UDA_LOG(UDA_LOG_DEBUG, "netcdf_reader: datablock writer, dim #%d len is : %d\n", i,
                    data_capnp_meta.shape[i]);
            // co-ordinate path always has leading slash
            std::string coordinate_path = capnp_coordinate_path + "/" + dim_capnp_meta.children[i];
            UDA_LOG(UDA_LOG_DEBUG, "netcdf_reader: datablock writer. dim %d co-ordinate path is %s \n", i,
                    coordinate_path.c_str());
            if (!_capnp_interface->node_exists(coordinate_path)) {
                UDA_LOG(UDA_LOG_DEBUG, "netcdf_reader: datablock writer. dim %d is arb units \n", i);
                dim->compressed = 1;
                dim->dim0 = 0.0;
                dim->diff = 1.0;
                dim->method = 0;
                dim->data_type = UDA_TYPE_UNSIGNED_INT;
                continue;
            } else { UDA_LOG(UDA_LOG_DEBUG, "netcdf_reader: datablock writer. dim %d is co-ordinate data \n", i); }
            std::string coordinate_data_path = coordinate_path + "/" + CAPNP_DATA_NODE_NAME;
            NodeMetaData coord_capnp_meta = _capnp_interface->get_node_metadata(coordinate_data_path);
            if (!data_capnp_meta.has_data()) {
                std::cout << "path: " << coordinate_data_path;
                throw std::runtime_error("expected data node for coord data in capnp buffer");
            }

            dim->data_type = coord_capnp_meta.type;
            UDA_LOG(UDA_LOG_DEBUG, "netcdf_reader: datablock writer. dim %d has datatype %d\n", i,
                    coord_capnp_meta.type);

            dim->dim = _capnp_interface->get_node_data(coordinate_path + "/" + CAPNP_DATA_NODE_NAME);
            if (dim->data_type == UDA_TYPE_FLOAT)
                UDA_LOG(UDA_LOG_DEBUG, "netcdf_reader: datablock writer. dim %d first value is %f\n", i,
                        ((float*) dim->dim)[0]);

            if (_capnp_interface->node_exists(coordinate_path + "/label")) {
                strcpy(dim->dim_label, _capnp_interface->get_node_data_string(coordinate_path + "/label").c_str());
            } else if (_capnp_interface->node_exists(coordinate_path + "/title")) {
                strcpy(dim->dim_label, _capnp_interface->get_node_data_string(coordinate_path + "/title").c_str());
            }
            if (_capnp_interface->node_exists(coordinate_path + "/units")) {
                strcpy(dim->dim_units, _capnp_interface->get_node_data_string(coordinate_path + "/units").c_str());
            }
            if (_capnp_interface->node_exists(coordinate_path + "/class") and
                _capnp_interface->get_node_data_string(coordinate_path + "/class") == "time") {
                data_block->order = i;
                UDA_LOG(UDA_LOG_DEBUG, "netcdf_reader: datablock writer, dim #%d is time data, setting order\n", i);
            }
        }
    }

private:
    Buffer _buffer;
    std::shared_ptr<UdaCapnpReader> _capnp_interface;
    // bool _structured;

};

} // namespace uda::plugins::netcdf
