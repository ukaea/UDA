% NOTE this is deprecated, use the matpyuda matlab module instead
% to use anyway: the individual functions need to be broken out into separate .m files
% the commands in lines 5-9 can then be run in the matlab interpreter

client = py.pyuda.Client();
client2 = py.mast.MastClient(client);
signals = client2.list_signals(alias='epm', shot=47125);
for i=1:length(signals);msignals(i)=string(signals{i}.signal_name);end
for i=1:length(msignals); a=client.get(msignals(i), '47125'); r(i) = get_matuda_signal(a); r(i).signal_name=msignals(i); end;


classdef UdaSignal
  properties 
    data
    dims UdaSignalDim
    description
    units
    label
    signal_name
  end
end

classdef UdaSignalDim
  properties
    data
    label
    units
  end
end

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
      data = int64(np_data);
  elseif string(py.str(py.type(np_data.dtype))) == "<class 'numpy.dtype[str_]'>"
    data = string(py.str(np_data));
  else
     t_str = string(py.str(np_data.dtype));
     ME = MException("get_np_data:TyeNotImplemented", "datatype %s not implemented", t_str); 
     throw(ME)
  end
end

function s = get_matuda_signal(pyuda_signal)
  s = UdaSignal;
  s.data = get_np_data(pyuda_signal.data);
  s.label = get_optional_string(pyuda_signal.label);
  s.units = get_optional_string(pyuda_signal.units);
  s.description = get_optional_string(pyuda_signal.description); 
  for i = 1:length(pyuda_signal.dims)
    s.dims(i) = UdaSignalDim;
    s.dims(i).data = get_np_data(pyuda_signal.dims{i}.data);
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
