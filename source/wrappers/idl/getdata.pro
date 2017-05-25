;--------------------------------------------------------------------------
; Function: VARTYPE (Version 2.10)            R.Martin Jan.2000
;
; Updated to include new variable types available in IDL5
;
; Calling Format:
;
;   1.result=VARTYPE(var)
;
;    The return value in this case is a string containing the IDL data
;    type in UPPER case letters. If the varibale is not defined the 
;    string contains 'NotDefined'. If the parameter {var} is a struture
;    whose first tag is UTYPE the return value is the contents of the
;    field UTYPE - the returned string is in LOWER case.
;
;   2.result=VARTYPE(var, /real)
;
;    The return value is TRUE if {var} is a number of any type,
;    except complex.
;
;   3.result=VARTYPE(var, /integer)
;
;    The return value is TRUE if {var} is an integer of any type,
;    eg. BYTE, INTEGER, LONG
;
;--------------------------------------------------------------------------

function vartype,               $;
    arg,                        $;
    integer=integer,            $;
    notinteger=notinteger,      $;
    real=real,                  $;
    notreal=notreal,            $;
    string=strkey,              $;
    notstring=notstrkey,        $;
    structure=structure,        $;
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

  vtype=['NotDefined', 'BYTE', 'INTEGER*2', 'INTEGER*4', 'REAL*4',     $
    'REAL*8', 'COMPLEX*4', 'STRING', 'STRUCTURE', 'COMPLEX*8',    $
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
;   Number  Type
;   0       Undefined
;   1       Byte
;   2       Int*2
;   3       Int*4
;   4       Real*4
;   5       Real*8
    6       complex
;   7       String
;   8       Structure
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
                  noidam=noidam, $
                  _extra=_extra

  common loc_getdata, special
  common loc_itemsbuffer, buffer

  if undefined(special) then special=0

  errmsg=''

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
  
  ;---------------------------------------------------------------------------------------------------
  ; For geometry database access :
  ; If signal starts with "GEOM::" or "SIGGEOM::" then use appropriate
  ; function
  ind_match = strpos(workname, 'SIGGEOM::')
  if ind_match eq 0 then begin
     workname = strlowcase(strmid(workname, strlen('SIGGEOM::')))
     struct = getgeomsignaldata(workname, worksource, _extra=_extra)

     return, struct
  endif
  
  ind_match = strpos(workname, 'GEOM::')
  if ind_match eq 0 then begin
     workname = strlowcase(strmid(workname, strlen('GEOM::')))
     struct = getgeomdata(workname, worksource, _extra=_extra)

     return, struct
  endif

  ;---------------------------------------------------------------------------------------------------

  source = worksource
  header = callidam2(workname, worksource, get_bad=allowbad, /get_timedble)

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
      header = callidam2(workname, worksource, /get_bad, /get_timedble)
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
     
     if(dblk.erc ne 0) then return, dblk    ; Error
     if(rank eq 0) then return, dblk        ; Group level structures have no shape nor coordinates!
     
     data_units = ''
     data_label = ''
     rc=getdataunits(header.handle, data_units)
     rc=getdatalabel(header.handle, data_label)

  endif else begin        
     dblk=getidamdata(header)
     
     if not_structure(dblk) then begin
        errmsg='Unable to read data array: Check the passed arguments!'
        goto, errorcatch
     endif
          
     data_units = dblk.data_units
     data_label = dblk.data_label

  endelse
  
  if not_structure(dblk) then begin
     errmsg='Unable to read data array'
     goto, errorcatch
  endif
  
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
       s[0]=size(data, /dim)        ;; OK if data is an array but not a top level structure array
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
            dunits:data_units,        $
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

  ;;if stringdata then rank = rank - 1        ; Reduce Rank for string data

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

  errmsg='No IDAM!'        ; When the get_ida function is not available!
  goto, errorcatch

  return, {erc:-1, errmsg:'No IDAM!'}      

end


