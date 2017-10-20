function putdata, arg1, arg2, arg3, arg4,    $
                  channels=channels,         $
                  chunksize=chunksize,       $
                  class=class,               $
                  close=close,               $
                  code=code,                 $
                  comment=comment,           $
                  compression=compression,   $
                  conventions=conventions,   $
                  create=create,             $
                  date=date,                 $
                  debug=debug,               $
                  delete=delete,             $
                  device=device,             $
                  type=type,                 $
                  dimensions=dimensions,     $
                  directory=directory,       $
                  errors=errors,             $
                  exp_number=exp_number,     $
                  fileid=fileid,             $
                  format=format,             $
                  group=group,               $
                  id=id,                     $
                  label=label,               $
                  length=length,             $
                  nocompliance=nocompliance, $
                  notstrict=notstrict,       $
                  offset=offset,             $
                  pass=pass,                 $
                  pulse=pulse,               $
                  range=range,               $
                  resolution=resolution,     $
                  scale=scale,               $
                  serial=serial,             $
                  shot=shot,                 $
                  status=status,             $
                  stepid=stepid,             $
                  time=time,                 $
                  title=title,               $
                  units=units,               $
                  unlimited=unlimited,       $
                  update=update,             $
                  varname=varname,           $
                  verbose=verbose,           $
                  version=version,           $
                  xml=xml,                   $ 
                  name=name,                 $
                  noecho=noecho

  common loc_putdata, putfileid

  if keyword_set(delete) then begin
     print, 'Sorry, no delete function is available atm'
     return, -1
  endif

  ; -----------------------------------------
  ; Check the step id provided
  ; -----------------------------------------
  if not_string(stepid) then begin
     print, 'Please specify the stepId, eg. dimension\n'
     return, -1
  endif

  if keyword_set(create) then stepid = 'create'
  if keyword_set(close) then stepid = 'close'
  if keyword_set(update) then stepid = 'update'

  step = strlowcase(strtrim(stepid[0], 2))

  stepid_options = ['create', 'close', 'update', 'device', $
                    'dimension', 'coordinate', 'variable', $
                    'attribute']
  ind_step = where(step eq stepid_options, count_step)

  if count_step eq 0 then begin
     print, '<< PUTDATA ERROR >> Step Id must be one of: '
     print, '                    create, close, update, device, dimension, coordinate, variable, attribute, signals'
     return, -1
  endif
  
  ; -----------------------------------------
  ; Construct the string to pass to the putdata plugin
  ; -----------------------------------------
  if (step eq 'create') or (step eq 'update') then comm = 'PUTDATA::open(' $
  else if (step eq 'close') then comm = 'PUTDATA::close('                  $
  else if (step eq 'device') then comm = 'PUTDATA::device('                $
  else if (step eq 'dimension') then comm = 'PUTDATA::dimension('          $
  else if (step eq 'coordinate') then comm = 'PUTDATA::coordinate('        $
  else if (step eq 'variable') then comm = 'PUTDATA::variable('            $
  else if (step eq 'attribute') then comm = 'PUTDATA::attribute('          $
  else if (step eq 'group') then comm = 'PUTDATA::group(' ; Was this part of the original putdata?

  ; -----------------------------------------
  ; Filename for create / update. File id otherwise
  ; -----------------------------------------                                          
  if (step eq 'create') or (step eq 'update') then begin
     if not_string(arg1) then begin
        print, '<< PUTDATA ERROR >>: Please provide a filename as the first argument.'
        return, -1
     endif

     comm = comm+'filename='+arg1[0]
  endif else begin
     if exists(fileid) then comm = comm+'fileid='+strtrim(fileid, 2) $
     else if exists(putfileid) then comm = comm+'fileid='+strtrim(putfileid[0], 2) $
     else begin
        print, '<< PUTDATA ERROR >>: No file id was given, and no fileid is stored. Please open the file (either with stepid=create or stepid=update)'
        return, -1
     endelse
  endelse

  ; -----------------------------------------
  ; If this is coordinate then work out what the arguments are 
  ; -----------------------------------------
  if (step eq 'coordinate') then begin
     nargs = (exists(arg1)+exists(arg2)+exists(arg3)+exists(arg4))

     if nargs eq 1 then begin
        ; Data array
        data_to_put = arg1
     endif else if nargs eq 2 then begin
        ; Start & Increment arrays
        starts = arg1
        increments = arg2
     endif else if nargs eq 3 then begin
        if is_integer(arg3) then begin
           ; Start, Increment & Count arrays
           starts = arg1
           increments = arg2
           counts = arg3
        endif else begin
           ; Data array + start & increment
           data_to_put = arg1
           starts = arg2
           increments = arg3
        endelse
     endif else if nargs eq 4 then begin
        data_to_put = arg1
        starts = arg2
        increments = arg3
        counts = arg4
     endif

     if exists(starts) then comm = comm+', starts='+strjoin(strtrim(starts, 2), ';')
     if exists(increments) then comm = comm+', increments='+strjoin(strtrim(increments, 2), ';')
     if exists(counts) then comm = comm+', counts='+strjoin(strtrim(counts, 2), ';')
  endif else if (step eq 'variable') or (step eq 'attribute') then begin
     data_to_put = arg1
     
     if (step eq 'variable') then begin
        if not_string(dimensions) then begin
           print, '<< PUTDATA ERROR >>: dimensions must be defined for variable step.'
           return, -1
        endif

        ; Reverse dimensions (Going from IDL => C)    
        reversed_dim = reverse(strsplit(dimensions, ',', /extract))
        dimensions = strjoin(reversed_dim, ';')
     endif
  endif

  ; -----------------------------------------
  ; If this is writing a device, then
  ; the device name can be passed as the
  ; first argument
  ; -----------------------------------------
  if (step eq 'device') then begin
     if not_string(arg1) and not_string(device) then begin
        print, '<< PUTDATA ERROR >>: No device name was given. Please supply the device name as the first argument.'
        return, -1
     endif

     if is_string(arg1) and not_string(device) then device = strtrim(arg1[0], 2)
  endif

  if (step eq 'dimension') then begin
     if not_integer(arg1) and not_integer(length) and undefined(unlimited) then begin
        print, '<< PUTDATA WARNING >> : No dimension length was given and the /unlimited flag was not set'
        print, '                        Assuming the dimension is unlimited'
        unlimited = 1B
     endif

     if is_integer(arg1) and not_integer(length) then length = strtrim(arg1[0], 2)

     if exists(length) then comm = comm+', length='+strtrim(length[0], 2)
  endif

  ; ----------------------------------
  ; Shot is special: we only need one of exp_number, pulse, shot
  ; ----------------------------------
  if exists(pulse) then exp_number = pulse $
  else if exists(shot) then exp_number = shot  

  if exists(exp_number) then comm = comm+', shot='+strtrim(exp_number[0], 2)

  ; ----------------------------------
  ; Add other parameters
  ; ----------------------------------            
  if exists(channels) then comm = comm+', channels='+strtrim(channels[0], 2)
  if exists(device) then comm = comm+', device='+strtrim(device[0], 2)
  if exists(chunksize) then comm = comm+', chunksize='+strtrim(chunksize[0], 2)
  if exists(class) then comm = comm+', class='+strtrim(class[0], 2)
  if exists(code) then comm = comm+', code='+strtrim(code[0], 2)
  if exists(comment) then comm = comm+', comment='+strtrim(comment[0], 2)
  if exists(compression) then comm = comm+', compression='+strtrim(compression[0], 2)
  if exists(conventions) then comm = comm+', conventions='+strtrim(conventions[0], 2)
  if exists(date) then comm = comm+', date='+strtrim(date[0], 2)
  if exists(dimensions) then comm = comm+', dimensions='+strtrim(dimensions[0], 2)
  if exists(directory) then comm = comm+', directory='+strtrim(directory[0], 2)                                
  if exists(errors) then comm = comm+', errors='+strtrim(errors[0], 2)     
  if exists(format) then comm = comm+', format='+strtrim(format[0], 2)
  if exists(group) then begin
     ; Group name must not have a leading /
     if strmid(group[0], 0, 1) eq '/' then group = strmid(group, 1)

     comm = comm+', group='+strtrim(group[0], 2)
  endif
  if exists(id) then comm = comm+', id='+strtrim(id[0], 2)
  if exists(label) then comm = comm+', label='+strtrim(label[0], 2)
  if exists(name) then comm = comm+', name='+strtrim(name[0], 2)
  if exists(offset) then comm = comm+', offset='+strtrim(offset[0], 2)
  if exists(pass) then comm = comm+', pass='+strtrim(pass[0], 2)
  if exists(range) then comm = comm+', range='+strjoin(strtrim(range, 2), ';')
  if exists(resolution) then comm = comm+', resolution='+strtrim(resolution[0], 2)
  if exists(scale) then comm = comm+', scale='+strtrim(scale[0], 2)
  if exists(serial) then comm = comm+', serial='+strtrim(serial[0], 2)
  if exists(status) then comm = comm+', status='+strtrim(status[0], 2)
  if exists(time) then comm = comm+', time='+strtrim(time[0], 2)
  if exists(title) then comm = comm+', title='+strtrim(title[0], 2)
  if exists(type) then comm = comm+', type='+strtrim(type[0], 2)
  if exists(units) then comm = comm+', units='+strtrim(units[0], 2)
  if exists(varname) then comm = comm+', varname='+strtrim(varname[0], 2)
  if exists(version) then comm = comm+', version='+strtrim(version[0], 2)
  if exists(xml) then comm = comm+', xml='+strtrim(xml[0], 2)

  ; ----------------------------------
  ; Keywords
  ; ----------------------------------
  if (step eq 'create') then comm = comm+', /create'
  if keyword_set(nocompliance) then comm = comm+', /nocompliance'
  if keyword_set(notstrict) then comm = comm+', /notstrict'
  if keyword_set(unlimited) then comm = comm+', /unlimited'  

  ; ----------------------------------
  ; Finish command
  ; ----------------------------------
  comm = comm+')'

  if keyword_set(verbose) or keyword_set(debug) then print, comm

  ; ----------------------------------
  ; Call command
  ; ----------------------------------
  if exists(data_to_put) then begin
     rtn = calluda(comm, data=data_to_put, debug=debug, verbose=verbose)
     
     if rtn.erc ne 0 then begin
        print, '<< PUTDATA ERROR >> : ', rtn.errmsg
        return, rtn.erc
     endif
     
     return, 0
  endif else begin
     rtn = calluda(comm, debug=debug, verbose=verbose)
     
     if rtn.erc ne 0 then begin
        print, '<< PUTDATA ERROR >> : ', rtn.errmsg
        return, rtn.erc
     endif

     if (step eq 'create') or (step eq 'update') and (tag_exists(rtn, 'data') eq 1B) then begin
        fileid = rtn.data[0]
        putfileid = fileid
     endif else if (step eq 'close') then begin
        putfileid = !NULL
     endif

     if keyword_set(verbose) then print, 'Opened file. File index is ', putfileid
     
     return, 0
  endelse

end
