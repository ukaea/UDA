% This may be slow! maybe better to transverse the python object and only extract the matlab data required... 

% top-level function to filter whether the returned structured object has a top-level data field
% before the usual "children" fields are 
function s = get_uda_structured_data(pyuda_object)
  if py.hasattr(pyuda_object, "data")
    atts = fieldnames(pyuda_object);
    protected_fields = ["data"];
    for i = 1: numel(atts)
      if any(strcmp(protected_fields, atts(i)))
        continue
      end
      name = atts{i};
      %disp(name);
      s.(name) = get_attribute_value(pyuda_object.(name));
    end
    s.data = get_uda_structured_data2(pyuda_object.data);
  else
    s = get_uda_structured_data2(pyuda_object);
  end
end 


% main recursive unpacking function to generate the matlab structured object
function s = get_uda_structured_data2(pyuda_object)
  s = struct;
  atts = fieldnames(pyuda_object);
  protected_fields = ["children"];
  for i = 1: numel(atts)
    if any(strcmp(protected_fields, atts(i)))
      continue
    end
    name = atts{i};
    %disp(name);
    s.(name) = get_attribute_value(pyuda_object.(name));
  end
  if py.hasattr(pyuda_object, "children")
    n_children = length(pyuda_object.children);
    for i = 1: n_children
      s.children(i) = get_uda_structured_data2(pyuda_object.children{i});
    end
  end
end


