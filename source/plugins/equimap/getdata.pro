;---------------------------------------------------------------------------------
; Function: GETITEMS
; Version : 1.00
; Arthur  : R Martin
; Date    : 19.11.07
;---------------------------------------------------------------------------------
; GETITEMS
;
; Returns a list a data items in a given ida-file.
;
; Calling Sequence
;
;   trace=GETIDAMITEMS(filename, count=count)
;
;   traces    - String Array - A list of ida-trace names found in the
;               give ida file.
;   filename  - String ida filename with full path
;   count     - Optional parameter, number of items returned
;
;---------------------------------------------------------------------------------
;

function getidamitems, sourcearg, count=count, filetype=filetypearg

  print, sourcearg[0]

  pos=strpos(sourcearg[0], /reverse_search, '.')
  type=strmid(sourcearg[0], pos)

  pos=strpos(sourcearg[0], /reverse_search, '/')
  prefix=strmid(sourcearg[0], pos, 4)

  if (type eq '.nc') then begin
    spawn, 'h5raw '+sourcearg[0], output, erroutput, count=conut
    ord=where(strpos(output, prefix) eq 0 and strlen(output) gt 5, count)
    if (count eq 0) then return, ''

    return, output[ord]
  endif

  if (type eq '.ipx') then begin
    count=0
    return, ''
  endif

  return, get_items(sourcearg[0])

errorcatch:

  print, 'GETDATA: '+errmsg
  count=0

  if is_structure(header) then  rc=freeidam(header.handle)

  return, ''

end

;--------------------------------------------------------------------------
; Function: VARTYPE (Version 2.10)			R.Martin Jan.2000
;
; Updated to include new variable types available in IDL5
;
; Calling Format:
;
;   1.result=VARTYPE(var)
;
;	The return value in this case is a string containing the IDL data
;	type in UPPER case letters. If the varibale is not defined the 
;	string contains 'NotDefined'. If the parameter {var} is a struture
;	whose first tag is UTYPE the return value is the contents of the
;	field UTYPE - the returned string is in LOWER case.
;
;   2.result=VARTYPE(var, /real)
;
;	The return value is TRUE if {var} is a number of any type,
;	except complex.
;
;   3.result=VARTYPE(var, /integer)
;
;	The return value is TRUE if {var} is an integer of any type,
;	eg. BYTE, INTEGER, LONG
;
;--------------------------------------------------------------------------

function vartype, 		$;
	arg,          		$;
	integer=integer,	$;
	notinteger=notinteger,	$;
	real=real,		$;
	notreal=notreal,	$;
	string=strkey,		$;
	notstring=notstrkey,	$;
        structure=structure,	$;
	version=dver
	
;-------------------------------------------------------------------------
; {Check input flags /VERSION & /HELP}
;

  if (keyword_set(dver)) then begin
    print, 'Function: VARTYPE (Version 2.10)'
  endif

;--------------------------------------------------------------------------
; {Check for input flags /INTEGER & /REAL}
;

  ntype=size(arg)
  ntype=ntype(ntype(0)+1)

  if keyword_set(strkey) then return, (ntype eq 7)
  if keyword_set(notstrkey) then return, (ntype ne 7)

  opt=byte([0,1,1,1,0,0,0,0,0,0,0,0,1,1,1,1])
  if (keyword_set(integer)) then return, opt(ntype)
  if (keyword_set(notinteger)) then return, 1-opt(ntype)

  opt=byte([0,1,1,1,1,1,0,0,0,0,0,0,1,1,1,1])
  if (keyword_set(real)) then return, opt(ntype)
  if (keyword_set(notreal)) then return, 1-opt(ntype)

  if (keyword_set(structure)) then begin
    return, (ntype eq 8)
  endif

;--------------------------------------------------------------------------
; {Determine variable type and return as string}
;

  vtype=['NotDefined', 'BYTE', 'INTEGER*2', 'INTEGER*4', 'REAL*4', 	$
	'REAL*8', 'COMPLEX*4', 'STRING', 'STRUCTURE', 'COMPLEX*8',	$
	'POINTER', 'OBJECT', 'BYTE*2', 'BYTE*4', 'INTEGER*8', 'BYTE*8']
  vtype=vtype(ntype)

  if (vtype eq 'STRUCTURE') then begin
    temp=tag_names(arg)
    if (temp(0) eq 'UTYPE') then begin
      intype=size(arg.utype)
      intype=intype(intype(0)+1)

      if (intype eq 7) then return, 'UTYPE:'+strlowcase(arg(0).utype(0))
    endif

    temp=tag_names(arg, /struc)
    if (temp ne '') then return, 'STRUCTURE:'+strlowcase(temp)
  endif

  return, vtype

