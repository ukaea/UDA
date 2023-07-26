classdef MastClient < matpyuda.Client
  methods
    function obj = MastClient()
        c0 = py.pyuda.Client();
        c1 = py.mast.MastClient(c0);
        obj = obj@matpyuda.Client(c1);
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
        for i = 1:length(pyobj)
            result(i) = struct(pyobj{i});
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
        pyobj = obj.python_client.list_signals(shot=options.shot, ...
                alias=options.alias, signal_type=options.signal_type, signal_search=options.signal_search, ...
                description_search=options.description_search, pass_number=options.pass_number, machine=options.machine);
        for i = 1:length(pyobj)
            result(i) = build_struct(pyobj{i});
        end
        
    end

  end
end

function s = build_struct(pyobj)
  s = struct;
  atts = fieldnames(pyobj);
  for i = 1: numel(atts)
    name = atts{i};
    s.(name) = matpyuda.get_attribute_value(pyobj.(name));
  end
end