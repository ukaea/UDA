function getcpf, columns, filters, table=table, physics=physics, plant=plant, verbose=verbose

    if undefined(table) then begin
        if not keyword_set(physics) and not keyword_set(plant) then table='Mast0' $
        else if keyword_set(physics) then table='Mast0'                           $
        else table='Plant'
    endif

    if undefined(verbose) then verbose = 0B

    url_obj = getCPFconnection(verbose=verbose)

    url_path = 'api/query?'
    delim = ''
    for i=0, n_elements(columns)-1 do begin
        url_path = url_path+delim+'columns='+columns[i]
        delim = '&'
    endfor

    for i = 0, n_elements(filters)-1 do begin
        url_path = url_path + delim + 'filters=' + filters[i]
        delim = '&'
    endfor

    if verbose then print, url_path

    url_obj->setProperty, url_path=url_path

    query_json = url_obj->get(/string_array, /buffer)
    query_results = json_parse(strjoin(query_json), /tostruct, /toarray)

    return, query_results

end