end

;----------------------------------------------------------------------
; Notes: The function SIZE returns information about a variable in a
; INT*4(n) where the n-2 element gives the variable type.
;
;     Number	Type   	
;	0	Undefined
;	1	Byte
;	2	Int*2
;	3	Int*4
;	4	Real*4
;	5	Real*8
;	6	complex
;	7	String
;	8	Structure
;
;----------------------------------------------------------------------
; Modification
;
;07.01.00 
; - output for UTYPE updated, now returns 'UTYPE:name'
; - output for fixed structure updated, now returns 'STRUCTURE:name'
; - help option removed
; - New variable available in IDL5 included
; - INT/REAL options recoded
;
;12.02.99 - Return STRUCTURE name
; 8.12.97 - Add /NOTreal and /NOTinteger options


;----------------------------------------------------------------------------
; Routine: GET_IDA (Version 5.11) 			R.Martin 29/01/2001
;
; Function to read IDA3 data. Note the default search path is now the
; local directory only.
;
; Format: 
;
; 1) Reading in a single data item
;
;   result=GET_IDA(shot=shotno, tracename, path=path)
;   result=GET_IDA(file=filename, tracename)
;
;   result    - data structure, for more info see 'data_structures'
;   filename  - string containing full filename eg 'MAST$DAT:XMA0027.35'
;   tracename - data item name, string.
;   shotno    - Integer shot number of IDA file.
;
;   path      - Optional parameter: string array containing a list of
;		directories to be search for the IDA-file.
;
; 2) Reading in multiple data items
;
;   result=GET_IDA(shot=shotno, tracelist, /point)
;
;   result    - array of pointers to read in data structures. NULL-pointer
;		indicates a read error
;   tracelist - string array of data item names
;   shotno    - Integer shot number of IDA-file.
;   
;   
;
;
; Notes:
;   1) EFIT data: a patch has been included so that if the number of domains
;   specified in the item header doesn't match the number written the data 
;   can still be read. 
;
;   2) File information: The item FINFO has been added to the return 
;   structure. This is a structure containing
;	FILENAME - the origonal filename when created.
;	TIME     - the creation time
;       DATE     - the creation date
;	VERSION	 - the file version. This is taken as the first element
;		of the first data item containing VERSION in the item name.
;	STATUS	 - ** No Status is returned if there is none in the file **
;                  otherwise the file status. -1 bad, 0 needs checking, 1 could
;                  be OK. 
;
;   3) GET_IDA can now read a 2D data array. The return structure is either
;   a LONGBLOCK or SHORTBLOCK depending on read settings, and contains
;   an additional items Y & YINFO. Y is a real array containing y-axis
;   position for each row of the data array. YINFO is a structure containing
;	YUNITS	 - Axis units, string.
;	YLABEL   - Axis label, string.
;
;
;
;
;   4) Optional Parameters
;      /raw              - return raw uncalibrated data, the bits!
;      error=errorcode   - Non-zero if an error reading data occurs.
;      /nomessage        - don't display messages when reading data.
;
;
;
;
;----------------------------------------------------------------------------
;

function get_ida, 		$;
	arg1, arg2, arg3,	$; List of trace names
	path=path,		$; Optional path (Default '')
	filename=filename,  	$; Input filename, including path
	shot=shotno,		$; Shot Number
	raw=raw,		$;
	nan=nan,		$;
	shorttrace=strace,	$;
	maketrace=maketrace,	$;
	point=point,		$;
	echo=echo,		$;
	nomessage=nomessage,	$;
	version=dver,		$; Show version number
	error=errno		 ; Return error number

;------------------------------------------------------------------------------

  if (keyword_set(dver)) then begin
    return, 'Function: GET_IDA (Version 5.10)'
  endif

