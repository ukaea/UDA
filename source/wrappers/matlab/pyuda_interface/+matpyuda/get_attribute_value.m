function data = get_attribute_value(value)
  switch class(value)
    case 'py.str'
      data = string(value);
    case 'py.list'
      data = unpack_python_list(value);
    elseif isa(value 'py.float32')
      data = double(value);
    case 'py.float64'
      data = double(value);
    case 'py.float'
      data = double(value);
    case 'py.double'
      data = double(value);
    case 'py.int32'
      data = get_py_int(value);
    case 'py.int64'
      data = get_py_int(value);
    case 'py.int'
      data = get_py_int(value);
    case 'py.uint32'
      data = get_py_uint(value);
    case 'py.uint64'
      data = get_py_uint(value);
  %  case 'py.bytes':
  %    data = string(value.decode());
    otherwise
      try 
        data = matpyuda.get_np_data(value);
      catch exception
        t_str = class(value);
        ME = MException("get_attribute_value:TyeNotImplemented", "datatype %s not implemented", t_str); 
        addCause(exception, ME);
        throw(exception)
      end
  end
end

function data = unpack_python_list(value)
  switch class(value{1})
    case "py.str"
      data = string(value);
    case "py.float32"
      data = double(value);
    case "py.float64"
      data = double(value);
    case "py.float"
      data = double(value);
    case "py.double"
      data = double(value);
    case "py.int32"
      data = get_py_int(value);
    case "py.int64"
      data = get_py_int(value);
    case "py.int"
      data = get_py_int(value);
%  case 'py.bytes':
%    data = string(value.decode());
    otherwise
      t_str = class(value{1});
      ME = MException("unpack_python_list:TyeNotImplemented", "datatype %s not implemented", t_str); 
      throw(ME)
  end
end

% seems that depending on whether system/ matlab is 32 bit or 64 bit only one of these will work
function i = get_py_int(pyval)
  try
    i = int32(pyval);
  catch ME
    i = int64(pyval);
  end
end

function i = get_py_uint(pyval)
  try
    i = uint32(pyval);
  catch ME
    i = uint64(pyval);
  end
end