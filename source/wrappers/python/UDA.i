%module c_uda

%include "std_string.i"
%include "std_vector.i"
%include "std_map.i"

%{
#include "UDA.hpp"
%}

%naturalvar;

%typemap(out) std::type_info& {
    if ($1 == &typeid(char)) {
        $result = PyString_FromString("int8");
    } else if ($1 == &typeid(short)) {
        $result = PyString_FromString("int16");
    } else if ($1 == &typeid(int)) {
        $result = PyString_FromString("int32");
    } else if ($1 == &typeid(unsigned int)) {
        $result = PyString_FromString("uint32");
    } else if ($1 == &typeid(long)) {
        $result = PyString_FromString("int64");
    } else if ($1 == &typeid(float)) {
        $result = PyString_FromString("float32");
    } else if ($1 == &typeid(double)) {
        $result = PyString_FromString("float64");
    } else if ($1 == &typeid(unsigned char)) {
        $result = PyString_FromString("uint8");
    } else if ($1 == &typeid(unsigned short)) {
        $result = PyString_FromString("uint16");
    } else if ($1 == &typeid(unsigned long)) {
        $result = PyString_FromString("uint64");
    } else if ($1 == &typeid(char *)) {
        $result = PyString_FromString("string");
    } else {
        PyErr_SetString(PyExc_RuntimeError, "Unknown type_info");
        $result = NULL;
    }
}

%include "vector.hpp"
%include "dim.hpp"

%apply unsigned int { uda::dim_type }

%template(DimVector) std::vector<uda::Dim>;
%template(FloatVector) std::vector<float>;
%template(DoubleVector) std::vector<double>;
%template(StringVector) std::vector<std::string>;
%template(BoolVector) std::vector<bool>;
%template(LongVector) std::vector<long>;
%template(IntVector) std::vector<int>;
%template(ShortVector) std::vector<short>;
%template(UShortVector) std::vector<unsigned short>;
%template(CharVector) std::vector<char>;
%template(UCharVector) std::vector<unsigned char>;
%template(CStrVector) std::vector<char*>;
%template(SizeVector) std::vector<size_t>;
%template(SizeVectorVector) std::vector<std::vector<size_t> >;
#ifdef A64
%template(UIntVector) std::vector<unsigned int>;
#else
%template(ULongVector) std::vector<unsigned long>;
#endif
%template(StringStringMap) std::map<std::string, std::string>;

%typemap(out) uda::Data * {
    if (uda::Array * array = dynamic_cast<uda::Array*>($1)) {
        $result = SWIG_NewPointerObj(SWIG_as_voidptr(array), SWIGTYPE_p_uda__Array, 0);
    } else if (uda::String * string = dynamic_cast<uda::String*>($1)) {
        $result = SWIG_NewPointerObj(SWIG_as_voidptr(string), SWIGTYPE_p_uda__String, 0);
    } else if (uda::Scalar * scalar = dynamic_cast<uda::Scalar*>($1)) {
        $result = SWIG_NewPointerObj(SWIG_as_voidptr(scalar), SWIGTYPE_p_uda__Scalar, 0);
    }
}

%include "client.hpp"

%extend uda::UDAException {
    char * __str__() {
        return const_cast<char *>(self->what());
    }
}

%include "data.hpp"
%include "array.hpp"
%include "scalar.hpp"
%include "structdata.hpp"
%include "treenode.hpp"
%include "string.hpp"
%include "result.hpp"

%template(fdata) uda::Array::as<float>;
%template(ddata) uda::Array::as<double>;
%template(cdata) uda::Array::as<char>;
%template(ucdata) uda::Array::as<unsigned char>;
%template(sdata) uda::Array::as<short>;
%template(usdata) uda::Array::as<unsigned short>;
%template(idata) uda::Array::as<int>;
%template(uidata) uda::Array::as<unsigned int>;
%template(ldata) uda::Array::as<long>;
%template(uldata) uda::Array::as<unsigned long>;
%template(string) uda::Array::as<char *>;

%template(fdata) uda::Vector::as<float>;
%template(ddata) uda::Vector::as<double>;
%template(cdata) uda::Vector::as<char>;
%template(ucdata) uda::Vector::as<unsigned char>;
%template(sdata) uda::Vector::as<short>;
%template(usdata) uda::Vector::as<unsigned short>;
%template(idata) uda::Vector::as<int>;
%template(uidata) uda::Vector::as<unsigned int>;
%template(ldata) uda::Vector::as<long>;
%template(uldata) uda::Vector::as<unsigned long>;
%template(string) uda::Vector::as<char *>;

%template(fdata) uda::Scalar::as<float>;
%template(ddata) uda::Scalar::as<double>;
%template(cdata) uda::Scalar::as<char>;
%template(ucdata) uda::Scalar::as<unsigned char>;
%template(sdata) uda::Scalar::as<short>;
%template(usdata) uda::Scalar::as<unsigned short>;
%template(idata) uda::Scalar::as<int>;
%template(uidata) uda::Scalar::as<unsigned int>;
%template(ldata) uda::Scalar::as<long>;
%template(uldata) uda::Scalar::as<unsigned long>;
%template(string) uda::Scalar::as<char *>;

%template(fdata) uda::StructData::as<float>;
%template(ddata) uda::StructData::as<double>;
%template(cdata) uda::StructData::as<char>;
%template(ucdata) uda::StructData::as<unsigned char>;
%template(sdata) uda::StructData::as<short>;
%template(usdata) uda::StructData::as<unsigned short>;
%template(idata) uda::StructData::as<int>;
%template(uidata) uda::StructData::as<unsigned int>;
%template(ldata) uda::StructData::as<long>;
%template(uldata) uda::StructData::as<unsigned long>;
%template(string) uda::StructData::as<char *>;
