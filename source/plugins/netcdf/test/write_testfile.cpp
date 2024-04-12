#include "netcdfpp.h"
#include <netcdf.h>
#include <vector>
#include <numeric>
#include <filesystem>
#include "random_generator.hpp"

#include <algorithm>
#include <list>
#include <random>
#include <type_traits>

#include "write_testfile.hpp"

// netcdfpp dimensions() method assumes dimensions are in same group as variables
/*
need to retrieve list of dims in each group (recursively upwards through parents) to find where 
a dim associated with a give variablen is located -- actually doesn't even need to be in direct lineage
nc_inq_dimids (int ncid, int *ndims, int *dimids, int include_parents)

currently, ncreader loops through all groups and tests if the specified dimid with the given dim name exists there

also: test if identical names are allowed in different groups? Otherise can dims be ambiguous?
how does indexing work? can grab a variable by index?

TO DO:
  change int, short etc to int32_t etc.
*/

using namespace uda::plugins::netcdf;
namespace nc_test_files
{
    void write_nested_file(std::string base_path)
    {
        netCDF::File file(base_path + "/signal_tests.nc", 'w');
        auto dim1 = file.add_dimension("dim1", 10);
        auto dim2 = file.add_dimension("dim2", 5);
        auto dim3 = file.add_dimension("dim3", 2);
        // auto dim_ulim = file.add_dimension("dim_ulim");

        auto meta_int_att = file.add_attribute("int_attribute");
        meta_int_att.set<int>(1);
        auto meta_float_att = file.add_attribute("float_attribute");
        meta_float_att.set<float>(32.5);
        auto meta_str_att = file.add_attribute("string_attribute");
        meta_str_att.set<std::string>("descriptive text: and, symbols!");

        auto dim1_var = file.add_variable<int>("dim1", {dim1});
        dim1_var.set<int>({1,2,3,4,5,6,7,8,9,10});
        auto dim2_var = file.add_variable<int>("dim2", {dim2});
        dim2_var.set<int>({1,2,3,4,5});
        auto dim3_var = file.add_variable<float>("dim3", {dim3});
        dim3_var.set<float>({0.01, 0.02});

        auto group1 = file.add_group("device_1");
        auto group2 = file.add_group("device_2");
        auto nested_group = group1.add_group("subgroup1");

        // netCDF::Attribute att;
        auto att = group1.add_attribute("string_attribute");
        att.set<std::string>(std::string("some description"));
        att = group1.add_attribute("int_attribute");
        att.set<int>(5);
        att = group1.add_attribute("float_attribute");
        att.set<float>(3.7);


        // auto testdimvar = group1.add_variable<int>("dim1", {dim1});
        // testdimvar.set<int>({10,9,8,7,6,5,4,3,2,1});

        // case 1.1: 1D int array
        auto int_variable = nested_group.add_variable<int>("int_variable_1d", {dim1});
        std::vector<int> int_var_values(10);
        std::iota(int_var_values.begin(), int_var_values.end(), 0);
        int_variable.set<int>(int_var_values);

        att = int_variable.add_attribute("string_attribute");
        att.set<std::string>(std::string("some description"));
        att = int_variable.add_attribute("int_attribute");
        att.set<int>(5);
        att = int_variable.add_attribute("float_attribute");
        att.set<float>(3.7);
        

        // case 1.2: 1D float array
        auto float_variable = nested_group.add_variable<float>("float_variable_1d", {dim1});
        std::vector<float> float_var_values(10);
        for (unsigned int i=0; i<10; i++)
        {
            float_var_values[i] = (float) i / 2.0;
        }
        float_variable.set<float>(float_var_values);

        att = float_variable.add_attribute("string_attribute");
        att.set<std::string>(std::string("some description"));
        att = float_variable.add_attribute("int_attribute");
        att.set<int>(5);
        att = float_variable.add_attribute("float_attribute");
        att.set<float>(3.7);

        // case 1.3: 1D double array
        auto double_variable = nested_group.add_variable<double>("double_variable_1d", {dim1});
        std::vector<double> double_var_values(10);
        for (unsigned int i=0; i<10; i++)
        {
            double_var_values[i] = (double) i / 3.0;
        }
        double_variable.set<double>(double_var_values);

        // 1.x all uda types here...

        // 2.1 2D int array
        auto int_variable_2d = group2.add_variable<int>("int_variable_2d", {dim2, dim3});
        std::vector<int> int_var_2d_values(10);
        std::iota(int_var_2d_values.begin(), int_var_2d_values.end(), 0);
        int_variable_2d.set<int>(int_var_2d_values);

        // 2.x all uda types here...

        file.sync();
        file.close();
    }