;------------------------------------------------------------------------------
; MAKETRACE option: return a blank trace structure to be filled by user
;

  if (keyword_set(maketrace)) then begin
    if (vartype(arg1, /notint)) then begin
      errno=20
      goto, errorcatch
    endif
    isize=long(arg1(0))

    if (isize le 0) then begin
      errno=21
      goto, errorcatch
    endif

    if (vartype(arg2, /notint)) then nodoms=1 else nodoms=arg2(0)
    tinfo={utype:'tinfo',		$
	    nodoms:nodoms,		$
	    tstart: fltarr(nodoms),	$
	    tfinish: fltarr(nodoms),	$
	    tstep: fltarr(nodoms),	$
	    nsamples: lonarr(nodoms),	$
	    tunits: '',			$
	    tlabel:''}

    dinfo={units:'', label:'', class:0}

    if (keyword_set(strace)) then begin
      trace={utype:'shorttrace',		$;
	    	data: fltarr(isize),		$;
		tinfo: tinfo,			$;
		dinfo: dinfo,			$;
		size: isize,	       		$;
		strshot:'',			$;
		chname: '', 			$;
		shot: 0L,			$;
		filename:''}		

    endif else begin
      trace={utype:'longtrace',			$;
		data: fltarr(isize),		$;
		time: fltarr(isize),		$; 
		tinfo: tinfo,			$;
		dinfo: dinfo,			$;
		size: 2*isize,			$;
		strshot:'',			$;
		chname: '', 			$;
		shot: 0L,			$;
		filename:''}		
    endelse

    return, trace
  endif


;------------------------------------------------------------------------------
; CHANNAME: Checks input parameter {channame} is a string array. 
; (filename, shotno0, test, channame, path) and uses them to create the
; IDA filename.
;

  if (vartype(arg1) ne 'STRING') then begin
    errno=10
    goto, errorcatch
  endif

;----------------------------------------------------------------------------
; PATH: Check input parameter PATH if not defined set to default (DAT:)
;

  if (n_elements(path) gt 0) then begin
    if (vartype(path) ne 'STRING') then begin
      errno=11
      goto, errorcatch
    endif

    tmppath=path
    for i=0, n_elements(path)-1 do begin
      if (strtrim(path(i), 2) ne '') then $
		tmppath(i)=expand_path(path(i))+'/'

;      spawn, 'echo '+path(i), result, /sh
;      tmppath(i)=result
    endfor 

  endif else begin
    tmppath=''
  endelse

;----------------------------------------------------------------------------
; Define filename and channame: This section checks the input 
; (filename, shotno0, test, channame) and uses them to create the
; IDA filename.
;

  if (n_elements(shotno) ne 0) then begin       
    if not(vartype(shotno, /integer)) then begin
      errno=12
      goto, errorcatch
    endif

    prefix=strlowcase(strtrim(arg1, 2))		;remove leading/trailing spaces
    pos=strpos(prefix, '_')			;find underscore
    prefix=strmid(prefix, 0, 3)			;reduce to 3 chars

    ord=where(pos eq 2 or pos eq 3, nchan)
    if (nchan eq 0) then begin
      errno=10
      goto, errorcatch
    endif
        
    el=where(pos eq 2, nel)
    if (nel gt 0) then prefix(el)=strmid(prefix(el), 0, 2)
    tmpchan=strtrim(arg1(ord), 2)
    prefix=prefix(ord)

    tmpfile=prefix+strshot(shotno(0))
    tmpshot=long(shotno(0))
  endif else begin
    if (vartype(filename) ne 'STRING') then begin
      errno=14
      goto, errorcatch
    endif

    nchan=n_elements(arg1)
    tmpchan=strtrim(arg1, 2)
    tmpshot=0L
    tmpfile=replicate(filename(0), nchan)
    tmppath=''
  endelse
  tmpfile=[tmpfile, '']

;---------------------------------------------------------------------------
;
;

  if (keyword_set(point)) then begin
    ptrlist=ptrarr(nchan)
  endif else begin
    ptrlist=ptr_new()
    nchan=1
  endelse


;---------------------------------------------------------------------------
; Read data from IDA file:
;
;

!quiet=1                   
@ida3

  errcount=0       				;number of bad traces
  fileno=0

  for i=0, nchan-1 do begin
    if (fileno eq 0) then begin
      errno=1                    
      ifile=0
      while (ifile lt n_elements(tmppath) and fileno eq 0) do begin
        itmpfile=tmppath(ifile)+tmpfile(i)

        itmpfile=(expand_path(itmpfile(0)))(0)

        if keyword_set(echo) then print, 'fileopen'
        fileno=ida_open(itmpfile, ida_read)
        ifile=ifile+1
      endwhile

      if (fileno ne 0) then begin
