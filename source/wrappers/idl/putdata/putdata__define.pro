
function putdata::close

  if self.fileid lt 0 then message, 'putdata::close << ERROR >> File is not open, cannot close.'

  ; Close file
  comm = 'PUTDATA::close(fileid='+strtrim(self.fileid, 2)+')'

  rtn = calluda(comm, debug=self.debug, verbose=self.verbose)

  if rtn.erc ne 0 then begin
     print, 'pudata::init << ERROR >> : ', rtn.errmsg
     return, -1
  endif

  ; Reset fileid and list of created dims
  self.fileid = -1 
  if self.created_dims ne !NULL then *self.created_dims = []

  return, 0
end

function putdata::add_attribute, data=data,       $
                                 name=name,       $
                                 varname=varname

  if self.fileid lt 0 then message, 'putdata::add_attribute << ERROR >> File is not open.'

  self->get_group, name, name=attname, group=group

  comm = 'PUTDATA::attribute(fileid='+strtrim(self.fileid, 2) $
         +', name='+attname $
         +', group='+group  

  if exists(varname) then comm = comm + ', varname='+varname 

  comm = comm + ')'

  rtn = calluda(comm, data=data, debug=self.debug, verbose=self.verbose)

  if rtn.erc ne 0 then begin
     print, 'pudata::add_attribute << ERROR >> : ', rtn.errmsg
     return, -1
  endif

  return, 0                                

end

function putdata::add_device, name=name, $
                              type=type, $
                              id=id,     $
                              serial=serial, $
                              resolution=resolution, $
                              range=range, $
                              channels=channels

  if self.fileid lt 0 then message, 'putdata::add_signal << ERROR >> File is not open.'

  comm = 'PUTDATA::device(fileid='+strtrim(self.fileid,2) $
         +', device='+name 
  if exists(type) then comm = comm + ', type='+type 
  if exists(id) then comm = comm +', id='+id 
  if exists(serial) then comm = comm +', serial='+serial 
  if exists(resolution) then comm = comm +', resolution='+strtrim(resolution,2) 
  if exists(range) then comm = comm +', range='+strjoin(strtrim(range, 2), ';') 
  if exists(channels) then comm = comm +', channels='+strtrim(channels,2)
  comm = comm + ')'

  rtn = calluda(comm, debug=self.debug, verbose=self.verbose)

  if rtn.erc ne 0 then begin
     print, 'pudata::add_device << ERROR >> : ', rtn.errmsg
     return, -1
  endif

  return, 0

end

function putdata::add_errors, errors,    $
                              dims=dims, $
                              name=name, $
                              chunksize=chunksize, $
                              compression=compression, $
                              scale=scale, $
                              offset=offset, $
                              label=label, $
                              title=title, $
                              comment=comment, $
                              units=units, $
                              device=device, $
                              channels=channels, $
                              signal=signal

  if self.fileid lt 0 then message, 'putdata::add_errors << ERROR >> File is not open.'
  if undefined(errors) then message, 'putdata::add_errors << ERROR >> No error data was passed.'

  ; Reverse dimensions (going from IDL => C)
  reversed_dim = reverse(strsplit(dims, ',', /extract))

  size_data = size(errors)
  ndims = size_data[0] 

  if ( ((ndims ne n_elements(reversed_dim)) and (n_elements(reversed_dim) gt 1)) $
     or ((ndims eq 0) and (n_elements(reversed_dim) eq 1) and n_elements(errors) ne 1)) then message, 'putdata::add_signal << ERROR >> The number of dimensions does not match the data shape'  
  dims = strjoin(reversed_dim, ';')

  ; -------------------
  ; Separate the name and group
  self->get_group, name, name=errorname, group=group
  
  ; -------------------
  ; Add errors
  ; -------------------  
  comm = 'PUTDATA::variable(fileid='+strtrim(self.fileid, 2) $
         +', name='+errorname                                  $
         +', dimensions='+dims                               $
         +', group='+group
         
  if exists(chunksize) then comm = comm + ', chunksize=' + strtrim(chunksize,2)
  if exists(compression) then comm = comm + ', compression=' + strtrim(compression,2)
  if exists(scale) then comm = comm + ', scale=' + strtrim(scale,2)
  if exists(offset) then comm = comm + ', offset=' + strtrim(offset,2)
  if exists(label) then comm = comm + ', label=' + strtrim(label,2)
  if exists(title) then comm = comm + ', title=' + strtrim(title,2)
  if exists(comment) then comm = comm + ', comment=' + strtrim(comment,2)
  if exists(units) then comm = comm + ', units=' + strtrim(units,2)
  if exists(device) then comm = comm + ', device=' + strtrim(device,2)
  if exists(channels) then comm = comm + ', channels=' + strtrim(channels,2)

  comm = comm + ')'

  rtn = calluda(comm, data=errors, debug=self.debug, verbose=self.verbose)     
  
  if rtn.erc ne 0 then begin
     print, 'pudata::add_errors << ERROR >> problem creating error signal : ', rtn.errmsg
     return, -1
  endif       

  ; -------------------
  ; Add attribute to the signal
  ; -------------------    
  if exists(signal) then begin
     rtn = self->add_attribute(data=errorname, $
                               name=group+'/errors', $
                               varname=signal)
     
     if rtn ne 0 then begin
        print, 'pudata::add_errors << ERROR >> problem creating error attribute for signal : ', rtn.errmsg
        return, -1
     endif

  endif
                              
  return, 0

