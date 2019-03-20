%module c_uda

%include "std_string.i"
%include "std_vector.i"
%include "std_map.i"
%include "carrays.i"

%array_class(unsigned char, ByteArray)

%{
#include "UDA.hpp"

static PyObject* UDA_Exception;
static PyObject* Protocol_Exception;
static PyObject* Server_Exception;
static PyObject* Invalid_Use_Exception;
%}

%init %{
    UDA_Exception = PyErr_NewException("pyuda.UDAException", NULL, NULL);
    Py_INCREF(UDA_Exception);
    PyModule_AddObject(m, "UDAException", UDA_Exception);

    Protocol_Exception = PyErr_NewException("pyuda.ProtocolException", UDA_Exception, NULL);
    Py_INCREF(Protocol_Exception);
    PyModule_AddObject(m, "ProtocolException", Protocol_Exception);

    Server_Exception = PyErr_NewException("pyuda.ServerException", UDA_Exception, NULL);
    Py_INCREF(Server_Exception);
    PyModule_AddObject(m, "ServerException", Server_Exception);

    Invalid_Use_Exception = PyErr_NewException("pyuda.InvalidUseException", UDA_Exception, NULL);
    Py_INCREF(Invalid_Use_Exception);
    PyModule_AddObject(m, "InvalidUseException", Invalid_Use_Exception);
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

%exception {
  try {
    $action
  }
  catch (uda::ProtocolException& e) {
    PyErr_SetString(Protocol_Exception, const_cast<char*>(e.what()));
    SWIG_fail;
  }
  catch (uda::ServerException& e) {
    PyErr_SetString(Server_Exception, const_cast<char*>(e.what()));
    SWIG_fail;
  }
  catch (uda::InvalidUseException& e) {
    PyErr_SetString(Invalid_Use_Exception, const_cast<char*>(e.what()));
    SWIG_fail;
  }
  catch (uda::UDAException& e) {
    PyErr_SetString(UDA_Exception, const_cast<char*>(e.what()));
    SWIG_fail;
  }
}

%include "data.hpp"
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
%include "array.hpp"
%include "scalar.hpp"
%include "structdata.hpp"
%include "treenode.hpp"
%include "string.hpp"
%include "result.hpp"

%template(fdata) uda::Array::_as<float>;
%template(ddata) uda::Array::_as<double>;
%template(cdata) uda::Array::_as<char>;
%template(ucdata) uda::Array::_as<unsigned char>;
%template(sdata) uda::Array::_as<short>;
%template(usdata) uda::Array::_as<unsigned short>;
%template(idata) uda::Array::_as<int>;
%template(uidata) uda::Array::_as<unsigned int>;
%template(ldata) uda::Array::_as<long>;
%template(uldata) uda::Array::_as<unsigned long>;
%template(string) uda::Array::_as<char *>;

%template(fdata) uda::Vector::_as<float>;
%template(ddata) uda::Vector::_as<double>;
%template(cdata) uda::Vector::_as<char>;
%template(ucdata) uda::Vector::_as<unsigned char>;
%template(sdata) uda::Vector::_as<short>;
%template(usdata) uda::Vector::_as<unsigned short>;
%template(idata) uda::Vector::_as<int>;
%template(uidata) uda::Vector::_as<unsigned int>;
%template(ldata) uda::Vector::_as<long>;
%template(uldata) uda::Vector::_as<unsigned long>;
%template(string) uda::Vector::_as<char *>;

%template(fdata) uda::Scalar::_as<float>;
%template(ddata) uda::Scalar::_as<double>;
%template(cdata) uda::Scalar::_as<char>;
%template(ucdata) uda::Scalar::_as<unsigned char>;
%template(sdata) uda::Scalar::_as<short>;
%template(usdata) uda::Scalar::_as<unsigned short>;
%template(idata) uda::Scalar::_as<int>;
%template(uidata) uda::Scalar::_as<unsigned int>;
%template(ldata) uda::Scalar::_as<long>;
%template(uldata) uda::Scalar::_as<unsigned long>;
%template(string) uda::Scalar::_as<char *>;

%template(fdata) uda::StructData::_as<float>;
%template(ddata) uda::StructData::_as<double>;
%template(cdata) uda::StructData::_as<char>;
%template(ucdata) uda::StructData::_as<unsigned char>;
%template(sdata) uda::StructData::_as<short>;
%template(usdata) uda::StructData::_as<unsigned short>;
%template(idata) uda::StructData::_as<int>;
%template(uidata) uda::StructData::_as<unsigned int>;
%template(ldata) uda::StructData::_as<long>;
%template(uldata) uda::StructData::_as<unsigned long>;
%template(string) uda::StructData::_as<char *>;

%template(fdata) uda::Dim::_as<float>;
%template(ddata) uda::Dim::_as<double>;
%template(cdata) uda::Dim::_as<char>;
%template(ucdata) uda::Dim::_as<unsigned char>;
%template(sdata) uda::Dim::_as<short>;
%template(usdata) uda::Dim::_as<unsigned short>;
%template(idata) uda::Dim::_as<int>;
%template(uidata) uda::Dim::_as<unsigned int>;
%template(ldata) uda::Dim::_as<long>;
%template(uldata) uda::Dim::_as<unsigned long>;
%template(string) uda::Dim::_as<char *>;
