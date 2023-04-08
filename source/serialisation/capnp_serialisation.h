#pragma once

#ifndef UDA_CLIENTSERVER_CAPNP_SERIALISATION_H
#define UDA_CLIENTSERVER_CAPNP_SERIALISATION_H

#include <cstdlib>
#include <cstdint>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Buffer {
    char* data;
    size_t size;
} BUFFER;

typedef struct Optional_Size_t {
    bool has_value;
    size_t value;
} OPTIONAL_SIZE_T;

struct TreeBuilder;
struct NodeBuilder;

struct TreeReader;
struct NodeReader;

BUFFER uda_capnp_serialise(TreeBuilder* tree);
TreeReader* uda_capnp_deserialise(const char* bytes, size_t size);

NodeReader* uda_capnp_read_root(TreeReader* tree);
size_t uda_capnp_num_children(NodeReader* node);
NodeReader* uda_capnp_read_child(TreeReader* tree, NodeReader* node, const char* name);
NodeReader* uda_capnp_read_child_n(TreeReader* tree, NodeReader* node, size_t index);
const char* uda_capnp_read_name(NodeReader* node);
int uda_capnp_read_type(NodeReader* node);
OPTIONAL_SIZE_T uda_capnp_read_rank(NodeReader* node);
bool uda_capnp_read_shape(NodeReader* node, size_t* shape);
bool uda_capnp_read_data(NodeReader* node, char* data);

TreeBuilder* uda_capnp_new_tree();
void uda_capnp_free_tree_builder(TreeBuilder* tree);
void uda_capnp_free_tree_reader(TreeReader* tree);

NodeBuilder* uda_capnp_get_root(TreeBuilder* tree);
void uda_capnp_set_node_name(NodeBuilder* node, const char* name);

void uda_capnp_add_children(NodeBuilder* node, size_t num_children);
NodeBuilder* uda_capnp_get_child(TreeBuilder* tree, NodeBuilder* node, size_t index);

void uda_capnp_add_array_f32(NodeBuilder* node, const float* data, size_t size);
void uda_capnp_add_array_f64(NodeBuilder* node, const double* data, size_t size);
void uda_capnp_add_array_i8(NodeBuilder* node, const int8_t* data, size_t size);
void uda_capnp_add_array_i16(NodeBuilder* node, const int16_t* data, size_t size);
void uda_capnp_add_array_i32(NodeBuilder* node, const int32_t* data, size_t size);
void uda_capnp_add_array_i64(NodeBuilder* node, const int64_t* data, size_t size);
void uda_capnp_add_array_u8(NodeBuilder* node, const uint8_t* data, size_t size);
void uda_capnp_add_array_u16(NodeBuilder* node, const uint16_t* data, size_t size);
void uda_capnp_add_array_u32(NodeBuilder* node, const uint32_t* data, size_t size);
void uda_capnp_add_array_u64(NodeBuilder* node, const uint64_t* data, size_t size);
void uda_capnp_add_array_char(NodeBuilder* node, const char* data, size_t size);

void uda_capnp_add_f32(NodeBuilder* node, float data);
void uda_capnp_add_f64(NodeBuilder* node, double data);
void uda_capnp_add_i8(NodeBuilder* node, int8_t data);
void uda_capnp_add_i16(NodeBuilder* node, int16_t data);
void uda_capnp_add_i32(NodeBuilder* node, int32_t data);
void uda_capnp_add_i64(NodeBuilder* node, int64_t data);
void uda_capnp_add_u8(NodeBuilder* node, uint8_t data);
void uda_capnp_add_u16(NodeBuilder* node, uint16_t data);
void uda_capnp_add_u32(NodeBuilder* node, uint32_t data);
void uda_capnp_add_u64(NodeBuilder* node, uint64_t data);
void uda_capnp_add_char(NodeBuilder* node, char data);

void uda_capnp_print_tree_builder(TreeBuilder* tree);
void uda_capnp_print_tree_reader(TreeReader* tree);

#ifdef __cplusplus
}
#endif

#endif // UDA_CLIENTSERVER_CAPNP_SERIALISATION_H