end

function putdata::add_signal, data,      $
                              dims=dims, $
                              name=name, $
                              chunksize=chunksize, $
                              compression=compression, $
                              scale=scale, $
                              offset=offset, $
                              label=label, $
                              title=title, $
                              comment=comment, $
                              units=units, $
                              device=device, $
                              channels=channels, $
                              errors=errors, $
                              elabel=errorslabel,     $
                              ecomment=errorscomment

  if self.fileid lt 0 then message, 'putdata::add_signal << ERROR >> File is not open.'
  if undefined(data) then message, 'putdata::add_signal << ERROR >> No data was passed.'

  ; Reverse dimensions (going from IDL => C)
  reversed_dim = reverse(strsplit(dims, ',', /extract))

  size_data = size(data)
  ndims = size_data[0] 

  if ( ((ndims ne n_elements(reversed_dim)) and (n_elements(reversed_dim) gt 1)) $
     or ((ndims eq 0) and (n_elements(reversed_dim) eq 1) and n_elements(data) ne 1)) then message, 'putdata::add_signal << ERROR >> The number of dimensions does not match the data shape'  
  dims = strjoin(reversed_dim, ';')

  ; -------------------
  ; Separate the name and group
  self->get_group, name, name=varname, group=group

  ; -------------------
  ; Create errors, if given
  ; -------------------
  if is_number(errors) then begin

     ; Create variable containing errors : first check if the shape matches
     size_errors = size(errors)
     if n_elements(size_data) ne n_elements(size_errors) then message, 'Errors have a different shape to the data!'
     shape_nomatch = (size_data[0:n_elements(size_data)-2] eq size_errors[0:n_elements(size_errors)-2])
     ind_nomatch = where(shape_nomatch eq 0, count_nomatch)
     if count_nomatch gt 0 then message, 'Errors have a different shape to the data!'

     error_name = varname+'_error'    

     comm = 'PUTDATA::variable(fileid='+strtrim(self.fileid, 2) $
            +', name='+error_name                               $
            +', dimensions='+dims                               $
            +', group='+group                                   

     if exists(errorslabel) then comm = comm+', label='+errorslabel  
     if exists(errorslabel) then comm = comm+', comment='+errorscomment  
     if exists(units) then comm = comm+', units='+units

     comm = comm+')'
     
     rtn = calluda(comm, data=errors, debug=self.debug, verbose=self.verbose)     

     if rtn.erc ne 0 then begin
        print, 'pudata::add_signal << ERROR >> problem creating errors : ', rtn.errmsg
        return, -1
     endif     
  endif else if is_string(errors) then error_name = errors
  
  ; -------------------
  ; Signal itself
  ; -------------------  
  comm = 'PUTDATA::variable(fileid='+strtrim(self.fileid, 2) $
         +', name='+varname                                  $
         +', dimensions='+dims                               $
         +', group='+group
         
  if exists(chunksize) then comm = comm + ', chunksize=' + strtrim(chunksize,2)
  if exists(compression) then comm = comm + ', compression=' + strtrim(compression,2)
  if exists(scale) then comm = comm + ', scale=' + strtrim(scale,2)
  if exists(offset) then comm = comm + ', offset=' + strtrim(offset,2)
  if exists(label) then comm = comm + ', label=' + strtrim(label,2)
  if exists(title) then comm = comm + ', title=' + strtrim(title,2)
  if exists(comment) then comm = comm + ', comment=' + strtrim(comment,2)
  if exists(units) then comm = comm + ', units=' + strtrim(units,2)
  if exists(device) then comm = comm + ', device=' + strtrim(device,2)
  if exists(channels) then comm = comm + ', channels=' + strtrim(channels,2)
  if exists(error_name) then comm = comm + ', errors=' + strtrim(error_name,2)

  comm = comm + ')'

  rtn = calluda(comm, data=data, debug=self.debug, verbose=self.verbose)     
  
  if rtn.erc ne 0 then begin
     print, 'pudata::add_signal << ERROR >> problem creating signal : ', rtn.errmsg
     return, -1
  endif       

  return, 0


