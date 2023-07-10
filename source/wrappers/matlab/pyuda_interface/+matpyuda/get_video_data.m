function result = get_video_data(pyobj)
  result = struct;
  atts = fieldnames(pyobj);
  protected_fields = ["frames"];
  for i = 1: numel(atts)
    if any(strcmp(protected_fields, atts(i)))
      continue
    end
    name = atts{i};
  %  disp(name);
    result.(name) = matpyuda.get_attribute_value(pyobj.(name));
  end
  if py.hasattr(pyobj, "n_frames")
    for i = 1:int32(pyobj.n_frames)
      result.frames(i) = matpyuda.get_video_data(pyobj.frames{i});
    end
  end
end