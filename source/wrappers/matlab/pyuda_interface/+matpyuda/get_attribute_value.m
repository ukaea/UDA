function data = get_attribute_value(value)
  if py.str(py.type(value)) == "<class 'str'>"
    data = string(value);
  elseif py.str(py.type(value)) == "<class 'list'>"
    data = unpack_python_list(value);
  elseif py.str(py.type(value)) == "<class 'float32'>"
    data = double(value);
  elseif py.str(py.type(value)) == "<class 'float64'>"
    data = double(value);
  elseif py.str(py.type(value)) == "<class 'float'>"
    data = double(value);
  elseif py.str(py.type(value)) == "<class 'double'>"
    data = double(value);
  elseif py.str(py.type(value)) == "<class 'int32'>"
    data = get_py_int(value);
  elseif py.str(py.type(value)) == "<class 'int64'>"
    data = get_py_int(value);
  elseif py.str(py.type(value)) == "<class 'int'>"
    data = get_py_int(value);
%  elseif class(value) == 'py.bytes':
%    data = string(value.decode());
  else
    try 
      data = matpyuda.get_np_data(value);
    catch exception
      t_str = string(py.str(py.type(value)));
      ME = MException("get_attribute_value:TyeNotImplemented", "datatype %s not implemented", t_str); 
      addCause(exception, ME);
      throw(exception)
    end
  end
end

function data = unpack_python_list(value)
  if py.str(py.type(value{1})) == "<class 'str'>"
    data = string(value);
  elseif py.str(py.type(value{1})) == "<class 'float32'>"
    data = double(value);
  elseif py.str(py.type(value{1})) == "<class 'float64'>"
    data = double(value);
  elseif py.str(py.type(value{1})) == "<class 'float'>"
    data = double(value);
  elseif py.str(py.type(value{1})) == "<class 'double'>"
    data = double(value);
  elseif py.str(py.type(value{1})) == "<class 'int32'>"
    data = get_py_int(value);
  elseif py.str(py.type(value{1})) == "<class 'int64'>"
    data = get_py_int(value);
  elseif py.str(py.type(value{1})) == "<class 'int'>"
    data = get_py_int(value);
%  elseif class(value) == 'py.bytes':
%    data = string(value.decode());
  else
      t_str = string(py.str(py.type(value{1})));
      ME = MException("unpack_python_list:TyeNotImplemented", "datatype %s not implemented", t_str); 
      throw(ME)
  end
end

function i = get_py_int(pyval)
  try
    i = int32(pyval);
  catch ME
    i = int64(pyval);
  end
end