;	err=ida_print_file(fileno)
;
        if keyword_set(echo) then print, 'getfinfo'
	err=ida_get_finfo(fileno, fname, ftime, fdate)
	
        if keyword_set(echo) then print, 'findversion'
	itemno=ida_find(fileno, itemname='*version*')
	if (itemno ne 0) then begin
          if keyword_set(echo) then print, 'vstruct'
  	  err=ida_get_structure(itemno, dclass, nodoms, spc, pck)

          if keyword_set(echo) then print, 'vdata'
          err=ida_get_data(itemno, idata, 		$
		ida_d4+ida_real,			$
		dunits, dlabel)

	  version=idata(0)
          err=ida_free(itemno)
	endif else version=0.0
	finfo={filename:fname,		$
		time:ftime,		$
		date:fdate,		$
		version:version}

        if keyword_set(echo) then print, 'findstatus'
	itemno=ida_find(fileno, itemname='*status*')
	if (itemno ne 0) then begin
          if keyword_set(echo) then print, 'vstruct'
  	  err=ida_get_structure(itemno, dclass, nodoms, spc, pck)

          if keyword_set(echo) then print, 'vdata'
          err=ida_get_data(itemno, idata, 		$
		ida_d4+ida_real,			$
		dunits, dlabel)

	  status=idata(0)
          err=ida_free(itemno)

          finfo=create_struct(finfo, 'status', status)
	endif 

        itemno=ida_find(fileno, itemname='time')
        globaltimeflag=(itemno ne 0)

        if (globaltimeflag) then begin
          err=ida_get_data(itemno, globaltime, 		$
		ida_d4+ida_real,			$
		dunits, dlabel)
	endif

      endif else errno=4
    endif
                       
    if (fileno ne 0) then begin 
      errno=2
      if keyword_set(echo) then print, 'findtrace'
      itemno=ida_find(fileno, itemname=tmpchan(i))

      if (itemno ne 0) then begin
        if keyword_set(echo) then print, 'tracestruct'
	err=ida_get_structure(itemno, dclass, nodoms, spc, pck)
        if (dclass eq 0) then goto, endread

        errno=3
	if (cdas_error(err) ne 0) or (nodoms eq 0) then goto, endread

        if keyword_set(echo) then print, 'tracetinfo'
        noerr=execute('err=ida_get_tinfo(itemno, nodoms, '+	$
		'tstart, tstep, tfinish, nsamples, tunits, tlabel)')

        if (n_elements(tstart) ne nodoms) then goto, endread
	if (cdas_error(err) ne 0) or (noerr eq 0) then goto, endread

	if (strupcase(tunits) eq 'S') then tlabel='Time (Sec)'

	tinfo={utype:'tinfo',		$
	    nodoms:nodoms,		$
	    tstart: tstart,		$
	    tfinish: tfinish,		$
	    tstep: tstep,		$
	    nsamples: nsamples,		$
	    tunits: tunits,		$
	    tlabel: tlabel}

        if keyword_set(echo) then print, 'tracetdata'
        if (keyword_set(raw)) then begin
          err=ida_get_data(itemno, idata, 		$
		ida_d4+ida_intg+ida_valu+ida_sgnd, 	$
		dunits, dlabel)
	endif else begin
          err=ida_get_data(itemno, idata, 		$
		ida_d4+ida_real, 			$
		dunits, dlabel)
        endelse

        if (n_elements(idata) eq 0) then goto, endread
        if (size(idata, /n_dim) gt 2) then goto, endread

        update_tinfo=0B
        if (vartype(dunits) ne 'STRING') then goto, endread

        dinfo={units:dunits, label:dlabel, class:dclass, pack:pck}
	totsamp=long(total(nsamples))

	xsize=size(idata)
        if (xsize(0) ne 0) then begin
          xsize=xsize(xsize(0))
        endif else begin
          xsize=n_elements(idata)
        endelse

        if (xsize lt totsamp) then begin
          if not(array_equal(nsamples, 1)) then goto, endread

	  totsamp=xsize
          nodoms=totsamp
	  tstart=tstart(0:(nodoms-1))
	  tfinish=tfinish(0:(nodoms-1))
	  tstep=tstep(0:(nodoms-1))
	  nsamples=nsamples(0:(nodoms-1))
        endif 


        totsamp=n_elements(idata)
	tinfo={utype:'tinfo',		$
	    nodoms:nodoms,		$
	    tstart: tstart,		$
	    tfinish: tfinish,		$
	    tstep: tstep,		$
	    nsamples: nsamples,		$
	    tunits: tunits,		$
	    tlabel: tlabel}

        if (tmpshot eq 0) then begin
          tmpstrshot=tmpfile(i)
        endif else begin
          tmpstrshot=strtrim(abs(tmpshot), 2)
	  if (tmpshot lt 0) then tmpstrshot='Z'+tmpstrshot
        endelse

	if (keyword_set(strace) and (globaltimeflag eq 0)) then begin
          trace={utype:'shorttrace',		$;
	    	data: temporary(idata),		$;
		tinfo: tinfo,			$;
		dinfo: dinfo,			$;
		finfo: finfo,			$;
		size: totsamp,	       		$;
		chname: tmpchan(i), 		$;
		strshot: tmpstrshot,		$;
		shot: tmpshot,			$;
		filename:tmpfile(i)}		

 	endif else begin
	  if (globaltimeflag) then time=globaltime else time=tvec(tinfo)

          trace={utype:'longtrace',		$;
		data: temporary(idata),		$;
		time: time,			$; 
		tinfo: tinfo,			$;
		dinfo: dinfo,			$;
		finfo: finfo,			$;
		size: 2*totsamp,		$;
		chname: tmpchan(i), 		$;
		strshot: tmpstrshot,		$;
		shot: tmpshot,			$;
		filename:tmpfile(i)}		
	endelse  

        if (size(trace.data, /n_dim) eq 2) then begin
          err=ida_get_yinfo(itemno, ystart, ystep, 	$;
		yfinish, ysamp, yunits, ylabel)

	  yinfo={units:yunits, label:ylabel}
          y=ystart+indgen(ysamp>1)*ystep
	  trace=create_struct(trace, 	$
		'yinfo', yinfo,		$
		'y', y)

	  trace.size=n_elements(trace.data)
	  if (keyword_set(strace)) then begin
	    trace.utype='shortblock'
	  endif else begin
	    trace.size=trace.size+n_elements(trace.time)
	    trace.utype='longblock'
	  endelse
	endif
	  

        errno=0
	ptrlist(i)=ptr_new(trace, /no_copy)