    // template<typename T, typename = typename std::enable_if<std::is_arithmetic<T>::value>::type>
    // class RandomGenerator 
    // {
    //     using distribution_type = typename std::conditional<std::is_integral<T>::value, std::uniform_int_distribution<T>,
    //         std::uniform_real_distribution<T>>::type;
    //
    //     T min = std::numeric_limits<T>().min();
    //     T max = std::numeric_limits<T>().max();
    //
    //     std::default_random_engine engine;
    //     distribution_type distribution;
    //
    //     public:
    //     inline RandomGenerator() : distribution(min, max){}
    //
    //     auto operator()() -> decltype(distribution(engine)) 
    //     {
    //         return distribution(engine);
    //     }
    //
    //     inline void reseed(int val)
    //     {
    //         engine.seed(val);
    //     }
    // };
    //
    // template<typename Container, typename T = typename Container::value_type>
    // auto make_generator(Container const&) -> decltype(RandomGenerator<T>()) 
    // {
    //     return RandomGenerator<T>();
    // }

    // later consider multidimension tests too
    template<typename T>
    void add_file(std::string filename)
    {
        filename += ".nc";
        size_t dimlength = 10;
        int numvars =3; 

        netCDF::File file(filename, 'w');
        auto dim1 = file.add_dimension("dim1", dimlength);

        auto generator = RandomGenerator<T>();

        // use random generator to fill values for each variable type
        for (int i=0; i <numvars; i++)
        {
            std::string name = "var" + std::to_string(i);
            auto var = file.add_variable<T>(name, {dim1});
            std::vector<T> vals(dimlength);
            generator.reseed(i*i + 1);
            std::generate(std::begin(vals), std::end(vals), generator);
            var. template set<T>(vals);

            if (i == 0)
            {
                auto file_att = file.add_attribute("attribute");
                file_att. template set<T>(vals[0]);
            }
            auto var_att = var.add_attribute("attribute");
            var_att. template set<T>(vals[0]);
        }
        file.sync();
        file.close();
    }

    // add file specialisation for stringtype !  

    void create_testfile_folder(std::string name)
    {
        if (!std::filesystem::is_directory(name) || !std::filesystem::exists(name)) 
        {
            std::filesystem::create_directory(name);
        }
    }

    void write_type_test(std::string base_name)
    {
        std::string folder_name = base_name + "/type_test_files";
        create_testfile_folder(folder_name);

        std::string file_name_base = "";
        std::string full_path = folder_name + "/" +  file_name_base;

        // if(std::is_same<T, char>::value)
        add_file<char>(full_path + "char");
        add_file<int>(full_path + "int");
        add_file<float>(full_path + "float");
        add_file<double>(full_path + "double");
        // add_file<long>(full_path + "long");
        add_file<long long>(full_path + "longlong");
        add_file<short>(full_path + "short");
        add_file<unsigned char>(full_path + "unsigned_char");
        add_file<unsigned int>(full_path + "unsigned_int");
        add_file<unsigned short>(full_path + "unsigned_short");
        // add_file<unsigned long>(full_path + "unsigned_long");
        add_file<unsigned long long>(full_path + "unsigned_longlong");
        // TODO
        add_file<char>(full_path + "string");
    }

    // struct TypeCompound {
    //     char c;
    //     int i[3][2];
    //     double d;
    //     bool operator==(const TypeCompound& rhs) const 
    //     {
    //         return c == rhs.c && i[0][0] == rhs.i[0][0] && i[0][1] == rhs.i[0][1] && i[1][0] == rhs.i[1][0] && i[1][1] == rhs.i[1][1] && i[2][0] == rhs.i[2][0]
    //             && i[2][1] == rhs.i[2][1] && d == rhs.d;
    //     }
    // };

    // template<typename T>
    // struct TypeCompound
    // {
    //     // does this need to be a c type?
    //     T data[10];
    //     // char description[30];
    //     char* description;
    //     char c;
    //     // bool operator== (const TypeCompound& rhs) const 
    //     // {
    //     //     return rhs.data == data && rhs.description == description && rhs.c == c;
    //     // }
    // };

