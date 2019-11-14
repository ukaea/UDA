function listcpfcolumns, table=table, verbose=verbose

    if undefined(table) then table = 'mast0'
    if undefined(verbose) then verbose = 0B

    url_obj = getcpfconnection(verbose=verbose)

    url_path = 'api/columns?table='+strtrim(table, 2)

    if verbose then print, url_path

    url_obj->setProperty, url_path=url_path

    query_json = url_obj->get(/string_array, /buffer)
    query_results = json_parse(strjoin(query_json), /tostruct, /toarray)

    return, query_results

end