endread:
        err=ida_free(itemno)
      endif

      if (tmpfile(i) ne tmpfile(i+1)) then begin
        err=ida_close(fileno)
	fileno=0
      endif
    endif

    case errno of
      0:text='['+itmpfile+', '+tmpchan(i)+']' 
      1:text='['+itmpfile+'] File not found'
      2:text='['+itmpfile+', '+tmpchan(i)+'] Trace not found'
      3:text='['+itmpfile+', '+tmpchan(i)+'] Corrupted data trace'
      4:text='['+itmpfile+'] Not an IDA File/File not found'
    endcase

    if (errno ne 0) then begin
      errcount=errcount+1
    endif

    if not(keyword_set(nomessage)) then print, text
  endfor

!quiet=0

  if keyword_set(point) then return, ptrlist
  if (errcount eq nchan) then begin
    errno=5

    return, 0
  endif

  errno=0

  trace=(*ptrlist(0))
  ptr_free, ptrlist

  return, trace
 
;-------------------------------------------------------------------------
errorcatch:

  case errno of 
    10:text='Parameter [TRACE] not defined correctly'
    11:text='Parameter [PATH] not defined correctly'
    12:text='Parameter [SHOT] not defined correctly'
    13:text='Error extracting file prefix from trace name ['+channame(cnt)+']'
    14:text='Parameter [FILE] not defined correctly'

    20:text='Parameter [totsamples] not defined correctly'
    21:text='Parameter [totsamples] must be greater than zero'
  else:begin
	end
  endcase

  if not(keyword_set(nomessage)) then begin
    print, 'Error in Routine: [GET_IDA (Version 5.11)]'
    print, text
  endif

  if (keyword_set(point)) then return, ptr_new()
  return, 0

end

;-------------------------------------------------------------------------
; Modification History 
;
; 12.02.2001
; Add NOMESSAGE option
;
; 26.09.2001
; Replace SPAWN command with EXPAND_PATH function
;
; 03.09.2001
; - New Error Checks DCLASS=0
;
; 30.01.2001 V5.12
; - Reword EFIT file fix so that 3D data can also be read in.
;
; 29.01.2001 V5.11
; - Use SPAWN command to expand '~'
; - Change from using path to tmppath within code
;
; Version 5.10
; - Add 2D data read facility
;   - New return structures shortBLOCK, longBLOCK
;   - Include YINFO in BLOCK structure
; - Remove test shot option, assumed to be indicated by negative shot numbers
; - Add helpful header
; - Add FINFO to return structures
;
;
; Future mods
; - remove size option
; - Remove explicite reference to tinfo, change to xinfo.
; - MAKETRACE move to seperate routine again.
;


