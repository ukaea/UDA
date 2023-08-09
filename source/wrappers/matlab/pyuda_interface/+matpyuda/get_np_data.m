function data = get_np_data(np_data)
  if np_data.dtype == py.numpy.float32
      data = get_np_data_item(np_data, @double);
  elseif np_data.dtype == py.numpy.float64
      data = get_np_data_item(np_data, @double);
  elseif np_data.dtype == py.numpy.int8
      data = get_np_data_item(np_data, @int8);
  elseif np_data.dtype == py.numpy.int32
      data = get_np_data_item(np_data, @int32);
  elseif np_data.dtype == py.numpy.int64
    try
      data = get_np_data_item(np_data, @int64);
    catch IGNORE_ME
      data = get_np_data_item(np_data, @int32);
    end
  elseif np_data.dtype == py.numpy.uint16 
      data = get_np_data_item(np_data, @uint16);
  elseif np_data.dtype == py.numpy.uint32
      data = get_np_data_item(np_data, @uint32);
  elseif np_data.dtype == py.numpy.uint64
      data = get_np_data_item(np_data, @uint64);
  elseif string(py.str(py.type(np_data.dtype))) == "<class 'numpy.dtype[str_]'>"
    data = string(py.str(np_data));
  else
     t_str = string(py.str(np_data.dtype));
     ME = MException("get_np_data:TyeNotImplemented", "datatype %s not implemented", t_str); 
     throw(ME)
  end
end

function data = get_np_double_data(np_data)
  try
    data = double(np_data);
  catch ME
    if (strcmp(ME.identifier, 'MATLAB:invalidConversion'))
      data = double(py.array.array('d', py.numpy.nditer(np_data)));
      shape = int32(np_data.shape);
      if (length(shape) > 1)
        data = reshape(data, shape);
      end
    else
      throw(ME)
    end
  end
end

function result = get_python_array_type_from_matlab_type_function(type_function)
  switch class(type_function(1))
    case 'double'
      result = 'd';
    case 'int32'
      result = 'i';
    case 'uint8'
      result = 'B';
    case 'uint16'
      result = 'H';
    case 'uint32'
      result = 'I';
    case 'uint64'
      result = 'L';
    otherwise
      tfunction_string = strip(formattedDisplayText(type_function));
      ME = MException("get_np_data:TyeNotImplemented", "datatype %s not implemented", tfunction_string); 
      throw(ME); 
  end
end

function data = get_np_data_item(np_data, type_func)
  try
    data = type_func(np_data);
  catch ME
    if (strcmp(ME.identifier, 'MATLAB:invalidConversion'))
      type_option = get_python_array_type_from_matlab_type_function(type_func);
      data = type_func(py.array.array(type_option, py.numpy.nditer(np_data)));
      shape = int32(np_data.shape);
      if (length(shape) > 1)
        data = reshape(data, shape);
      end
    else
      throw(ME)
    end
  end
end