end

function putdata::add_dim, values=values,                 $  
                           starts=starts,                 $  
                           increments=increments,         $
                           counts=counts,                 $
                           name=name,                     $ ; Required
                           title=title,                   $ ; Optional : should we make this required?
                           label=label,                   $ ; Optional
                           class=class,                   $ ; Optional
                           comment=comment,               $ ; Optional
                           units=units,                   $ ; Optional
                           chunksize=chunksize,           $ ; Optional
                           compression=compression,       $ ; Optional
                           errors=errors,                 $ ; Optional
                           errorslabel=errorslabel,       $
                           errorscomment=errorscomment


  if self.fileid lt 0 then message, 'putdata::add_dim << ERROR >> File is not open.'

  if undefined(name) then begin
     message, 'You must provide a name for the dimension'
  endif

  ; Check if dimension has already been created
  ind_name = where(*self.created_dims eq name, count_name)
  if count_name ne 0 then message, 'Dimension already exists!'

  ; Get group from the dimension name
  self->get_group, name, name=dimname, group=group

  ; -------------------
  ; Check that we have sufficient info to make a coordinate array
  ; -------------------
  if undefined(values) and undefined(starts) and undefined(increments) and undefined(counts) then begin
     message, 'putdata::add_dim << ERROR >> You must either define values for the dimension, or starts, increments and counts.'
  endif

  if undefined(values) and (undefined(starts) or undefined(increments) or undefined(counts)) then begin
     message, 'putdata::add_dim << ERROR >> If you do not give values for the dimension, you must supply starts, increments and counts.'
  endif else if undefined(values) then begin
     if not_number(starts) then message, 'starts must be a double array'
     if not_number(increments) then message, 'increments must be a double array'
     if not_integer(counts) then message, 'counts must be an integer array'

     starts = double(starts)
     increments = double(increments)
  endif

  if is_string(values) then message, 'putdata::add_dim << ERROR >> Dimensions can-not be string arrays!'
 
  ; Determine dimension length
  dimension_length = 0

  if exists(values) then dimension_length = n_elements(values) $
  else if exists(counts) then dimension_length = total(counts)

  if dimension_length eq 0 then message, 'putdata::add_dim << ERROR >> Could not determine dimension length'

  ; -------------------
  ; Create Dimension
  ; -------------------
  comm = 'PUTDATA::dimension(fileid='+strtrim(self.fileid, 2)        $
                           +', length='+strtrim(dimension_length, 2) $
                           +', name='+dimname                        $
                           +', group='+group                         $
                           +', /unlimited)'

  rtn = calluda(comm, debug=self.debug, verbose=self.verbose)

  if rtn.erc ne 0 then begin
     print, 'pudata::add_dim << ERROR >> : ', rtn.errmsg
     return, -1
  endif

  ; -------------------
  ; Create Coordinate
  ; -------------------
  comm = 'PUTDATA::coordinate(fileid='+strtrim(self.fileid, 2) $
         +', name='+dimname $
         +', group='+group

  ; Add starts, increments and counts if they exist. Arrays must be ; separated
  if exists(starts) then comm = comm+', starts='+strjoin(strtrim(starts, 2), ';')
  if exists(increments) then comm = comm+', increments='+strjoin(strtrim(increments, 2), ';')
  if exists(counts) then comm = comm+', counts='+strjoin(strtrim(counts, 2), ';')

  ; Optional extras
  if exists(chunksize) then comm = comm+', chunksize='+strtrim(chunksize, 2)
  if exists(compression) then comm = comm+', compression='+strtrim(compression, 2)
  if exists(label) then comm = comm+', label='+strtrim(label, 2)
  if exists(class) then comm = comm+', class='+strtrim(class, 2)
  if exists(title) then comm = comm+', title='+strtrim(title, 2)
  if exists(comment) then comm = comm+', comment='+strtrim(comment, 2)
  if exists(units) then comm = comm+', units='+strtrim(units, 2)

  comm = comm+')'

  if exists(values) then begin
     rtn = calluda(comm, data=values, debug=self.debug, verbose=self.verbose)
  endif else begin
     rtn = calluda(comm, debug=self.debug, verbose=self.verbose)
  endelse

  if rtn.erc ne 0 then begin
     print, 'pudata::add_dim << ERROR >> : ', rtn.errmsg
     return, -1
  endif

  *self.created_dims = [*self.created_dims, name]

  ; -------------------
  ; Create errors, if given
  ; -------------------
  if is_number(errors) then begin
     ; Create variable containing errors
     if n_elements(errors) ne (dimension_length) then message, 'Errors have a different dimension to the length!'

     error_name = dimname+'_error'    

     comm = 'PUTDATA::variable(fileid='+strtrim(self.fileid, 2) $
            +', name='+error_name                               $
            +', dimensions='+dimname                            $
            +', group='+group                                   

     if exists(errorslabel) then comm = comm+', label='+errorslabel  
     if exists(errorslabel) then comm = comm+', comment='+errorscomment  
     if exists(units) then comm = comm+', units='+units

     comm = comm+')'
     
     rtn = calluda(comm, data=errors, debug=self.debug, verbose=self.verbose)     

     if rtn.erc ne 0 then begin
        print, 'pudata::add_dim << ERROR >> problem creating errors : ', rtn.errmsg
        return, -1
     endif

     ; Add attribute to the coordinate to say what the errors are
     comm = 'PUTDATA::attribute(fileid='+strtrim(self.fileid, 2) $
            +', group='+group                                    $
            +', name=errors'                                     $
            +', varname='+dimname                                $
            +')'

     rtn = calluda(comm, data=error_name, debug=self.debug, verbose=self.verbose)

     if rtn.erc ne 0 then begin
        print, 'pudata::add_dim << ERROR >> problem creating attribute for errors : ', rtn.errmsg
        return, -1
     endif
     
  endif 


  return, 0B
