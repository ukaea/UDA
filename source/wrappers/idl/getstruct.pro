
;===================================================================================================================== 
; DGMuir 26Feb2010: Modifications required to access Hierarchical Data Structures
;===================================================================================================================== 
; Interrogate IDAM for details of Hierarchical Data Structures, then build them recursively
;
; Change History:
; 
; 10Feb2010    DGMuir    Original Version
;---------------------------------------------------------------------------------------------------------------------

;;; @/usr/local/fusion/idlcodes/dgm/makeidamstructure.pro

; Interrogate IDAM for details of Hierarchical Data Structures and build then recursively
;
; Check if the atomic data is a pointer. If so then use pointer within the structure - variable length arrays?
; Check tag names are legal: don't begin with a numeral; no spaces; no reserved words. If found then prefix with 'a' or substitute '_'
;
; Change History:
; 
; 10Feb2010    DGMuir    Original Version
; 16May2011    dgm    If an atomic data array has zero count then add a null pointer to the structure 
; 03Nov2011    dgm    Added additional checks to checknamesarelegal
;            Fixed bug where a modified tag name was used to request data from the IDAM accessors
; 16May2012    dgm    Fixed bug where a User Defined Types is UNLIMITED and has 0 number of array elements
; 15Aug2014    dgm    Added keyword /children to findidamtreestructure to manage same named items 
;---------------------------------------------------------------------------------------------------------------------
;---------------------------------------------------------------------------------------------------------------------

function checkheapmemory 
   r = MEMORY(/structure)
   print, r  
   return, 0
end   

function checknamesarelegal, names, verbose=verbose

; change to using:   IDL_VALIDNAME(name [, /CONVERT_ALL] [, /CONVERT_SPACES])

   found  = 0
   prefix = '_' ;'a_'
   prefix2= '_' ;'a'
   sub    = '_'
   reserved = ['AND','BEGIN','BREAK','CASE','COMMON','COMPILE_OPT','CONTINUE','DO','ELSE','END','ENDCASE','ENDELSE', $
               'ENDFOR','ENDIF','ENDREP', 'ENDSWITCH','ENDWHILE','EQ','FOR','FORWARD_FUNCTION','FUNCTION ','GE','GOTO', $
           'GT','IF','INHERITS','LE','LT','MOD','NE','NOT','OF', 'ON_IOERROR','OR','PRO','REPEAT','SWITCH','THEN', $
           'UNTIL','WHILE','XOR']
   count = n_elements(names)
   if(count eq 0) then return, names
   for i = 0L, count-1 do begin
      w = WHERE(reserved eq strupcase(names[i]), nw)    ; IDL reserved words
      if(nw ne 0) then begin
         found    = 1
     if keyword_set(verbose) then print,'WARNING: An Illegal structure name tag has been detected ['+names[i]+'] - Prefixing with '+prefix
     names[i] = prefix+names[i]            ; No guarantee the name is unique!
      endif else begin
         
     badchars  = BYTE(' \|,<.>/?;:@#~[{]}`¬!"£$%^&*()-=+')
     nbadchars = n_elements(badchars)
     for j=0,nbadchars-1 do begin 
        c = badchars[j]
        r = strpos(names[i], string(c))            ; Search for illegal characters within the tag name
        if(r ge 0) then begin
           if keyword_set(verbose) then print,'WARNING: An Illegal structure name tag has been detected ['+names[i]+'] - Replacing '+string(c)+' with '+sub
           b = BYTE(names[i])
           w = WHERE(b eq c, nw)    
           if(nw gt 0) then begin
              b[w] = sub
              names[i] = STRING(b)
           endif
           found = 1
        endif
     endfor
     
     b = BYTE(strmid(names[i],0,1))            ; Numeric First character
     if(b ge BYTE('0') and b le BYTE('9')) then begin
        if keyword_set(verbose) then print,'WARNING: An Illegal structure name tag has been detected ['+names[i]+'] - Prefixing with '+prefix
        names[i] = prefix+names[i]        
            found = 1
         endif
      endelse 
   endfor
   
   if(found) then begin                    ; Check for dup names (single pass only)
      s = SORT(names)
      for i = 1L, count-1 do begin
         if(names[s[i]] eq names[s[i-1]]) then begin
        print,'ERROR: Duplicate structure name tags have been detected ['+names[i]+'] - Prefixing with '+prefix2
        if(strmid(names[i],0,2) eq prefix) then begin
           names[i] = prefix2+names[i]
        endif else begin
           names[i] = prefix+names[i]
        endelse
     endif
      endfor      
   endif

   return, names
