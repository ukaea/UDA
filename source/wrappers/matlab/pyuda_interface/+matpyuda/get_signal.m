function s = get_matuda_signal(pyuda_signal)
  s = matpyuda.Signal;
  s.data = matpyuda.get_np_data(pyuda_signal.data);
  s.label = get_optional_string(pyuda_signal.label);
  s.units = get_optional_string(pyuda_signal.units);
  s.description = get_optional_string(pyuda_signal.description); 
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
