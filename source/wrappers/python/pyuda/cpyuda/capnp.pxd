from libcpp cimport bool as cbool
from handle cimport Handle


cdef extern from "uda/serialisation/capnp_serialisation.h":
    ctypedef struct TreeReader
    ctypedef struct NodeReader
    ctypedef struct Optional_Size_t:
        cbool has_value
        size_t value

    TreeReader* uda_capnp_deserialise(char* bytes, size_t size);

    NodeReader* uda_capnp_read_root(TreeReader* tree);
    size_t uda_capnp_num_children(NodeReader* node);
    NodeReader* uda_capnp_read_child(TreeReader* tree, NodeReader* node, const char* name);
    NodeReader* uda_capnp_read_child_n(TreeReader* tree, NodeReader* node, size_t index);
    const char* uda_capnp_read_name(NodeReader* node);
    int uda_capnp_read_type(NodeReader* node);
    Optional_Size_t uda_capnp_read_rank(NodeReader* node);
    cbool uda_capnp_read_shape(NodeReader* node, size_t* shape);
    size_t uda_capnp_read_num_slices(NodeReader* node);
    cbool uda_capnp_read_is_eos(NodeReader* node);
    size_t uda_capnp_read_slice_size(NodeReader* node, size_t slice_num);
    cbool uda_capnp_read_data(NodeReader* node, size_t slice_num, char* data);
    void uda_capnp_print_tree_reader(TreeReader* tree);


cdef class CapnpTreeNode:

    cdef Handle _handle
    cdef TreeReader* _tree
    cdef NodeReader* _node
    cdef list _children
    cdef int _children_init
    cdef int _data_init
    cdef object _data

    @staticmethod
    cdef new_(Handle handle, TreeReader* tree, NodeReader* node)