end

function makeidamstructurearrayatomic, handle, tree, count, rank, shape, debug=debug, verbose=verbose

; handle: IDAM data handle
; tree  : Data Tree node with Atomic Data only
; count : the size of the Structure array
; rank  : the rank of the Structure array
; shape : the shape of the Structure array

; returns integer if error (0: system, -1: No Data)
;      structure if OK

   forward_function checknamesarelegal
   
   if NOT keyword_set(debug) then debug = 0 else debug = 1
   if NOT keyword_set(verbose) then verbose = 0 else verbose = 1
   
   if(debug) then print,'function makeidamstructurearrayatomic    tree = '+strtrim(tree,2)+'    count = '+strtrim(count,2)
    
;---------------------------------------------------------------------------------------------------------------------
; Structure has Only Atomic members
   
   acount = getidamnodeatomiccount(handle, tree, debug=debug, verbose=verbose)        ; Count of the Tree Node Structure atomic type components

   if(acount eq 0) then begin
      if (verbose) then print,'IDL makeidamstructure ERROR: the structure has No atomic members when expected!'
      return, 0        ; System Error 
   endif      
      
   anamelist = getidamnodeatomicnames(handle, tree, debug=debug, verbose=verbose)        ; Names of the Atomic Components 
   apointer  = getidamnodeatomicpointers(handle, tree, debug=debug, verbose=verbose)        ; Is this Atomic Component a Pointer ? 
 
   anamelist2 = anamelist
   ; Ensure names are OK: Legal IDL variable names! 
   anamelist2 = idl_validname(anamelist2, /CONVERT_ALL)

   xcount = getidamnodeatomicdatacount(handle, tree, anamelist[0], debug=debug, verbose=verbose)    ; Array of non-zero count?   
   if(xcount gt 0) then begin
      data = getidamnodeatomicdata(handle, tree, anamelist[0], debug=debug, verbose=verbose)       
      if(apointer[0] and is_number(data)) then begin
         p=ptr_new(data)
         astr = CREATE_STRUCT(anamelist2[0], p)         
      endif else begin
         astr = CREATE_STRUCT(anamelist2[0], data)         
      endelse
   endif else begin
     p=ptr_new()                        ; Return a NULL pointer
     astr = CREATE_STRUCT(anamelist2[0], p)               
   endelse   

   for i=1L, acount-1 do begin
      xcount = getidamnodeatomicdatacount(handle, tree, anamelist[i], debug=debug, verbose=verbose)    ; Array of non-zero count?   
      if(xcount gt 0) then begin
         data = getidamnodeatomicdata(handle, tree, anamelist[i], debug=debug, verbose=verbose)
         if(apointer[i] and is_number(data)) then begin
            p=ptr_new(data)
        astr = CREATE_STRUCT(astr, anamelist2[i], p)         
         endif else begin
        astr = CREATE_STRUCT(astr, anamelist2[i], data)   
         endelse
      endif else begin
           p=ptr_new()                        ; Return a NULL pointer
     astr = CREATE_STRUCT(astr, anamelist2[i], p)         
      endelse
   endfor

   parent  = getidamnodeparent(handle, tree, debug=debug, verbose=verbose)
   childid = getidamnodechildid(handle, parent, tree, debug=debug, verbose=verbose)        ; Base Branch ID

   if(rank le 1) then begin 

      str = REPLICATE(astr, count)                ; Create the Array of data structures by Replication 

      for j=1L, count-1 do begin
         node = getidamnodechild(handle, parent, childid+j, debug=debug, verbose=verbose)     
         for i=0L, acount-1 do begin
            xcount = getidamnodeatomicdatacount(handle, node, anamelist[i], debug=debug, verbose=verbose)    ; Array of non-zero count?   
            if(xcount gt 0) then begin
               data = getidamnodeatomicdata(handle, node, anamelist[i], debug=debug, verbose=verbose)
               if(apointer[i] and is_number(data)) then begin
                  p=ptr_new(data)
                  str[j].(i) = p
               endif else begin
                  str[j].(i) = data          
               endelse
            endif else begin
               p=ptr_new()                        ; Return a NULL pointer
               str[j].(i) = p
            endelse
         endfor     
      endfor
      
   endif else begin
   
      if(rank gt 2) then begin
     print, 'ERROR: Not configured for structured arrays of rank > 2'
     return, -1
      endif
      
      str = REPLICATE(astr, shape[0], shape[1])
      
      offset = 1L 
      for j=0L, shape[1]-1L do begin
         for k=1L, shape[0]-1L do begin
            node = getidamnodechild(handle, parent, childid+offset, debug=debug, verbose=verbose)
            offset = offset+1
            for i=0L, acount-1 do begin
               xcount = getidamnodeatomicdatacount(handle, node, anamelist[i], debug=debug, verbose=verbose)    ; Array of non-zero count?
               if(xcount gt 0) then begin
                  data = getidamnodeatomicdata(handle, node, anamelist[i], debug=debug, verbose=verbose)
                  if(apointer[i] and is_number(data)) then begin
                     p=ptr_new(data)
                     str[k,j].(i) = p
                  endif else begin
                     str[k,j].(i) = data
                  endelse
               endif else begin
                 p=ptr_new()                        ; Return a NULL pointer
                 str[k,j].(i) = p
               endelse
            endfor
         endfor
      endfor         
   endelse

   return, str    
