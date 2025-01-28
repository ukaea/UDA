#include <serialisation/capnp_serialisation.h>
#include <uda/types.h>
#include <uda/c++/UDA.hpp>

#include <boost/program_options.hpp>
#include <boost/range/combine.hpp>
#include <gsl/span>
#include <iostream>
#include <string>

struct CLIException : public uda::UDAException {
    explicit CLIException(std::string what) noexcept : uda::UDAException(std::move(what)) {}
};

template <typename T> std::ostream& operator<<(std::ostream& out, gsl::span<T> span)
{
    out << "[";
    const char* delim = "";
    int i = 0;
    for (const auto& el : span) {
        if (i == 10) {
            out << delim << "... (" << span.size() << " elements)";
            break;
        }
        out << delim << el;
        delim = ", ";
        ++i;
    }
    out << "]";
    return out;
}

template <typename T> std::ostream& operator<<(std::ostream& out, const std::vector<T>& vec)
{
    auto span = gsl::span{vec.data(), vec.size()};
    out << span;
    return out;
}

namespace po = boost::program_options;

bool replace(std::string& str, const std::string& from, const std::string& to)
{
    size_t start_pos = str.find(from);
    if (start_pos == std::string::npos) {
        return false;
    }
    str.replace(start_pos, from.length(), to);
    return true;
}

template <typename T> void print_atomic_data(void* data, size_t rank, size_t count)
{
    T* ptr = reinterpret_cast<T*>(data);
    if (rank == 0) {
        std::cout << ptr[0] << "\n";
    } else {
        auto span = gsl::span{ptr, count};
        std::cout << span << "\n";
    }
}

void print_tree(const uda::TreeNode& node, const std::string& indent)
{
    std::cout << indent << "name: " << node.name() << "\n";

    auto names = node.atomicNames();
    auto pointers = node.atomicPointers();
    auto ranks = node.atomicRank();
    auto shapes = node.atomicShape();
    auto types = node.atomicTypes();

    std::string name;
    bool pointer;
    size_t rank;
    std::vector<size_t> shape;
    std::string type;

    for (const auto& el : boost::combine(names, pointers, ranks, shapes, types)) {
        boost::tie(name, pointer, rank, shape, type) = el;

        auto data = node.structureComponentData(name);

        if (type.substr(0, 6) == "STRING") {
            if (type == "STRING") {
                std::cout << reinterpret_cast<char*>(data) << "\n";
            } else if (rank == 0 && shape[0] == 1 && !pointer) {
                std::cout << reinterpret_cast<char**>(data)[0] << "\n";
            } else {
                for (size_t i = 0; i < rank; ++i) {
                    std::cout << reinterpret_cast<char**>(data)[i] << "\n";
                }
            }
        } else {
            std::string base_type = type;
            replace(base_type, " *", "");
            size_t count = std::accumulate(shape.begin(), shape.end(), 1, std::multiplies<>());

            if (base_type == "char") {
                print_atomic_data<char>(data, rank, count);
            } else if (base_type == "short") {
                print_atomic_data<short>(data, rank, count);
            } else if (base_type == "int") {
                print_atomic_data<int>(data, rank, count);
            } else if (base_type == "long") {
                print_atomic_data<long>(data, rank, count);
            } else if (base_type == "long long") {
                print_atomic_data<long long>(data, rank, count);
            } else if (base_type == "float") {
                print_atomic_data<float>(data, rank, count);
            } else if (base_type == "double") {
                print_atomic_data<double>(data, rank, count);
            } else if (base_type == "unsigned char") {
                print_atomic_data<unsigned char>(data, rank, count);
            } else if (base_type == "unsigned short") {
                print_atomic_data<unsigned short>(data, rank, count);
            } else if (base_type == "unsigned int") {
                print_atomic_data<unsigned int>(data, rank, count);
            } else if (base_type == "unsigned long") {
                print_atomic_data<unsigned long>(data, rank, count);
            } else if (base_type == "unsigned long long") {
                print_atomic_data<unsigned long long>(data, rank, count);
            }
        }
    }

    for (const auto& child : node.children()) {
        print_tree(child, indent + "  ");
    }
}