;---------------------------------------------------------------------------------
; Function: GETDATA
; Version : 1.30
; Arthur  : R Martin
; Date    : 20.04.07
;---------------------------------------------------------------------------------
; GETDATA
;
; Function for reading in experimental data, using the IDAM interface.
;
; For more information see: http://fusweb2/rmartin/IDL-Library/getdata.html
;
;
;
; Calling Sequence
;
;   trace=GETDATA(item_name, source)
;
;   trace     - a structure, use the command (MORE, trace) to
;               investigate data structure.
;
;   item_name - string
;   source    - Either integer pulse number for the default experiment
;               or a string containing the URL for the data
;
; Example calls
;
; 1) Reading MAST data
;
;   trace=GETDATA('amb_ccbv03', 14504)
;   trace=GETDATA('amb_ccbv03', 'MAST::14504')
;
; 2) Reading MAST test shot data
;
;   trace=GETDATA('amb_ccbv01', -1120)
;   trace=GETDATA('amb_ccbv01', 'MAST::Z1120')
;   trace=GETDATA('amb_ccbv01', 'z1120')
;
; 3) Reading an IDA file
;
;   trace=GETDATA('amb_ccbv01', 'IDA::$MAST_DATA/14504/LATEST/amb0145.04')
;
;---------------------------------------------------------------------------------
;

function getdata, namearg, sourcearg, pass=passarg, tlast=tlast, tfirst=tfirst, $
                  noecho=noecho, $
                  allowbad=allowbad, $
                  handle=handle, $
                  noidam=noidam
		  
  common loc_getdata, special
  common loc_itemsbuffer, buffer

  if undefined(special) then special=0

  errmsg=''
debug=1
verbose=1		  
  

  if (getenv('NOIDAM') eq '1') then noidam=1B

  if not_string(namearg) then begin
    errmsg='Data item not defined'
    goto, fatalerror
  endif

  if undefined(sourcearg) then begin
    errmsg='Source Argument Undefined'
    goto, fatalerror
  endif

  if not_integer(sourcearg) and not_string(sourcearg) then begin
    errmsg='Source Argument must be either a pulse number or URL-filename'
    goto, fatalerror
  endif

  workname=strtrim(namearg[0],2)
  worksource=strtrim(sourcearg[0],2)
  if (stregex(worksource, '[zZ-][0-9]+') eq 0) then begin
    workname=strlowcase(workname)
    testshot=1B
    shotno=-long(strmid(worksource,1))
    worksource='MAST::Z'+string(-shotno, format='(i05)')

    if (strmid(workname, 0, 1) eq '/') then begin
      if undefined(buffer) then buffer={file:''}

      prefix=strmid(workname, 0, 4)
      filename='$MAST_ZSHOT'+prefix+'z'+string(-shotno, format='(i05)')
      if (buffer.file ne filename) then begin
        void=file_search(filename+'.*', count=count)
        if (count eq 0) then begin
          errmsg='Unable to find Z-file'
          goto, errorcatch
        endif
        pos=strpos(void, '.', /reverse_search)
        type=strmid(void, pos)

        filename=filename+type

        items=getidamitems(filename)
        buffer={file:filename, type:type, items:items}
      endif

      void=where(strlowcase(buffer.items) eq workname, found)
      if (found eq 0) then begin
        errmsg='Item not found in Z-file'
        goto, errorcatch
      endif

      name=buffer.items[void[0]]

      if (buffer.type eq '.nc') then begin
        return, getdata(name, 'NETCDF::'+filename, /noecho)
      endif

      errmsg='Unable to read file type ['+type+']'
      goto, errorcatch
    endif

    prefix=(strlowcase(strmid(strsplit(workname, '_', /extract), 0, 3)))[0]
    filename=['$MAST_CALIB/'+prefix+'/', '$MAST_ZSHOT/']+make_idaname(shotno, workname)

    address='IDA::'+expand_path(filename(~file_test(filename[0])))
    if keyword_set(noidam) then goto, noidam

    source = address
    header = callidam2(workname, address, get_bad=allowbad, /get_timedble, debug=debug, verbose=verbose)
    
  endif else begin
    if is_integer(sourcearg[0]) then begin
      shotno=long(sourcearg[0])
      worksource='MAST::'+string(shotno, format='(i05)')
      if keyword_set(noidam) then begin
        address='$MAST_DATA/'+strtrim(shotno,2)+'/LATEST/'+make_idaname(shotno, workname)
        goto, noidam
      endif

      source = strtrim(shotno,2)
      header = callidam2(workname, shotno, get_bad=allowbad, /get_timedble, debug=debug, verbose=verbose)
            
    endif else begin
      source = worksource
      header = callidam2(workname, worksource, get_bad=allowbad, /get_timedble, debug=debug, verbose=verbose)
    endelse
  endelse
  