end

function makeidamstructureitem, handle, tree, debug=debug, verbose=verbose

   forward_function makeidamstructure
   forward_function checknamesarelegal

; handle: IDAM data handle
; tree  : Starting Data Tree node from which to make the data structure

; returns integer if error  (0: system, -1: No Data)
;      structure if OK

   if NOT keyword_set(debug) then debug = 0 else debug = 1
   if NOT keyword_set(verbose) then verbose = 0 else verbose = 1
   
   if(debug) then print,'function makeidamstructureitem    tree = '+strtrim(tree,2)
    
;---------------------------------------------------------------------------------------------------------------------
; List all Atomic Type Components within this Data Tree Node        
   
   acount    = getidamnodeatomiccount(handle, tree, debug=debug, verbose=verbose)       ; Count of the Tree Node Structure atomic type components
   anamelist = getidamnodeatomicnames(handle, tree, debug=debug, verbose=verbose)       ; Names of the Atomic Components
   apointer  = getidamnodeatomicpointers(handle, tree, debug=debug, verbose=verbose)    ; Is this Atomic Component a Pointer ?
  
   if(acount gt 0) then begin
      anamelist2 = anamelist
      ; Ensure names are OK: Legal! 
      anamelist2 = idl_validname(anamelist2, /CONVERT_ALL)
      xcount = getidamnodeatomicdatacount(handle, tree, anamelist[0], debug=debug, verbose=verbose);
      if(xcount gt 0) then begin
         data = getidamnodeatomicdata(handle, tree, anamelist[0], debug=debug, verbose=verbose) 
         if(apointer[0] and is_number(data)) then begin
            p=ptr_new(data)
            astr = CREATE_STRUCT(anamelist2[0], p)
         endif else begin
            astr = CREATE_STRUCT(anamelist2[0], data)         
         endelse
      endif else begin
         p=ptr_new()                        ; Return a NULL pointer
         astr = CREATE_STRUCT(anamelist2[0], p)
      endelse     
      for j=1, acount-1 do begin
         xcount = getidamnodeatomicdatacount(handle, tree, anamelist[j], debug=debug, verbose=verbose);
         if(xcount gt 0) then begin
            data = getidamnodeatomicdata(handle, tree, anamelist[j], debug=debug, verbose=verbose)
            if(apointer[j] and is_number(data)) then begin
               p=ptr_new(data)
               astr = CREATE_STRUCT(astr, anamelist2[j], p)
            endif else begin
               astr = CREATE_STRUCT(astr, anamelist2[j], data)
            endelse
         endif else begin
            p=ptr_new()                        ; Return a NULL pointer
            astr = CREATE_STRUCT(astr, anamelist2[j], p)
         endelse
      endfor
   endif   

   ;rc = checkheapmemory()
   
;---------------------------------------------------------------------------------------------------------------------
; Structured Components within this Node: Build the structure hierarchy recursively
; Tree nodes with NULL data generally mean there is no data but a structure definition exists. 
; An example is a netcdf4 user defined type with an UNLIMITED coordinate dimension that is currently 0 in value.         

   scount    = getidamnodestructurecount(handle, tree, debug=debug, verbose=verbose)    ; Count of the Tree Node Structure structure type components
   snamelist = getidamnodestructurenames(handle, tree, debug=debug, verbose=verbose)    ; Names of the Structured components
    
   if(scount gt 0) then begin
      snamelist2 = snamelist
      ; Ensure names are OK: Legal! 
      snamelist2 = idl_validname(snamelist2, /CONVERT_ALL)