#ifdef CAPNP_ENABLED
template <typename T>
void print_capnp_data(NodeReader* node, const std::vector<size_t>& shape, const std::string& indent)
{
    auto num_slices = uda_capnp_read_num_slices(node);

    if (shape.empty()) {
        if (num_slices > 1) {
            throw CLIException("Invalid scalar data");
        }
        T scalar;
        uda_capnp_read_data(node, 0, reinterpret_cast<char*>(&scalar));
        std::cout << indent << "data: " << scalar << "\n";
    } else if (num_slices == 1) {
        auto count = uda_capnp_read_slice_size(node, 0);
        auto data = std::make_unique<T[]>(count / sizeof(T));
        uda_capnp_read_data(node, 0, reinterpret_cast<char*>(data.get()));
        auto span = gsl::span{data.get(), count / sizeof(T)};
        std::cout << indent << "data: " << span << "\n";
    } else {
        for (size_t slice_num = 0; slice_num < num_slices; ++slice_num) {
            auto count = uda_capnp_read_slice_size(node, slice_num);
            auto data = std::make_unique<T[]>(count / sizeof(T));
            uda_capnp_read_data(node, slice_num, reinterpret_cast<char*>(data.get()));
            auto span = gsl::span{data.get(), count / sizeof(T)};
            std::cout << indent << "data [" << slice_num << "]: " << span << "\n";
        }
    }
}

void print_capnp_node(TreeReader* tree, NodeReader* node, const std::string& indent)
{
    auto num_children = uda_capnp_num_children(node);

    auto name = uda_capnp_read_name(node);
    auto type = uda_capnp_read_type(node);
    auto rank = uda_capnp_read_rank(node);

    std::cout << indent << "name: " << name << "\n";

    if (rank.has_value) {
        std::vector<size_t> shape = {};
        if (rank.value > 0) {
            shape.resize(rank.value);
            uda_capnp_read_shape(node, shape.data());
        }

        switch (type) {
            case UDA_TYPE_CHAR:
                print_capnp_data<char>(node, shape, indent);
                break;
            case UDA_TYPE_SHORT:
                print_capnp_data<short>(node, shape, indent);
                break;
            case UDA_TYPE_INT:
                print_capnp_data<int>(node, shape, indent);
                break;
            case UDA_TYPE_LONG:
                print_capnp_data<long>(node, shape, indent);
                break;
            case UDA_TYPE_LONG64:
                print_capnp_data<long long>(node, shape, indent);
                break;
            case UDA_TYPE_UNSIGNED_CHAR:
                print_capnp_data<unsigned char>(node, shape, indent);
                break;
            case UDA_TYPE_UNSIGNED_SHORT:
                print_capnp_data<unsigned short>(node, shape, indent);
                break;
            case UDA_TYPE_UNSIGNED_INT:
                print_capnp_data<unsigned int>(node, shape, indent);
                break;
            case UDA_TYPE_UNSIGNED_LONG:
                print_capnp_data<unsigned long>(node, shape, indent);
                break;
            case UDA_TYPE_UNSIGNED_LONG64:
                print_capnp_data<unsigned long long>(node, shape, indent);
                break;
            case UDA_TYPE_FLOAT:
                print_capnp_data<float>(node, shape, indent);
                break;
            case UDA_TYPE_DOUBLE:
                print_capnp_data<double>(node, shape, indent);
                break;
        }
    }

    for (size_t n = 0; n < num_children; ++n) {
        auto child = uda_capnp_read_child_n(tree, node, n);
        print_capnp_node(tree, child, indent + "  ");
    }
}

void print_capnp(const char* bytes, size_t len)
{
    TreeReader* tree = uda_capnp_deserialise(bytes, len);
    NodeReader* root = uda_capnp_read_root(tree);
    print_capnp_node(tree, root, "");
}
#endif // CAPNP_ENABLED