;---------------------------------------------------------------------------------------------------

  errmsg = ''
  
  if undefined(header) then begin
    errmsg='Source argument not defined correctly'
    goto, errorcatch
  endif

  if not_structure(header) then begin
    errmsg=''
    rc=geterrormsg(header, errmsg)

    if null_string(errmsg) then errmsg='No Data Found'
    goto, errorcatch
  endif

  if (header.error_code ne 0 or header.handle lt 0) then begin
    if keyword_set(special) then begin
      rc=freeidam(header.handle)
      source = worksource
      header = callidam2(workname, worksource, /get_bad, /get_timedble, debug=debug, verbose=verbose)
      errmsg = 'Bad Data'
    endif else begin
      errmsg='IDAM error ['+header.error_msg+']'
      goto, errorcatch
    endelse
  endif
  
  rank=getrank(header.handle)

;====================================================================================================
; DGMuir 26Feb2010: Modifications required to access Hierarchical Data Structures
  
; Is the Data a Hierarchical Data Type: Test and Register

  udregister = setidamdatatree(header.handle)
  
  if(udregister) then begin  
     dblk = getstruct(workname, source, debug=debug, verbose=verbose, priorhandle=header.handle, /usepriorhandle)  

     if not_structure(dblk) then begin
        errmsg='Unable to read data array'
        goto, errorcatch
     endif
     
     if(dblk.erc ne 0) then return, dblk	; Error
     if(rank eq 0) then return, dblk		; Group level structures have no shape nor coordinates!
     
     data_units = ''
     data_label = ''
     rc=getdataunits(header.handle, data_units)
     rc=getdatalabel(header.handle, data_label)

  endif else begin        
     dblk=getidamdata(header)

     if not_structure(dblk) then begin
        errmsg='Unable to read data array'
        goto, errorcatch
     endif

     data_units = dblk.data_units
     data_label = dblk.data_label

  endelse
    
  data=dblk.data
  
;====================================================================================================  
; Organise the data according to the shape of the dimension coordinates    
  
  if (size(data, /n_dim) lt rank) then begin
    s=replicate(1L, rank)

; DGMuir 16Aug2010: Correct reform to structure array shape
    
    if(udregister) then begin
        for i=0, (rank-1) do begin
           s[i] = getdimnum(header.handle, long(i))
	endfor      
    endif else begin    
       s[0]=size(data, /dim)		;; OK if data is an array but not a top level structure array
    endelse
           
    data=reform(data, s)
  endif
  
;====================================================================================================  
; 08Nov2011 dgm
; Check for String arrays: convert and reduce rank

  stringdata = 0
  if( getdatatype(header.handle) eq getdatatypeid('string') ) then begin
     if(rank eq 1) then begin
        data = string(data)
        stringdata = 1
	rank = 0 
     endif else begin
        xxnum  = n_elements(data[0,*])
	xxdata = strarr(xxnum)
	for i=0, xxnum-1 do xxdata[i] = string(data[*,i])
	data = xxdata
	stringdata = 1
	rank = 0 
     endelse	
  endif   

;====================================================================================================  
; Error data   

  eclass=geterrortype(header.handle)
  if (eclass ne 0) then begin
    edata=getdataerror(header.handle)

    imin=min(edata, max=imax)
    if (imin eq 0.0) and (imax eq 0.0) then begin
      eclass=0
      edata=0.0
    endif else begin
      eclass=1
      edata=reform(edata, size(data, /dim))
    endelse
  endif else edata=0.0

;====================================================================================================  
; Returned structure component naming

  tindex=getorder(header.handle)
  
  if (rank ne 0) and (tindex ge 0) then begin
    dtype=indgen(rank)
    dtype=dtype+(dtype lt tindex)
    dtype[tindex]=0

    order=indgen(rank)

    if keyword_set(tfirst) or keyword_set(tlast) then begin
      map=indgen(rank)
      if keyword_set(tlast) then begin
        map=move+(map ge tindex)
        map[rank-1]=tindex
      endif else begin
        map=map-(map le tindex)
        map[0]=tindex
      endelse

      data=temporary(transpose(data, map))
      if (eclass eq 1) then edata=temporary(transpose(edata, map))

      order=order[map]
      dtype=dtype[map]
    endif
  endif else begin
    order=indgen(rank>1)
    dtype=order+1
  endelse

  if exists(passarg) then begin
    struct={name:workname,          $
            source:worksource,      $
            status:header.status,   $
            data:data,              $
            dunits:data_units,	    $
            dlabel:data_label,      $
            pass:passarg,           $
            eclass:eclass,          $
            edata:edata}
  endif else begin
    struct={name:workname,          $
            source:worksource,      $
            status:header.status,   $
            data:data,              $
            dunits:data_units,      $
            dlabel:data_label,      $
            eclass:eclass,          $
            edata:edata}
  endelse

  ;;if stringdata then rank = rank - 1		; Reduce Rank for string data

  for j=0, (rank-1) do begin
    i = j 
    if(stringdata) then i = j+1 else i = j

    ablk=getidamdimdata(header, order[i])    
    if not_structure(ablk) then begin
      errmsg='Unable to read axis info ['+strtrim(i,2)+']'
      goto, errorcatch
    endif       