end

function putdata::open, filename=filename,   $ ; Required
                        directory=directory, $ ; Optional for the moment
                        title=title,         $ ; Required unless update is set
                        comment=comment,     $ ; Optional
                        class=class,         $ ; Required (Raw, Analysed or Modelled), unless update is set
                        shot=shot,           $ ; Required for Raw or Analysed files, unless update is set
                        pass=pass,           $ ; Required for Analysed files, unless update is set
                        status=status,       $ ; Required for Analysed files, unless update is set
                        code=code,           $ ; Optional
                        version=version,     $ ; Optional
                        xml=xml,             $ ; Optional
                        update=update,       $ ; Use keyword if updating rather than creating new file
                        debug=debug,         $ ; Optional
                        verbose=verbose
                        
  if self.fileid ge 0 then message, 'putdata::open << ERROR >> File '+self.filename+' is already open.'                        

  if undefined(filename) or not_string(filename) then message, 'filename must be supplied.'

  self.filename = filename

  ; Conventions
  conventions = 'Fusion Data Conventions 1.1'
  
  ; Get the current time and date
  timedate = systime()
  month = strmid(timedate, 4, 3)
  day = strmid(timedate, 8, 2)
  year = strmid(timedate, 20, 4)
  creation_date = day+' '+month+' '+year
  creation_time = strmid(timedate, 11, 5)

  comm = 'PUTDATA::open(filename='+filename $
         +', date='+creation_date           $
         +', time='+creation_time           $
         +', conventions='+conventions           

  if exists(directory) then begin