void print_data(const uda::Data* data, int uda_type)
{
    if (data->isNull()) {
        throw CLIException("No data returned");
    }

    auto array = dynamic_cast<const uda::Array*>(data);
    if (array) {
        switch (uda_type) {
            case UDA_TYPE_CHAR:
                std::cout << array->as<char>() << std::endl;
                break;
            case UDA_TYPE_SHORT:
                std::cout << array->as<short>() << std::endl;
                break;
            case UDA_TYPE_INT:
                std::cout << array->as<int>() << std::endl;
                break;
            case UDA_TYPE_LONG:
                std::cout << array->as<long>() << std::endl;
                break;
            case UDA_TYPE_LONG64:
                std::cout << array->as<long long>() << std::endl;
                break;
            case UDA_TYPE_UNSIGNED_CHAR:
                std::cout << array->as<unsigned char>() << std::endl;
                break;
            case UDA_TYPE_UNSIGNED_SHORT:
                std::cout << array->as<unsigned short>() << std::endl;
                break;
            case UDA_TYPE_UNSIGNED_INT:
                std::cout << array->as<unsigned int>() << std::endl;
                break;
            case UDA_TYPE_UNSIGNED_LONG:
                std::cout << array->as<unsigned long>() << std::endl;
                break;
            case UDA_TYPE_UNSIGNED_LONG64:
                std::cout << array->as<unsigned long long>() << std::endl;
                break;
            case UDA_TYPE_FLOAT:
                std::cout << array->as<float>() << std::endl;
                break;
            case UDA_TYPE_DOUBLE:
                std::cout << array->as<double>() << std::endl;
                break;
        }
    }

    auto scalar = dynamic_cast<const uda::Scalar*>(data);
    if (scalar) {
        switch (uda_type) {
            case UDA_TYPE_CHAR:
                std::cout << scalar->as<char>() << std::endl;
                break;
            case UDA_TYPE_SHORT:
                std::cout << scalar->as<short>() << std::endl;
                break;
            case UDA_TYPE_INT:
                std::cout << scalar->as<int>() << std::endl;
                break;
            case UDA_TYPE_LONG:
                std::cout << scalar->as<long>() << std::endl;
                break;
            case UDA_TYPE_LONG64:
                std::cout << scalar->as<long long>() << std::endl;
                break;
            case UDA_TYPE_UNSIGNED_CHAR:
                std::cout << scalar->as<unsigned char>() << std::endl;
                break;
            case UDA_TYPE_UNSIGNED_SHORT:
                std::cout << scalar->as<unsigned short>() << std::endl;
                break;
            case UDA_TYPE_UNSIGNED_INT:
                std::cout << scalar->as<unsigned int>() << std::endl;
                break;
            case UDA_TYPE_UNSIGNED_LONG:
                std::cout << scalar->as<unsigned long>() << std::endl;
                break;
            case UDA_TYPE_UNSIGNED_LONG64:
                std::cout << scalar->as<unsigned long long>() << std::endl;
                break;
            case UDA_TYPE_FLOAT:
                std::cout << scalar->as<float>() << std::endl;
                break;
            case UDA_TYPE_DOUBLE:
                std::cout << scalar->as<double>() << std::endl;
                break;
        }
    }

    auto string = dynamic_cast<const uda::String*>(data);
    if (string) {
        std::cout << string->str();
    }

    auto vector = dynamic_cast<const uda::Vector*>(data);
    if (vector) {
        switch (uda_type) {
            case UDA_TYPE_CHAR:
                std::cout << vector->as<char>() << std::endl;
                break;
            case UDA_TYPE_SHORT:
                std::cout << vector->as<short>() << std::endl;
                break;
            case UDA_TYPE_INT:
                std::cout << vector->as<int>() << std::endl;
                break;
            case UDA_TYPE_LONG:
                std::cout << vector->as<long>() << std::endl;
                break;
            case UDA_TYPE_LONG64:
                std::cout << vector->as<long long>() << std::endl;
                break;
            case UDA_TYPE_UNSIGNED_CHAR:
                std::cout << vector->as<unsigned char>() << std::endl;
                break;
            case UDA_TYPE_UNSIGNED_SHORT:
                std::cout << vector->as<unsigned short>() << std::endl;
                break;
            case UDA_TYPE_UNSIGNED_INT:
                std::cout << vector->as<unsigned int>() << std::endl;
                break;
            case UDA_TYPE_UNSIGNED_LONG:
                std::cout << vector->as<unsigned long>() << std::endl;
                break;
            case UDA_TYPE_UNSIGNED_LONG64:
                std::cout << vector->as<unsigned long long>() << std::endl;
                break;
            case UDA_TYPE_FLOAT:
                std::cout << vector->as<float>() << std::endl;
                break;
            case UDA_TYPE_DOUBLE:
                std::cout << vector->as<double>() << std::endl;
                break;
        }
    }
}