;;;;; dgm 26May2010 increase dimensions from 4 to 7

    case dtype[i] of
      0:ablk={time:ablk.dim, tunits:ablk.dim_units, tlabel:ablk.dim_label}
      1:ablk={x:ablk.dim, xunits:ablk.dim_units, xlabel:ablk.dim_label}
      2:ablk={y:ablk.dim, yunits:ablk.dim_units, ylabel:ablk.dim_label}
      3:ablk={z:ablk.dim, zunits:ablk.dim_units, zlabel:ablk.dim_label}
      4:ablk={w:ablk.dim, wunits:ablk.dim_units, wlabel:ablk.dim_label}
      5:ablk={v:ablk.dim, vunits:ablk.dim_units, vlabel:ablk.dim_label}
      6:ablk={u:ablk.dim, uunits:ablk.dim_units, ulabel:ablk.dim_label}
    endcase

    case j of
      0:a0=ablk
      1:a1=ablk
      2:a2=ablk
      3:a3=ablk
      4:a4=ablk
      5:a5=ablk
      6:a6=ablk
    endcase
  endfor

  erc={erc:0, errmsg:errmsg}
      
  case rank of
    0: struct=create_struct(struct, erc)
    1: struct=create_struct(struct, a0, erc)
    2: struct=create_struct(struct, a0, a1, erc)
    3: struct=create_struct(struct, a0, a1, a2, erc)
    4: struct=create_struct(struct, a0, a1, a2, a3, erc)
    5: struct=create_struct(struct, a0, a1, a2, a3, a4, erc)
    6: struct=create_struct(struct, a0, a1, a2, a3, a4, a5, erc)
    7: struct=create_struct(struct, a0, a1, a2, a3, a4, a5, a6, erc)
    else: begin 
      errmsg='Too many dimensions for GETDATA'
      goto, errorcatch
    end
  endcase

  if ~keyword_set(noecho) then print, '['+strtrim(worksource, 2)+', '+namearg[0]+'] '+errmsg

  if arg_present(handle) then handle=header else rc=freeidam(header.handle)

  return, struct

errorcatch:

  if ~keyword_set(noecho) then print, '['+worksource+', '+namearg[0]+'] '+errmsg
  return, {erc:-1, errmsg:errmsg}

fatalerror:

  print, 'GETDATA: '+errmsg
  print, 'GETDATA: Calling format - trace=GETDATA(channel_name, source)'

  return, {erc:-1, errmsg:errmsg}

noidam:

  ;;errmsg='No IDAM!'		; When the get_ida function is not available!
  ;;to, errorcatch


  trace=get_ida(file=address, workname, error=error, /nomessage)
  if (error ne 0) then begin
    errmsg='IDA read error'
    goto, errorcatch
  endif

  struct={name:workname,            $
          source:worksource,        $
          status:1,                 $
          data:trace.data,          $
          dunits:trace.dinfo.units, $
          dlabel:trace.dinfo.label, $
          pass:0,             $
          eclass:0,                 $
          edata:0}

  a0={time:trace.time, tunits:'S', tlabel:'Time (Sec)'}
  rank=size(trace.data, /n_dim) 
  if (rank gt 1) then a1={x:trace.x, xunits:trace.xinfom_units, xlabel:ablk.dim_label}
  if (rank gt 2) then a2={y:ablk.dim, yunits:ablk.dim_units, ylabel:ablk.dim_label}

  erc={erc:0, errmsg:errmsg}
      
  case rank of
    0: struct=create_struct(struct, erc)
    1: struct=create_struct(struct, a0, erc)
    2: struct=create_struct(struct, a0, a1, erc)
    3: struct=create_struct(struct, a0, a1, a2, erc)
  endcase
      
  return, struct      

end
