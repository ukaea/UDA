cdef extern from "winsock2.h":
    pass
include "handle.pyx"
include "client.pyx"
include "dim.pyx"
include "exceptions.pyx"
include "result.pyx"
include "tree_node.pyx"
include "types.pyx"
IF CAPNP:
    include "capnp_tree.pyx"