;childcount = getidamnodechildrencount(handle, tree)
;if(childcount eq 0 and scount eq 1) then begin
;   sstr = CREATE_STRUCT(snamelist2[0], 0)             
;endif else begin

      node = findidamtreestructure(handle, tree, snamelist[0], debug=debug, verbose=verbose, /children)
      sstr0 = makeidamstructure(handle, node, debug=debug, verbose=verbose)
;TODO use structure shape!
      if(NOT is_structure(sstr0)) then begin
         ;return, sstr0   
         sstr = CREATE_STRUCT(snamelist2[0], 0)
      endif else begin
         sstr = CREATE_STRUCT(snamelist2[0], sstr0)   
      endelse
       
      for j=1, scount-1 do begin   
         node = findidamtreestructure(handle, tree, snamelist[j], debug=debug, verbose=verbose)
         if(node ne 0L) then begin
            sstr0 = makeidamstructure(handle, node, debug=debug, verbose=verbose)
            if(NOT is_structure(sstr0)) then return, sstr0   
            sstr = CREATE_STRUCT(sstr, snamelist2[j], sstr0)
         endif else sstr = CREATE_STRUCT(sstr, snamelist2[j], 0);
      endfor
   endif

;---------------------------------------------------------------------------------------------------------------------
; Return Concatenated Structures

   if(scount eq 0 and acount eq 0) then begin
      if (verbose) then print,'IDL makeidamstructureitem ERROR: the passed data tree node has neither Atomic nor Structural elements!'
      return, 0        ; System Error 
   endif   
  
   if(scount eq 0) then return, astr        ; If no structured component, then return the atomic elements 
   if(acount eq 0) then return, sstr        ; If no atomic component, then return the structured elements 

   return, CREATE_STRUCT(astr, sstr)        ; Combine both Atomic and Structured components    
end
   

function makeidamstructure, handle, tree, debug=debug, verbose=verbose

  forward_function makeidamstructureitem

; handle: IDAM data handle
; tree  : Starting Data Tree node from which to begin making the data structure

; returns integer if error  (0: system, -1: No Data)
;      structure if OK

   if NOT keyword_set(debug) then debug = 0 else debug = 1
   if NOT keyword_set(verbose) then verbose = 0 else verbose = 1
   
   if(debug) then print,'function makeidamstructure    tree = '+strtrim(tree,2)

; Create Array of the structures  
  
   if(tree eq 0) then begin
      if (verbose) then print,'IDL makeidamstructure ERROR: the passed data tree node is Null: No Data returned!'
      return, -1    ; No Data! 
   endif
   
   parent = getidamnodeparent(handle, tree, debug=debug, verbose=verbose)
   
   if(parent le 0) then begin
      if(parent lt 0) then begin
         if (verbose) then print,'IDL makeidamstructure ERROR: there is No Root Data tree node consistent with the IDAM handle and child node provided'
         return, 0    ; System error
      endif
      
      if(debug) then begin
         print,'IDL makeidamstructure WARNING: the passed data tree node is the Root Node'
         print,'Adopting the First Child node as the starting node for the structure build'
      endif
      
      parent = tree                                        ; Swap
      ntree  = getidamnodechild(handle, parent, 0, debug=debug, verbose=verbose)        ; Find the first child node

   endif else ntree = tree
   
   childid = getidamnodechildid(handle, parent, tree, debug=debug, verbose=verbose)        ; Base Branch ID
      
   if(childid lt 0) then begin
      if (verbose) then print,'IDL makeidamstructure ERROR: the Child Node First Branch ID was not found!'
      return, 0        ; System Error
   endif                            ; Node is a child
 
   count = getidamnodestructuredatacount(handle, ntree, debug=debug, verbose=verbose)    ; Count of Tree Node Structure Array elements       
   rank  = getidamnodestructuredatarank(handle, ntree,  debug=debug, verbose=verbose)    ; Structure Array rank       
   shape = getidamnodestructuredatashape(handle, ntree, debug=debug, verbose=verbose)    ; Structure Array shape       

;print, count
;print, rank
;print, shape

; If the structure contains child structures, then build recursively
; otherwise, create an array of structures and assign each set of atomic values separately and non-recursively.
; This ensures efficiency when the data structures are simple.

   scount = getidamnodestructurecount(handle, ntree, debug=debug, verbose=verbose)
   
   if(scount eq 0 and count gt 0) then begin        ; Create a single structure element and replicate     
;TODO use structure shape!
      return, makeidamstructurearrayatomic(handle, ntree, count, rank, shape, debug=debug, verbose=verbose)
   endif

