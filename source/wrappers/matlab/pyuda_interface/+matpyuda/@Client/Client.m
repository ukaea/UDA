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

        function result = get(obj, signal, source)
            pyobj = obj.python_client.get(string(signal), string(source));
            if py.str(py.type(pyobj)) == "<class 'pyuda._signal.Signal'>"
                result = matpyuda.get_signal(pyobj);
            elseif py.str(py.type(pyobj)) == "<class 'pyuda._structured.StructuredData'>"
                result = matpyuda.get_structured_data(pyobj);
            elseif py.str(py.type(pyobj)) == "<class 'pyuda._string.String'>"
                result = string(pyobj.str);
            else
                t_str = string(py.str(pyobj));
                ME = MException("matpyuda.client.get:TyeNotImplemented", "Unexpected return type from pyuda: %s", t_str); 
                throw(ME)
            end
        end

    end
end