function data = get_np_data(np_data)
  if np_data.dtype == py.numpy.float32
      data = double(np_data);
  elseif np_data.dtype == py.numpy.float64
      data = double(np_data);
  elseif np_data.dtype == py.numpy.int8
      data = int8(np_data);
  elseif np_data.dtype == py.numpy.int32
      data = int32(np_data);
  elseif np_data.dtype == py.numpy.int64
    try
      data = int64(np_data);
    catch IGNORE_ME
      data = int32(np_data);
    end
  elseif string(py.str(py.type(np_data.dtype))) == "<class 'numpy.dtype[str_]'>"
    data = string(py.str(np_data));
  else
     t_str = string(py.str(np_data.dtype));
     ME = MException("get_np_data:TyeNotImplemented", "datatype %s not implemented", t_str); 
     throw(ME)
  end
end
