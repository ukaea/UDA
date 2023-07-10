function data = get_attribute_value(value)
  if py.str(py.type(value)) == "<class 'str'>"
    data = string(value);
  elseif py.str(py.type(value)) == "<class 'float32'>"
    data = double(value);
  elseif py.str(py.type(value)) == "<class 'float64'>"
    data = double(value);
  elseif py.str(py.type(value)) == "<class 'float'>"
    data = double(value);
  elseif py.str(py.type(value)) == "<class 'double'>"
    data = double(value);
  elseif py.str(py.type(value)) == "<class 'int32'>"
    data = int32(value);
  elseif py.str(py.type(value)) == "<class 'int64'>"
    data = int64(value);
  elseif py.str(py.type(value)) == "<class 'int'>"
    data = int32(value);
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
