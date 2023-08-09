function tests = TestMatpyudaMast
    tests = functiontests(localfunctions);
end

function test_base_client_constructor(testCase)
    try
        client = matpyuda.Client();
        verifyInstanceOf(testCase, client, ?matpyuda.Client);
    catch ME
        verifyFail(testCase);
    end
end

function test_get_port(testCase)
    client = matpyuda.Client();
    port = client.port();
    verifyClass(testCase, port, ?int32);
end

function test_getset_port(testCase)
    client = matpyuda.Client();
    client.port = 12345;
    port = client.port();
    verifyEqual(testCase, port, int32(12345));
end

function test_get_server(testCase)
    client = matpyuda.Client();
    server = client.server();
    verifyClass(testCase, server, ?string);
end

function test_getset_server(testCase)
    client = matpyuda.Client();
    client.server = "madeup.server.addr";
    server = client.server();
    verifyEqual(testCase, server, "madeup.server.addr");
end

function test_get_property_logical(testCase)
    client = matpyuda.Client();
    p = client.get_property("get_meta");
    verifyTrue(testCase, islogical(p));
end

function test_getset_property_logical(testCase)
    client = matpyuda.Client();
    client.set_property('get_meta', py.False);
    p(1) = client.get_property("get_meta");
    client.set_property('get_meta', py.True);
    p(2) = client.get_property("get_meta");
    verifyEqual(testCase, p, [false, true]);
end

function test_get_property_val(testCase)
    client = matpyuda.Client();
    p = client.get_property("timeout");
    verifyTrue(testCase, isnumeric(p));
end

function test_getset_property_val(testCase)
    client = matpyuda.Client();
    client.set_property('timeout', 0);
    p(1) = client.get_property("timeout");
    client.set_property('timeout', 600);
    p(2) = client.get_property("timeout");
    verifyEqual(testCase, p, int32([0, 600]));
end

function test_get_string(testCase)
    client = matpyuda.Client();
    client.port =56565;
    client.server ="uda2.mast.l";

    s = client.get("help::help()", "");
    verifyClass(testCase, s, ?string);
    
end

function test_get_1d_double_signal(testCase)
    client = matpyuda.Client();
    client.port = 56565;
    client.server= "uda2.mast.l";
    client.set_property('get_meta', py.True);

    ip = client.get('ip', '30420');
    assertClass(testCase, ip, ?matpyuda.Signal);
    verifyEqual(testCase, length(ip.data), 30000);
    verifyEqual(testCase, length(ip.dims), 1);
    verifyEqual(testCase, length(ip.dims.data), 30000);
    verifyEqual(testCase, length(ip.errors), 30000);

    verifyEqual(testCase, ip.label, "Plasma Current");
    verifyEqual(testCase, ip.units, "kA");
    verifyEqual(testCase, ip.description, "");

    assertClass(testCase, ip.meta, ?struct);
    verifyEqual(testCase, ip.meta.exp_number, int32(30420));
end

%function test_get_signal_meta(testCase)
%    
%end

function test_get_2d_double_signal(testCase)
    client = matpyuda.Client();
    client.port = 56565;
    client.server= "uda2.mast.l";
    client.set_property('get_meta', py.True);

    data = client.get('ayc_ne', '27999');
    assertClass(testCase, data, ?matpyuda.Signal);
    verifyEqual(testCase, size(data.data), [89, 130]);
    verifyEqual(testCase, length(data.dims), 2);
    verifyEqual(testCase, length(data.dims(1).data), 89);
    verifyEqual(testCase, length(data.dims(2).data), 130);
    verifyEqual(testCase, size(data.errors), [89, 130]);

    verifyEqual(testCase, data.label, "electron density");
    verifyEqual(testCase, data.units, "m^-3");
    verifyEqual(testCase, data.description, "");

    assertClass(testCase, data.meta, ?struct);
    verifyEqual(testCase, data.meta.exp_number, int32(27999));

end

function test_get_1d_int_signal(testCase)
  client = matpyuda.Client();
  client.port = 56565;
  client.server= "uda2.mast.l";
  client.set_property('get_meta', py.True);
  data = client.get('EPM/OUTPUT/NUMERICALDETAILS/MAXIMUMITERATIONCOUNT', 45125);
  verifyEqual(testCase, length(data.data), 30);
  verifyEqual(testCase, data.data,int32([ 0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15, 16, ...
                                   17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29]));
end

%function test_get_2d_int_signal(testCase)
%    
%end

function test_get_structured(testCase)
    client = matpyuda.Client();
    client.port = 56565;
    client.server= "uda2.mast.l";
    client.set_property('get_meta', py.True);

    data = client.get('ane', '45125');
    verifyEqual(testCase, data.children.numEntries, 3);
    verifyEqual(testCase, length(data.children('density').data), 80000);
    verifyEqual(testCase, length(data.children('CO2').data), 80000);
    verifyEqual(testCase, length(data.children('HeNe').data), 80000);
end

function test_get_video(testCase)
    client = matpyuda.Client();
    client.port = 56565;
    client.server= "uda2.mast.l";
    client.set_property('get_meta', py.True);
    data = client.get("NEWIPX::read(shot=47125, ipxtag=rgb, last=0)","");

    verifyEqual(testCase, size(data.frames(1).k), [480, 640]);
    verifyEqual(testCase, length(data.frame_times), 248);
    verifyEqual(testCase, data.filter, "(Da/SS)/C-CXRS/He-CXRS");
end

%% FOLLOWING TESTS REQUIRE MastClient IMPLEMENTATION TO BE UNCOMMENTED IN MATPYUDA PYTOHN PACKAGE
%% ... mast specific stuff eventually needs to move out of this main repo ... 
%
%function test_mast_client_constructor(testCase)
%    try
%        client = matpyuda.MastClient();
%        verifyInstanceOf(testCase, client, ?matpyuda.Client);
%    catch(ME)
%        verifyFail(testCase);
%    end
%end
%
%function test_mast_list_signals(testCase)
%    client = matpyuda.MastClient();
%    client.port = 56565;
%    client.server = "uda2.mast.l";
%
%    python_list = client.python_client.list_signals(alias='ane', shot='27999');
%    matlab_list = client.list_signals(alias='ane', shot='27999');
%
%    verifyEqual(testCase, length(matlab_list), length(python_list));
%    verifyEqual(testCase, matlab_list(1).signal_name, string(python_list{1}.signal_name));
%
%end
%
%function test_mast_geometry_data(testCase)
%    client = matpyuda.MastClient();
%    client.port = 56565;
%    client.server= "uda2.mast.l";
%    client.set_property('get_meta', py.True);
%
%    data = client.geometry("/magnetics/fluxloops", "47699");
%    node = matpyuda.get_node_from_path(data.data, "centrecolumn/f_c_a01/data/coordinate");
%    verifyEqual(testCase, node.r, 0.2738, "AbsTol", 5e-5);
%    verifyEqual(testCase, node.z, 1.2935, "AbsTol", 5e-5);
%end
%
%function test_mast_image_data(testCase)
%    client = matpyuda.MastClient();
%    client.port = 56565;
%    client.server= "uda2.mast.l";
%    client.set_property('get_meta', py.True);
%
%    data = client.get_images("rgb", "47125", last_frame=0);
%    verifyEqual(testCase, size(data.frames(1).k), [480, 640]);
%    verifyEqual(testCase, length(data.frame_times), 248);
%    verifyEqual(testCase, data.filter, "(Da/SS)/C-CXRS/He-CXRS");
%end