;     ; The plugin adds a / to make the full path. If it is given here
;     ; as well it causes problems, so remove trailing slash if given
;     ind_slash = strpos(directory, '/', /reverse_search)
;     if ind_slash eq strlen(directory)-1 then directory = strmid(directory, 0, strlen(directory)-1)

     comm = comm+', directory='+directory
  endif
  if exists(title) then comm = comm+', title='+title
  if exists(comment) then comm = comm+', comment='+comment
  if exists(class) then comm = comm+', class='+class
  if exists(shot) then comm = comm+', shot='+strtrim(shot, 2)
  if exists(pass) then comm = comm+', pass='+strtrim(pass, 2)
  if exists(status) then comm = comm+', status='+strtrim(status, 2)
  if exists(code) then comm = comm+', code='+strtrim(code, 2)
  if exists(version) then comm = comm+', version='+strtrim(version, 2)
  if exists(xml) then comm = comm+', xml='+strtrim(xml, 2)

  if not keyword_set(update) then comm = comm+', /create)' $
  else comm = comm+')'

  ; Open the file
  rtn = calluda(comm, debug=self.debug, verbose=self.verbose)

  if rtn.erc ne 0 then begin
     print, 'pudata::init << ERROR >> : ', rtn.errmsg
     return, -1
  endif

  ; Store file id
  self.fileid = rtn.data[0]

  if keyword_set(verbose) then print, 'pudata::init << INFO >> Opened file '+self.filename, ' : fileid is ', self.fileid

  return, rtn.erc
end

pro putdata::get_group, fullpath, name=name, group=group

  group = '/'

  if strmid(fullpath, strlen(fullpath)-1) eq '/' then fullpath = strmid(fullpath, 0, strlen(fullpath)-1)
  ind_slash = strpos(fullpath, '/', /reverse_search)
  
  if ind_slash ge 0 then begin
     group = strmid(fullpath, 0, ind_slash)
     name = strmid(fullpath, ind_slash+1)
  endif else begin
     name = fullpath
  endelse
end

pro putdata::cleanup

  if self.fileid ge 0 then begin
     rtn = self->close()
  endif

  if self.created_dims ne !NULL then ptr_free, self.created_dims
end

function putdata::init, filename=filename,   $ ; Required
                        directory=directory, $ ; Optional for the moment
                        title=title,         $ ; Required unless update is set
                        comment=comment,     $ ; Optional
                        class=class,         $ ; Required (Raw, Analysed or Modelled), unless update is set
                        shot=shot,           $ ; Required for Raw or Analysed files, unless update is set
                        pass=pass,           $ ; Required for Analysed files, unless update is set
                        status=status,       $ ; Required for Analysed files, unless update is set
                        code=code,           $ ; Optional
                        version=version,     $ ; Optional
                        xml=xml,             $ ; Optional
                        update=update,       $ ; Use keyword if updating rather than creating new file
                        debug=debug,         $ ; Optional
                        verbose=verbose        ; Optional                        

  ; Empty array to store created dimensions
  self.created_dims = ptr_new([])
  self.fileid = -1


  erc = self->open( filename=filename, directory=directory,    $
                    title=title, comment=comment, class=class, $ 
                    shot=shot, pass=pass, status=status,       $
                    code=code, version=version, xml=xml,       $
                    update=update, debug=debug, verbose=verbose)

  if erc ne 0 then return, -1

  return, 1
end

pro putdata__define

  void = { putdata,      $
           fileid: -1,   $
           filename: '', $
           update: 0B,   $
           created_dims: ptr_new(), $
           debug: 0B,    $
           verbose: 0B     $
         }

end