; Child data structures   
   
   if(count gt 0) then begin               
      str = makeidamstructureitem(handle, ntree, debug=debug, verbose=verbose)

      if(NOT is_structure(str)) then begin
         return, str            ; Pass back error flag (non structure)
      endif     

      if(rank le 1) then begin      
         for j=1L, count-1L do begin
            node = getidamnodechild(handle, parent, childid+j, debug=debug, verbose=verbose)
        nstr = makeidamstructureitem(handle, node, debug=debug, verbose=verbose)        ; Build Structure Hierarchy     
        if(is_structure(nstr)) then  str = [str, nstr]        ; Build Array of Structures (Must be the same size, type etc.)
         endfor
      endif else begin      
         if(rank gt 2) then begin
        print, 'ERROR: Not configured for structured arrays of rank > 2'
        return, -1
     endif
     str = REPLICATE(str, shape[0], shape[1])
     offset = 1L 
         for j=0L, shape[1]-1L do begin
        for k=1L, shape[0]-1L do begin
               node = getidamnodechild(handle, parent, childid+offset, debug=debug, verbose=verbose)
           offset = offset+1         
           nstr = makeidamstructureitem(handle, node, debug=debug, verbose=verbose)        ; Build Structure Hierarchy     
           if(NOT is_structure(nstr)) then begin
              print, 'ERROR: Structure Not returned when expected!'
              return, -1
           endif
               str[k,j] = nstr        ; Build Array of Structures (Must be the same size, type etc.)        
        endfor
     endfor   
      endelse      
            
   endif else str = 0

;TODO use structure shape!

   return, str
end  

;---------------------------------------------------------------------------------
; Function: GETSTRUCT

; To return a structure from a previous call to IDAM pass in the prior handle value
;
; returned: {erc,     // Error Code
;            errmsg}    // Error Message
;         signal,    // Data object name 
;         source,    // Data Source 
;            handle,    // IDAM data handle         
;            data}    // Data   

;---------------------------------------------------------------------------------

function getstruct, signal, source, debug=debug, verbose=verbose, priorhandle=priorhandle, usepriorhandle=usepriorhandle                    

  if (NOT keyword_set(usepriorhandle)) then $
     handle = idamgetapi(signal, strtrim(source,2), debug=debug, verbose=verbose) $
  else $
     handle = priorhandle      
  
  if(handle < 0) then begin
     if (keyword_set(verbose)) then print, 'Error accessing data'
     errmsg=''
     rc = geterrormsg(handle, errmsg)
     if (keyword_set(verbose)) then print, errmsg          
     return, {erc:handle, errmsg:errmsg, signal:signal, source:source, handle:handle} 
  endif
  
  errcode = geterrorcode(handle)
  if(errcode ne 0) then begin
     errmsg=''
     rc = geterrormsg(handle, errmsg)
     if (keyword_set(verbose)) then print, errmsg
     rc=freeidam(handle)
     return, {erc:errcode, errmsg:errmsg, signal:signal, source:source, handle:handle}
  endif
     
;====================================================================================================
; Is the Data a Hierarchical Data Type: Test and Register

  udregister = setidamdatatree(handle)
  
  if(udregister) then begin
  
     node = getidamnodechild(handle, 0, 0)        ; Data is located in the first child node

; *** NOTE: Regularisation may not be necessary if the data array within VLEN structure is a pointer type!
; *** This needs testing and implementing before rollout
     
     rc = regulariseidamvlenstructures(handle, node, debug=debug, verbose=verbose)    ; Regularise the Shape of all VLEN Structured components   

     if(rc ne 0) then begin
        if (keyword_set(verbose)) then print, 'IDAM error [Unable to Regularise VLEN data structures]'
    return, {erc:rc,errmsg:'Unable to Regularise VLEN data structures',signal:signal,source:source,handle:handle}
     endif

     str = makeidamstructure(handle, node, debug=debug, verbose=verbose)        ; Support IDL function - create structure from tree
     
     if(NOT is_structure(str)) then begin
        if(str eq 0) then return, {erc:-1, errmsg:'System Error (use verbose keyword to clarify)!', $
                               signal:signal,source:source,handle:handle}
        return, {erc:-1, errmsg:'No Data Returned!', signal:signal,source:source,handle:handle}
     endif 
     return, {erc:0, errmsg:'', signal:signal,source:source,handle:handle, data:str}

  endif else begin        
     if (keyword_set(verbose)) then print, 'Not a Structured Item: Use getdata!'
     return, {erc:-1, errmsg:'Not a Structured Item: Use getdata!', signal:signal,source:source,handle:handle} 
  endelse
  
end