void conflicting_options(const boost::program_options::variables_map& vm, const std::string& opt1,
                         const std::string& opt2)
{
    if (vm.count(opt1) && !vm[opt1].defaulted() && vm.count(opt2) && !vm[opt2].defaulted()) {
        throw po::error(std::string("conflicting options '") + opt1 + "' and '" + opt2 + "'");
    }
}

int main(int argc, const char** argv)
{
    po::options_description desc("Allowed options");
    desc.add_options()("help", po::bool_switch(), "produce help message")(
        "host,h", po::value<std::string>()->default_value("localhost"),
        "server host name")("port,p", po::value<int>()->default_value(56565), "server port")(
        "request", po::value<std::string>(), "request")("source", po::value<std::string>()->default_value(""),
                                                        "source")("ping", po::bool_switch(), "ping the server");

    po::positional_options_description p;
    p.add("request", 1);

    po::variables_map vm;
    try {
        po::store(po::command_line_parser(argc, argv).options(desc).positional(p).run(), vm);
        po::notify(vm);

        conflicting_options(vm, "ping", "request");
        if (!vm["ping"].as<bool>() && vm.count("request") == 0) {
            throw po::error("either 'ping' or 'request' must be provided");
        }
    } catch (po::error& err) {
        if (vm["help"].as<bool>()) {
            std::cout << "Usage: " << argv[0] << " [options] request\n";
            std::cout << desc << "\n";
            return 1;
        } else {
            std::cout << "Error: " << err.what() << "\n\n";
            std::cout << "Usage: " << argv[0] << " [options] request\n";
            std::cout << desc << "\n";
            return -1;
        }
    };

    if (vm["help"].as<bool>()) {
        std::cout << "Usage: " << argv[0] << " [options] request\n";
        std::cout << desc << "\n";
        return 1;
    }

    if (vm.count("host")) {
        uda::Client::setServerHostName(vm["host"].as<std::string>());
    }

    if (vm.count("port")) {
        uda::Client::setServerPort(static_cast<int>(vm["port"].as<int>()));
    }

    std::string request;
    if (vm["ping"].as<bool>()) {
        request = "HELP::ping()";
    } else {
        request = vm["request"].as<std::string>();
    }

    if (request == "-") {
        std::stringstream ss;
        std::string line;
        while (std::getline(std::cin, line)) {
            ss << line;
        }
        request = ss.str();
    }
    std::cout << "request: " << request << "\n";

    std::string source = vm["source"].as<std::string>();

    uda::Client client;
    try {
        auto& res = client.get(request, source);

        if (res.isTree()) {
            print_tree(res.tree(), "");
        } else if (res.uda_type() == UDA_TYPE_CAPNP) {
#ifdef CAPNP_ENABLED
            print_capnp(res.raw_data(), res.size());
#else
            std::cout << "Cap'n Proto not enabled - cannot display data\n";
#endif
        } else {
            print_data(res.data(), res.uda_type());
        }

        client.close();

    } catch (uda::UDAException& err) {
        std::cerr << "UDA error: " << err.what() << "\n";
        return -1;
    }

    return 0;
}