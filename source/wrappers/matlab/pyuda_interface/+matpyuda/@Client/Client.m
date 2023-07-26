classdef Client
    properties (SetAccess = public)
        port 
        server
    end

    properties (SetAccess = private)
        python_client
    end

    methods
        function obj = Client(varargin)
            if nargin == 0
                obj.python_client = py.matpyuda.Client();
            elseif nargin == 1
                obj.python_client = varargin{1};
            end
        end

        function value = get.port(obj)
            value = int32(obj.python_client.get_port());
        end

        function obj = set.port(obj, number)
            obj.python_client.set_port(int32(number));
        end

        function value = get.server(obj)
            value = string(obj.python_client.get_server());
        end

        function obj = set.server(obj, address)
            obj.python_client.set_server(string(address));
        end

        function value = get_property(obj, name)
            pyobj = obj.python_client.get_property(name);
            if class(pyobj) == "logical"
                value = pyobj;
            else
                value = matpyuda.get_attribute_value(pyobj);
            end
        end

        function set_property(obj, name, value)
            obj.python_client.set_property(string(name), string(value));
        end

        function reset_connection(obj)
            obj.python_client.reset_connection();
        end

        function result = convert_pyuda_obj_to_matlab_type(obj, pyobj)
            if py.str(py.type(pyobj)) == "<class 'pyuda._signal.Signal'>"
                result = matpyuda.get_signal(pyobj);
            elseif py.str(py.type(pyobj)) == "<class 'pyuda._structured.StructuredData'>"
                result = matpyuda.get_structured_data(pyobj);
            elseif py.str(py.type(pyobj)) == "<class 'pyuda._string.String'>"
                result = string(pyobj.str);
            elseif py.str(py.type(pyobj)) == "<class 'pyuda._video.Video'>"
                result = matpyuda.get_video_data(pyobj);
            else
                t_str = string(py.str(pyobj));
                ME = MException("matpyuda.client.get:TyeNotImplemented", "Unexpected return type from pyuda: %s", t_str); 
                throw(ME)
            end
        end

        function result = get(obj, signal, source)
            pyobj = obj.python_client.get(string(signal), string(source));
            result = convert_pyuda_obj_to_matlab_type;
        end

        function result = get_batch(obj, signals, sources)
            pyobj = obj.python_client.get_matlab_batch(signals, sources);
            n_signals = length(pyobj);
            for i = 1:n_signals
              result(i) = convert_pyuda_obj_to_matlab_type(pyobj{i});
            end
        end

        function result = get_text(obj, file)
            pyobj = obj.python_client.get_text(file);
            result = string(pyobj);
        end

        function get_file(obj, file, output_path)
            obj.python_client.get_file(file, output_path)
        end

    end
end