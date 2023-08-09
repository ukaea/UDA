function s = get_matuda_signal(pyuda_signal)
  s = matpyuda.Signal;
  s.data = matpyuda.get_np_data(pyuda_signal.data);
  s.errors = matpyuda.get_np_data(pyuda_signal.data);
  s.label = get_optional_string(pyuda_signal.label);
  s.units = get_optional_string(pyuda_signal.units);
  s.description = get_optional_string(pyuda_signal.description); 
  s.meta = parse_python_dictionary(pyuda_signal.meta);
  for i = 1:length(pyuda_signal.dims)
    s.dims(i) = matpyuda.SignalDim;
    s.dims(i).data = matpyuda.get_np_data(pyuda_signal.dims{i}.data);
    s.dims(i).label = get_optional_string(pyuda_signal.dims{i}.label);
    s.dims(i).units = get_optional_string(pyuda_signal.dims{i}.units);
  end
end

function s = get_optional_string(field)
  if py.str(py.type(field)) == "<class 'NoneType'>"
    s = "";
  else
    s = string(field);
  end
end

% can replace with struct(py_dict). Matlab will natively parse python dictionaries into matlab structs.
function s = parse_python_dictionary(pyobj)

  % NOTE: pyrun introduced in matlab 2021b. NOT compatible with matlab 2019. 
  % use builtin conversion to struct instead
  %key_names = pyrun("r = [k for k in meta.keys()]", "r", meta=pyobj);

  s = struct(pyobj);
  key_names = fieldnames(s);

  num_fields = length(key_names);
  if num_fields == 0
    return
  end

  for i = 1: num_fields
    name = key_names{i};
    value = pyobj{name};
  %  disp(name);
    try
    s.(string(name)) = matpyuda.get_attribute_value(value);
    catch exception
      if class(value) == "py.bytes"
        s.(string(name)) = string(value.decode());
      end
    end
  end
end