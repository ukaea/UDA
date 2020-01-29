function getfile, filesource, outputfile

  errmsg=''

  if undefined(filesource) then begin
    errmsg='File Source Argument Undefined'
    goto, fatalerror
  endif

  if not_string(filesource) then begin
    errmsg='File Source Argument must be the URL-filename'
    goto, fatalerror
  endif

  if undefined(outputfile) then begin
    errmsg='Output File Argument Undefined'
    goto, fatalerror
  endif

  if not_string(outputfile) then begin
    errmsg='Output File Argument must be the URL-filename that output file will be written to'
    goto, fatalerror
  endif

  workfilesource=strtrim(filesource[0],2)
  workoutputfile=strtrim(outputfile[0],2)

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

  data=dblk.data

  openw, outfile, workoutputfile, /get_lun

  writeu, outfile, data

  close, outfile

  return, {erc: 0, errmsg: errmsg}

errorcatch:

  if ~keyword_set(noecho) then print, 'file source: '+workfilesource+', output file: '+workoutputfile+'] '+errmsg
  return, {erc:-1, errmsg:errmsg}

fatalerror:

  print, 'GETFILE: '+errmsg
  print, 'GETFILE: Calling format - trace=GETFILE(file_source, output_file)'

  return, {erc:-1, errmsg:errmsg}

end