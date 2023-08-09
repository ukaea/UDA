function t = run_matpyuda_timing_tests()
  t = struct;
  tic;
  disp("running client tests")
  t.client_timeit = test_client_instantiation();
  t.client_single = test_client_instantiation2();
  disp("running signal tests 1d")
  t.signal_1d = test_1d_signal();
  disp("running signal tests 2d")
  t.signal_2d = test_2d_signal();
  disp("running striuctured data tests")
  t.structured = test_structured_data();
  disp("running video frame tests")
  t.video_frame = test_video_frame();
  disp("running video (all frames) tests")
  t.video_full = test_video_full();
  disp("running geometry tests")
  t.geometry = test_geometry_magnetics();
  toc
end

function t = test_client_instantiation()
  t = struct;
  pyclient = @() py.pyuda.Client();
  t.python_time = timeit(pyclient);

  simple_client = @() matpyuda.Client();
  t.simple_time = timeit(simple_client);

  mast_client = @() matpyuda.MastClient();
  t.mast_time = timeit(mast_client);
end

function t = test_client_instantiation2()
  t = struct;
  t1 = tic;
  pyclient = py.pyuda.Client();
  t.python_time = toc(t1);

  t2 = tic;
  simple_client = matpyuda.Client();
  t.simple_time = toc(t2);

  t3 = tic;
  mast_client = matpyuda.MastClient();
  t.mast_time = toc(t3);
end

function t = test_generic_get(signal, shots)
  start_time = tic;
  client = matpyuda.MastClient();
  client_time = toc(start_time);
  for i = length(shots):-1:1
    shot = string(shots(i));
    t2 = tic;
    pyobj = client.python_client.get(signal, shot);
    python_times(i) = toc(t2);
    t3 = tic;
    mobj = client.convert_pyuda_obj_to_matlab_type(pyobj);
    conversion_times(i) = toc(t3);
  end
  t = struct;
  t.client_time = client_time;
  t.python_times = python_times;
  t.conversion_times = conversion_times;
  t.get_times = python_times + conversion_times;
end

function t = test_1d_signal()
  t = test_generic_get("ip", ["47125", "47126", "47127", "47128", "47129", "47130"]);
end

function t = test_2d_signal()
  t = test_generic_get("AYC/N_E", ["47125", "47126", "47127", "47128", "47129", "47130"]);
end

function t = test_structured_data()
  t = test_generic_get("ane", ["47125", "47126", "47127", "47128", "47129", "47130"]);
end

function t = test_get_images(signal, shots, last_frame)
  start_time = tic;
  client = matpyuda.MastClient();
  client_time = toc(start_time);
  for i = length(shots):-1:1
    shot = string(shots(i));
    t2 = tic;
    pyobj = client.python_client.get_images(signal, shot, last_frame=last_frame);
    python_times(i) = toc(t2);
    t3 = tic;
    mobj = client.convert_pyuda_obj_to_matlab_type(pyobj);
    conversion_times(i) = toc(t3);
    disp(length(mobj.frames));
  end
  t = struct;
  t.client_time = client_time;
  t.python_times = python_times;
  t.conversion_times = conversion_times;
  t.get_times = python_times + conversion_times;
end

function t = test_video_frame()
  t = test_get_images("rgb", ["47125", "47126", "47127", "47128", "47129", "47130"], int32(0));
end

function t = test_video_full()
  t = test_get_images("rgb", ["47131", "47132", "47133", "47134", "47135", "47136"], py.None);
end

function t = test_geometry(signal, shots)
  start_time = tic;
  client = matpyuda.MastClient();
  client_time = toc(start_time);
  for i = length(shots):-1:1
    shot = string(shots(i));
    t2 = tic;
    pyobj = client.python_client.geometry(signal, shot);
    python_times(i) = toc(t2);
    t3 = tic;
    mobj = client.convert_pyuda_obj_to_matlab_type(pyobj);
    conversion_times(i) = toc(t3);
  end
  t = struct;
  t.client_time = client_time;
  t.python_times = python_times;
  t.conversion_times = conversion_times;
  t.get_times = python_times + conversion_times;
end

function t = test_geometry_magnetics()
  t = test_geometry("/magnetics/fluxloops", ["47131", "47132", "47133", "47134", "47135", "47136"]);
end