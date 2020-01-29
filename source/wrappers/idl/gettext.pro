function gettext, filesource

  errmsg=''

  if undefined(filesource) then begin
    errmsg='File Source Argument Undefined'
    goto, fatalerror
  endif

  if not_string(filesource) then begin
    errmsg='File Source Argument must be the URL-filename'
    goto, fatalerror
  endif

  workfilesource=strtrim(filesource[0],2)

  header = callidam2('bytes::read(path='+workfilesource+')', '')

  if undefined(header) then begin
    errmsg='File Source argument not defined correctly'
    goto, errorcatch
  endif

  if not_structure(header) then begin
    errmsg=''
    rc=geterrormsg(header, errmsg)

    if null_string(errmsg) then errmsg='No File Found'
    goto, errorcatch
  endif

  if (header.error_code ne 0 or header.handle lt 0) then begin
    errmsg='IDAM error ['+header.error_msg+']'
    goto, errorcatch
  endif

  dblk=getidamdata(header)

  if not_structure(dblk) then begin
     errmsg='Unable to read file'
     goto, errorcatch
  endif

  data=string(dblk.data)

  return, {erc: 0, errmsg: errmsg, text: data}

errorcatch:

  if ~keyword_set(noecho) then print, 'file source: '+workfilesource+'] '+errmsg
  return, {erc:-1, errmsg:errmsg}

fatalerror:

  print, 'GETFILE: '+errmsg
  print, 'GETFILE: Calling format - trace=GETFILE(file_source, output_file)'

  return, {erc:-1, errmsg:errmsg}

end