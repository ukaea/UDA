classdef MastClient < matpyuda.Client
  methods
    function obj = MastClient()
        c0 = py.matpyuda.MastClient();
        obj = obj@matpyuda.Client(c0);
    end

    function result = convert_pyuda_obj_to_matlab_type(obj, pyobj)
      if isa(pyobj,'py.mast.geom.geometry.GeometryData')
        result = matpyuda.get_structured_data(pyobj);
      else
        result = convert_pyuda_obj_to_matlab_type@matpyuda.Client(obj, pyobj);
      end
    end

    function result = list(obj, options)
        arguments
            obj
            options.list_type = 0
            options.shot = py.None
            options.alias = py.None
            options.signal_type = py.None
            options.signal_search = py.None
            options.description_search = py.None
            options.pass_number = py.None
            options.machine string = 'mastu'
        end
        % args = py.dict(options); % cannot unpack py dict into kwargs through matlab 
        pyobj = obj.python_client.list(list_type=py.int(options.list_type), shot=options.shot, ...
                alias=options.alias, signal_type=options.signal_type, signal_search=options.signal_search, ...
                description_search=options.description_search, pass_number=options.pass_number, machine=options.machine);
        for i = length(pyobj):-1:1
            result(i) = build_struct(pyobj{i});
        end
    end

    function result = list_signals(obj, options)
        arguments
            obj
            options.shot = py.None
            options.alias = py.None
            options.signal_type = py.None
            options.signal_search = py.None
            options.description_search = py.None
            options.pass_number = py.None
            options.machine string = 'mastu'
        end
        % args = py.dict(options); % cannot unpack py dict into kwargs through matlab 
        pyobj = obj.python_client.list_signals(shot=options.shot, alias=options.alias, signal_type=options.signal_type, ...
                                               signal_search=options.signal_search, description_search=options.description_search, ...
                                               pass_number=options.pass_number, machine=options.machine);
        for i = length(pyobj):-1:1
            result(i) = build_struct(pyobj{i});
        end
    end

    function result = get_images(obj, signal, source, options)
      arguments
        obj
        signal
        source
        options.first_frame = py.None
        options.last_frame = py.None
        options.stride = py.None
        options.frame_number = py.None
        options.header_only = py.None
        options.rcc_calib_path = py.None
      end

      kwargs = py.dict(options);
      pyobj = pyrun("r = client.get_images(signal, source, **kwargs)", "r", ...
                    client=obj.python_client, signal=signal, source=source, kwargs=kwargs);
      result = obj.convert_pyuda_obj_to_matlab_type(pyobj);
    end

    function result = geometry(obj, signal, source, options)
      arguments
        obj
        signal
        source
        options.version_config = py.None
        options.version_cal = py.None
        options.no_cal = py.None
        options.cal_source = py.None
        options.cartesian_coords = py.None
        options.cylindrical_coords = py.None
      end

      %pyobj = obj.python_client.geometry(string(signal), string(source), version_config=options.version_config, ...
      %                                   version_cal=options.version_cal, no_cal=options.no_cal, cal_source=options.cal_source, ...
      %                                   cartersian_coords=options.cartesian_coords, cylindrical_coords=optoins.cylindrical_coords)

      kwargs = py.dict(options);
      pyobj = pyrun("r = client.geometry(signal, source, **kwargs)", "r", ...
                    client=obj.python_client, signal=signal, source=source, kwargs=kwargs);
      result = obj.convert_pyuda_obj_to_matlab_type(pyobj);
    end

  end % methods
end

function s = build_struct(pyobj)
  s = struct;
  atts = fieldnames(pyobj);
  for i = 1: numel(atts)
    name = atts{i};
    if py.hasattr(pyobj, name)
      s.(name) = matpyuda.get_attribute_value(pyobj.(name));
    end
  end
end