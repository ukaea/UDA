function node = get_node_from_path(input_node, pathstr)
    tokens = split(pathstr, "/");
    try
        node = input_node.children(tokens(1));
    catch ME
        throw(ME)
    end

    if length(tokens) > 1
        try
            node = get_node_from_tokens(node, tokens(2:end));
        catch ME
        throw(ME)
        end
    end
end

function node = get_node_from_tokens(input_node, tokens)
    try
        node = input_node.children(tokens(1));
    catch ME
        throw(ME)
    end

    if length(tokens) > 1
        try
            node = matpyuda.get_node_from_tokens(node, tokens(2:end));
        catch ME
        throw(ME)
        end
    end
end