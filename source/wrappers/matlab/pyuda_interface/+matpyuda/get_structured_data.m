%% This may be slow! maybe better to transverse the python object and only extract the matlab data required... 
%
%% better to replace two-level system with a check for pyuda type StructuredData. Then only need a single recursive function.
%
%% top-level function to filter whether the returned structured object has a top-level data field
%% before the usual "children" fields are 
%function s = get_uda_structured_data(pyuda_object)
%  if py.hasattr(pyuda_object, "data")
%    atts = fieldnames(pyuda_object);
%    protected_fields = ["data"];
%    for i = 1: numel(atts)
%      if any(strcmp(protected_fields, atts(i)))
%        continue
%      end
%      name = atts{i};
%      %disp(name);
%      s.(name) = matpyuda.get_attribute_value(pyuda_object.(name));
%    end
%    s.data = get_uda_structured_data2(pyuda_object.data);
%  else
%    s = get_uda_structured_data2(pyuda_object);
%  end
%end 
%
%
%% main recursive unpacking function to generate the matlab structured object
%function s = get_uda_structured_data2(pyuda_object)
%  s = struct;
%  atts = fieldnames(pyuda_object);
%  protected_fields = ["children"];
%  for i = 1: numel(atts)
%    if any(strcmp(protected_fields, atts(i)))
%      continue
%    end
%    name = atts{i};
%    %disp(name);
%    s.(name) = matpyuda.get_attribute_value(pyuda_object.(name));
%  end
%  if (py.hasattr(pyuda_object, "children") and length(pyuda_object.children) > 0 )
%    n_children = length(pyuda_object.children);
%    for i = n_children:-1:1
%      s.children(i) = get_uda_structured_data2(pyuda_object.children{i});
%    end
%  end
%end

% single function implementation. currently slow...
% think need to create lists of sub-structures and only traverse down the level once that node is otherwise complete
% otherwise visiting each node multiple times.

function s = get_structured_data(pyuda_object)
  s = struct;
  atts = fieldnames(pyuda_object);
  for i = 1: numel(atts)
    name = atts{i};
    if isa(pyuda_object.(name), 'py.list') &&  length(pyuda_object.(name)) == 0
        continue
      end
    if isa(pyuda_object.(name), 'py.pyuda._structured.StructuredData')
      s.(name) = matpyuda.get_structured_data2(pyuda_object.(name));
    elseif isa(pyuda_object.(name), 'py.list')  && isa(pyuda_object.(name){1}, 'py.pyuda._structured.StructuredData')
      n_children = length(pyuda_object.(name));
      for j = n_children:-1:1
        pyuda_struct_list = pyuda_object.(name);
        s.(name)(i) = matpyuda.get_structured_data2(pyuda_struct_list{i});
      end
    else
      s.(name) = matpyuda.get_attribute_value(pyuda_object.(name));
    end
  end
end