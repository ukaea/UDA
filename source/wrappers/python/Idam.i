%module cidam

%include "std_string.i"
%include "std_vector.i"
%include "std_map.i"

%{
#include "Idam.hpp"
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

%apply unsigned int { Idam::dim_type }

%template(DimVector) std::vector<Idam::Dim>;
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

%typemap(out) Idam::Data * {
    if (Idam::Array * array = dynamic_cast<Idam::Array*>($1)) {
        $result = SWIG_NewPointerObj(SWIG_as_voidptr(array), SWIGTYPE_p_Idam__Array, 0);
    } else if (Idam::String * string = dynamic_cast<Idam::String*>($1)) {
        $result = SWIG_NewPointerObj(SWIG_as_voidptr(string), SWIGTYPE_p_Idam__String, 0);
    } else if (Idam::Scalar * scalar = dynamic_cast<Idam::Scalar*>($1)) {
        $result = SWIG_NewPointerObj(SWIG_as_voidptr(scalar), SWIGTYPE_p_Idam__Scalar, 0);
    }
}

%include "client.hpp"

%extend Idam::IdamException {
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

%template(fdata) Idam::Array::as<float>;
%template(ddata) Idam::Array::as<double>;
%template(cdata) Idam::Array::as<char>;
%template(ucdata) Idam::Array::as<unsigned char>;
%template(sdata) Idam::Array::as<short>;
%template(usdata) Idam::Array::as<unsigned short>;
%template(idata) Idam::Array::as<int>;
%template(uidata) Idam::Array::as<unsigned int>;
%template(ldata) Idam::Array::as<long>;
%template(uldata) Idam::Array::as<unsigned long>;
%template(string) Idam::Array::as<char *>;

%template(fdata) Idam::Vector::as<float>;
%template(ddata) Idam::Vector::as<double>;
%template(cdata) Idam::Vector::as<char>;
%template(ucdata) Idam::Vector::as<unsigned char>;
%template(sdata) Idam::Vector::as<short>;
%template(usdata) Idam::Vector::as<unsigned short>;
%template(idata) Idam::Vector::as<int>;
%template(uidata) Idam::Vector::as<unsigned int>;
%template(ldata) Idam::Vector::as<long>;
%template(uldata) Idam::Vector::as<unsigned long>;
%template(string) Idam::Vector::as<char *>;

%template(fdata) Idam::Scalar::as<float>;
%template(ddata) Idam::Scalar::as<double>;
%template(cdata) Idam::Scalar::as<char>;
%template(ucdata) Idam::Scalar::as<unsigned char>;
%template(sdata) Idam::Scalar::as<short>;
%template(usdata) Idam::Scalar::as<unsigned short>;
%template(idata) Idam::Scalar::as<int>;
%template(uidata) Idam::Scalar::as<unsigned int>;
%template(ldata) Idam::Scalar::as<long>;
%template(uldata) Idam::Scalar::as<unsigned long>;
%template(string) Idam::Scalar::as<char *>;

%template(fdata) Idam::StructData::as<float>;
%template(ddata) Idam::StructData::as<double>;
%template(cdata) Idam::StructData::as<char>;
%template(ucdata) Idam::StructData::as<unsigned char>;
%template(sdata) Idam::StructData::as<short>;
%template(usdata) Idam::StructData::as<unsigned short>;
%template(idata) Idam::StructData::as<int>;
%template(uidata) Idam::StructData::as<unsigned int>;
%template(ldata) Idam::StructData::as<long>;
%template(uldata) Idam::StructData::as<unsigned long>;
%template(string) Idam::StructData::as<char *>;