    struct TypeOpaque 
    {
        char c;
        bool operator==(const TypeOpaque& rhs) const { return c == rhs.c; }
    };

    enum class Enum : int 
    {
        E1 = 1,
        E2 = 2,
        E3 = 3,
    };

    template<typename T>
    void write_compound_file(std::string filename)
    {
        size_t dimlength = 10;
        int seed = 1;

        // use random generator to fill values for each variable type
        auto generator = RandomGenerator<T>();
        std::vector<T> vals(dimlength);
        generator.reseed(seed);
        std::generate(std::begin(vals), std::end(vals), generator);

        std::string description = "some text";

        netCDF::File file(filename + ".nc", 'w');
        auto dim1 = file.add_dimension("dim1", 1);
        auto type_compound = file.template add_type_compound<TypeCompound<T>>("type_compound");
        type_compound.template add_compound_field_array<T>("data", offsetof(TypeCompound<T>, data), {(int)dimlength});
        type_compound.template add_compound_field<char*>("description", offsetof(TypeCompound<T>, description));
        type_compound.template add_compound_field<char>("c", offsetof(TypeCompound<T>, c));

        auto var_compound = file.add_variable("var_compound", type_compound, {dim1});

        TypeCompound<T> obj; 

        std::copy(vals.data(), vals.data() + dimlength, obj.data);
        
        obj.c = 'a';
        obj.description = (char*) description.c_str();

        var_compound.template set<TypeCompound<T>>({obj});
        
        file.sync();
        file.close();
    }

    void write_usertype_test(std::string base_path)
    {
        std::string folder_name = base_path + "/usertype_test_files";
        create_testfile_folder(folder_name);

        std::string file_name_base = "";
        std::string full_path = folder_name + "/" +  file_name_base;

        // if(std::is_same<T, char>::value)
        write_compound_file<char>(full_path + "char");
        write_compound_file<int32_t>(full_path + "int");
        write_compound_file<float>(full_path + "float");
        write_compound_file<double>(full_path + "double");
        // write_compound_file<long>(full_path + "long"); //
        write_compound_file<long long>(full_path + "longlong");
        write_compound_file<short>(full_path + "short");
        write_compound_file<unsigned char>(full_path + "unsigned_char");
        write_compound_file<unsigned int>(full_path + "unsigned_int");
        write_compound_file<unsigned short>(full_path + "unsigned_short");
        // write_compound_file<unsigned long>(full_path + "unsigned_long"); //
        write_compound_file<unsigned long long>(full_path + "unsigned_longlong");
        // TODO
        // write_compound_file<char>(full_path + "string");
    }

    struct CapnpTestObject
    {
        // CapnpTestObject (char* n) : name(n) {}
        char* name;
    };

    void write_capnp_test_files(std::string base_path, bool repeated_node)
    {
        std::string file_name = repeated_node ? "capnp_test_repeated" : "capnp_test_unique";
        netCDF::File file(base_path + "/" + file_name + ".nc", 'w');
        auto dim1 = file.add_dimension("dim1", 1);
        auto group1 = file.add_group("group_1");
        // auto group2 = file.add_group("group_2");

        auto capnp_struct = file.add_type_compound<CapnpTestObject>("capnp_struct");
        capnp_struct.add_compound_field<char*>("name", offsetof(CapnpTestObject, name));

        auto var1 = group1.add_variable("gas_baffle_upper", capnp_struct, {dim1});
        auto var2 = group1.add_variable("gas_baffle_lower", capnp_struct, {dim1});

        CapnpTestObject capnp_obj_1, capnp_obj_2; 

        std::string var1_name = "gas_baffle";
        capnp_obj_1.name = (char*) var1_name.c_str();
        
        std::string var2_name = repeated_node ? var1_name : "gas_baffle_lower";
        capnp_obj_2.name = (char*) var2_name.c_str();

        var1.set<CapnpTestObject>({capnp_obj_1});
        var2.set<CapnpTestObject>({capnp_obj_2});
        
        file.sync();
        file.close();
    }

    void write_test_files(std::string file_path)
    {
        create_testfile_folder(file_path);
        write_nested_file(file_path);
        write_type_test(file_path);
        write_usertype_test(file_path);
        write_capnp_test_files(file_path, true);
        write_capnp_test_files(file_path, false);

        // return 0;
    }

};

// int main()
// {
//     nc_test_files::write_test_files();
//     return 0;
// }

