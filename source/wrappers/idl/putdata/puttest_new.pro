pro puttest_new, test, directory=directory

; Testing IDAM PUT Tools

   ;; DLM_REGISTER,'/home/dgm/putdata_new.dlm'		; Use the Published version
   ;; DLM_REGISTER,'./putdata_new.dlm'		; Use the Published version
   
   ;; ensure the correct module is loaded

   debug   = 0
   verbose = 0
   
   if undefined(directory) then directory = './'

   if(test eq 1) then begin
   
      $rm testnew1a.nc 

      print, '   '
      print, '------------ TEST 1A --------------'
      print, 'Open file in directory given by directory keyword. Set various top-level attributes.'
      print, '------------------------------------'
      
      pd = obj_new('putdata', filename='testnew1a.nc', directory=directory,                 $
                   title='Test #1A', comment='Test comment for 1A', class='Analysed',    $
                   shot=123456L, pass=0, status=1L, code='puttest_new.pro', version=99L, $
                   xml='<xml>Some XML could go here</xml>', debug=debug, verbose=verbose)

      if ~obj_valid(pd) then print, 'Test #1A failed' $
      else print, 'Test #1A passed'

      ; Update status
      rtn = pd->add_attribute(data=0L, name='status')

      rtn = pd->close()      

      if rtn ne 0 then print, 'Test #1A, failed to close file' $
      else print, 'Test #1A, file closed'

      $rm testnew1b.nc 

      print, '   '
      print, '------------ TEST 1B --------------'
      print, 'Open file with directory part of the filename. Set various top-level attributes'
      print, '------------------------------------'
      
      filename = directory+'/testnew1b.nc'

      rtn = pd->open(filename=filename,    $
               title='Test #1B', comment='Test comment for 1B', class='Analysed',    $
               shot=123456L, pass=0, status=1L, code='puttest_new.pro', version=99L, $
               xml='<xml>Some XML could go here</xml>', debug=debug, verbose=verbose)
      
      if rtn ne 0 then print, 'Test #1B failed' $
      else print, 'Test #1B passed'
      
      rtn = pd->close()      

      if rtn ne 0 then print, 'Test #1B, failed to close file' $
      else print, 'Test #1B, file closed'

      obj_destroy, pd
      
      return         
   endif

;-----------------------------------------------------------------------------------------
; #2  Update the file test1a.nc (Cannot extend the space allocated to previous data entities)

   if(test eq 2) then begin      

     print, '   '
     print, '------------ TEST 2A --------------'
     print, 'Update test1a.nc. Changing class of data, title etc. Not changing anything in the file.'
     print, '------------------------------------'

      pd = obj_new('putdata', filename='testnew1a.nc', directory=directory, $
                   class='raw', title='Test #2A', $
                   shot=654321L, status=2L, $
                   comment='Comment for test #2A', code='puttest', version=88L,$
                   xml='<xml>Modified XML</xml>', $
                   /update, $
                   debug=debug, verbose=verbose)

      if ~obj_valid(pd) then print, 'Test #2A failed' $
      else print, 'Test #2A passed'

      rtn = pd->close()      

      if rtn ne 0 then print, 'Test #2A, failed to close file' $
      else print, 'Test #2A, file closed'

      
      print, '   '
      print, '------------ TEST 2B --------------'
      print, 'Update test1b.nc. Changing class of data, title etc. Not changing anything in the file.'
      print, '------------------------------------'
      

      filename = directory+'testnew1b.nc'
      rtn = pd->open(filename=filename,$
                     class='raw', title='Test #2B', shot=654321L, $
                     status=2L, $
		    comment='Comment for test #2B', code='puttest', version=88L,$
                     xml='<xml>Modified XML</xml>', /update, $
                     debug=debug, verbose=verbose)

      if ~obj_valid(pd) then print, 'Test #2B failed' $
      else print, 'Test #2B passed'

      rtn = pd->close()      

      if rtn ne 0 then print, 'Test #2B, failed to close file' $
      else print, 'Test #2B, file closed'

      obj_destroy, pd
      
      return   
   endif    

;-----------------------------------------------------------------------------------------
; #3  Add Devices documentation
   
   if(test eq 3) then begin

     $rm testnew3.nc

     print, '   '
     print, '------------ TEST 3 --------------'
     print, 'Open new file (testnew3.nc). Create devices and associated attributes.'
     print, '------------------------------------'
      
      pd = obj_new('putdata', filename='testnew3.nc', directory=directory, $
                   title='Test #3', class='raw', shot=123456L,             $
                   debug=debug, verbose=verbose)
    
      rtn = pd->add_device(name='dataq1', serial="abc123", type="def456", id='identity #1', $
                           resolution=16, range=[100.0,200], channels=32)
      rtn = pd->add_device(name='dataq2', serial="abc234", type="def567", id='identity #2', $
                           resolution=16L, range=[100.0D,200D], channels=32L)
		    
      rtn = pd->add_attribute(data='new stuff on dataq1', name='/devices/dataq1/notes')
      rtn = pd->add_attribute(data='new stuff on dataq2', name='/devices/dataq2/notes')
		    
; Close the File

      if rtn ne 0 then print, 'Test #3B, failed to close file' $
      else print, 'Test #3B, file closed'
 
      rtn = pd->close()
      
      obj_destroy, pd

      return     

   endif  

;-----------------------------------------------------------------------------------------
; #4  Create a minimal file and Add Group Level Attributes

   if(test eq 4) then begin

$rm test4.nc
   
     print, '   '
     print, '------------ TEST 4 --------------'
     print, 'Create new file & add attribtues of all different scalar types & complex types. Use different group levels and overwrite some of the attributes.'
     print, '------------------------------------'

     pd = obj_new('putdata', filename='testnew4.nc', directory=directory, $ 
                  title='Test #4', class='raw', shot=123456L, $
                  debug=debug, verbose=verbose) 

; string values
  
      rtn = pd->add_attribute(data='Hello World',  name='/a/stringscalar')

; scalar values
  
      rtn = pd->add_attribute(data=3.1415927,  name='/a/floatscalar')
      rtn = pd->add_attribute(data=3.1415927D, name='/a/doublescalar')
      rtn = pd->add_attribute(data=byte(1),    name='/a/bytescalar')
      rtn = pd->add_attribute(data=2,          name='/a/shortscalar') 
      rtn = pd->add_attribute(data=long(3),    name='/a/intscalar')
      rtn = pd->add_attribute(data=long64(4),  name='/a/long64scalar')
      rtn = pd->add_attribute(data=uint(5),    name='/a/ushortscalar')
      rtn = pd->add_attribute(data=ulong(6),   name='/a/uintscalar')
      rtn = pd->add_attribute(data=ulong64(7), name='/a/ulong64scalar')

; Write and Overwrite a float scalar
      rtn = pd->add_attribute(data=3.1415927, name='/b/floatscalar')
      rtn = pd->add_attribute(data=2.718281828, name='/b/floatscalar')

; Test Complex (User Defined Type) Attributes

      test = COMPLEX(6.0,7.0)
      rtn = pd->add_attribute(data=test, name='complex1')
      rtn = pd->add_attribute(data=test, name='/a/complex1')
      rtn = pd->add_attribute(data=test, name='/a/b/complex1')
      
      test = DCOMPLEX(8.0,9.0)
      rtn = pd->add_attribute(data=test, name='dcomplex1')
      rtn = pd->add_attribute(data=test, name='/a/dcomplex1')
      rtn = pd->add_attribute(data=test, name='/a/b/dcomplex1')
      
; Close the File 

      rtn = pd->close()
      
      obj_destroy, pd

      return     

   endif


   ;-----------------------------------------------------------------------------------------
; #5  Add Group Level Attribute Arrays (not string arrays - yet - not implemented!)

   if(test eq 5) then begin

     print, '   '
     print, '------------ TEST 5 --------------'
     print, 'Update test4.nc. Add array attributes of all types.'
     print, '------------------------------------'

      
      pd = obj_new('putdata', filename='testnew4.nc', directory=directory, $
                   title='Test #5', class='raw', shot=123456L, /update,    $  
                   debug=debug, verbose=verbose)
   
      rtn = pd->add_attribute(data=[1.0,2.0,3.0,4.0,5.0],       name='/a/b/floatarray')
      rtn = pd->add_attribute(data=[1.0D,2.0D,3.0D,4.0D,5.0D],  name='/a/b/doublearray')
      rtn = pd->add_attribute(data=byte([10,20,30,40,50]),      name='/a/b/bytearray')
      rtn = pd->add_attribute(data=[1,2,3,4,5],                 name='/a/b/shortarray')
      rtn = pd->add_attribute(data=[1L,2L,3L,4L,5L],            name='/a/b/intarray')
      rtn = pd->add_attribute(data=long64([1,2,3,4,5]),         name='/a/b/long64array')
      rtn = pd->add_attribute(data=uint([1,2,3,4,5]),           name='/a/b/ushortarray')
      rtn = pd->add_attribute(data=ulong([1,2,3,4,5]),          name='/a/b/uintarray')
      rtn = pd->add_attribute(data=ulong64([1,2,3,4,5]),        name='/a/b/ulong64array')

      test = [COMPLEX(16.0,17.0),COMPLEX(18.0,19.0)]
      rtn = pd->add_attribute(data=test,   name='/a/b/complexarray')

      test = [DCOMPLEX(16.5,17.5),DCOMPLEX(18.5,19.5)]
      rtn = pd->add_attribute(data=test,   name='/a/b/dcomplexarray')

; Close the File 
 
      rtn = pd->close()
      
      obj_destroy, pd
      
      return     

   endif


   ;-----------------------------------------------------------------------------------------
   ; #6 Add Coordinates: Various Scalar Types

   if(test eq 6) then begin   
      print, '------------ TEST 6 --------------'
      print, 'Create new file. Add scalar dimensions & coordinates'
      print, '------------------------------------'   
   
      pd = obj_new('putdata', filename='testnew6.nc', directory=directory, $
                   title='Test #5', class='raw', shot=123456L, debug=debug, verbose=verbose)

      rtn = pd->add_dim(values=3.14159, name='/a/unit', label='unitary pi value', $
                        units='counts')		       

      rtn = pd->add_dim(values=2.71828D, name='/a/b/single', units='counts', $
                        label='unitary e value')
  
      rtn = pd->add_dim(values=byte(10),    name='/a/b/c/byte')      
      rtn = pd->add_dim(values=20,          name='/a/b/c/short')  
      rtn = pd->add_dim(values=30L,         name='/a/b/c/int')   
      rtn = pd->add_dim(values=long64(40),  name='/a/b/c/long')      
      rtn = pd->add_dim(values=uint(50),    name='/a/b/c/ushort')   
      rtn = pd->add_dim(values=ulong(60),   name='/a/b/c/uint')      
      rtn = pd->add_dim(values=ulong64(70), name='/a/b/c/ulong')   
      rtn = pd->add_dim(values=complex(80.0,90.0),  name='/a/b/c/complex')       
      rtn = pd->add_dim(values=dcomplex(80.0,90.0), name='/a/b/c/dcomplex')
   

      rtn = pd->close()
      
      obj_destroy, pd
      return
   endif

;-----------------------------------------------------------------------------------------
; #7 Add Coordinates: Various array types
   
   if(test eq 7) then begin

$rm testnew7.nc

     print, '   '
     print, '------------ TEST 7 --------------'
     print, 'Open new file (testnew7.nc). Add array dimensions and coordinates of all types. No unlimited dimensions.'
     print, '------------------------------------'

      fileId=0L
      
       pd = obj_new('putdata', filename='testnew7.nc', directory=directory, $
                    title='Test #7', class='raw', shot=123456L, $
                    debug=debug, verbose=verbose)
		    
      rtn = pd->add_dim(values=findgen(3),       name='/a/float')
      rtn = pd->add_dim(values=dindgen(3),       name='/a/double')
      rtn = pd->add_dim(values=indgen(3,/byte),  name='/a/byte')
      rtn = pd->add_dim(values=indgen(3),        name='/a/short')
      rtn = pd->add_dim(values=indgen(3,/long),  name='/a/int')
      rtn = pd->add_dim(values=indgen(3,/l64),   name='/a/long')
      rtn = pd->add_dim(values=indgen(3,/uint),  name='/a/ushort')
      rtn = pd->add_dim(values=indgen(3,/ulong), name='/a/uint')
      rtn = pd->add_dim(values=indgen(3,/ul64),  name='/a/ulong')
      rtn = pd->add_dim(values=cindgen(3),       name='/a/complex')
      rtn = pd->add_dim(values=dcindgen(3),      name='/a/dcomplex')
		                  
; Close the File 
 
      rtn = pd->close()

      obj_destroy, pd 
      return     

   endif 

;-----------------------------------------------------------------------------------------
; #8 Coordinates: Domain Data

   if(test eq 8) then begin

$rm testnew8.nc

     print, '   '
     print, '------------ TEST 8 --------------'
     print, 'Create new file. Write dimensions and coordinates using start, increment, count'
     print, '------------------------------------'

      pd = obj_new('putdata', filename='testnew8.nc', title='Test #8', class='raw', $
                   shot=654321L, directory=directory, debug=debug, verbose=verbose)

      start     = 0.0D
      increment = 0.5D
      count     = ulong(3)
        
      rtn = pd->add_dim(starts=start, increments=increment, counts=count, name='/a/m')
      rtn = pd->add_dim(values=[-1.0,-2.0, -3.0], starts=start, increments=increment, counts=count, name='/a/o')
 
      start     = [0.0D, 10.0D, 20.0D]
      increment = [0.5D, 0.1D, 0.2D]
      count     = [ulong(3), ulong(3), ulong(3)]
        
      rtn = pd->add_dim(starts=start, increments=increment, counts=count, name='/a/p')

; Close the File 
 
      rtn = pd->close()

      obj_destroy, pd 

      
      return     

   endif         

             
   if (test eq 9) then begin

$rm testnew9.nc

   print, '   '
   print, '------------ TEST 9 --------------'
   print, 'Create new file. Write variables with scalar data of all types and same dimension. Also attach device to the file.'
   print, '------------------------------------'

   pd = obj_new('putdata', filename='testnew9.nc', directory=directory, title='Test #9', class='raw', $
                shot=123456L, debug=debug, verbose=verbose)
		    
   rtn = pd->add_dim(values=3.1415927, name='/a/x')
   
   rtn = pd->add_device(name='abc123', serial="abc321", type="def456", id='identity #1', $
                        resolution=16, range=[100.0,200], channels=32)

   rtn = pd->add_signal(1.0E3, name='/a/float', dims='x', $
                        scale=10.0, offset=-1.0, $
                        units = 'counts.m/s^2', label = 'test label', title = 'test title', $
                        comment='test comment', device='abc123', channel=1)
      
   rtn = pd->add_signal(2.0D3,      name='/a/double', dims='x')
   rtn = pd->add_signal(byte(30),   name='/a/byte',   dims='x')
   rtn = pd->add_signal(40,         name='/a/short',  dims='x')
   rtn = pd->add_signal(50L,        name='/a/long',   dims='x')
   rtn = pd->add_signal(long64(60), name='/a/long64', dims='x')
   rtn = pd->add_signal(uint(70),   name='/a/ushort', dims='x')
   rtn = pd->add_signal(ulong(80),  name='/a/ulong',  dims='x')
   rtn = pd->add_signal(ulong64(90),name='/a/ulong64',dims='x')
   rtn = pd->add_signal(complex(100.0,110.0), name='/a/complex', dims='x')
   rtn = pd->add_signal(dcomplex(120.0,130.0), name='/a/dcomplex', dims='x')
   
; Close the File 
   
   rtn = pd->close()
   
   obj_destroy, pd
      
   return     
      
  endif

   if (test eq 10) then begin

$rm testnew10.nc

   print, '   '
   print, '------------ TEST 10 --------------'
   print, 'Create new file. Write 1D variables of all types and same dimension. Also attach device to the file.'
   print, '------------------------------------'

   pd = obj_new('putdata', filename='testnew10.nc', directory=directory, title='Test #10', class='raw', $
                shot=123456L, debug=debug, verbose=verbose)
		    
   rtn = pd->add_dim(values=[1.0,2.0,3.0], name='/a/x')

   rtn = pd->add_device(name='abc123', serial="abc321", type="def456", id='identity #1', $
                        resolution=16, range=[100.0,200], channels=32)

   rtn = pd->add_signal([4.0,5.0,6.0], name='/a/float', dims='x', $
                        scale=10.0, offset=-1.0, $
                        units = 'm.A N*s-kg', label = 'test label', title = 'test title', $
                        comment='test comment', device='abc123', channel=1)

   rtn = pd->add_signal(dindgen(3),      name='/a/double', dims='x')
   rtn = pd->add_signal(indgen(3,/byte), name='/a/byte',   dims='x')
   rtn = pd->add_signal(indgen(3),       name='/a/short',  dims='x')
   rtn = pd->add_signal(indgen(3,/long), name='/a/long',   dims='x')
   rtn = pd->add_signal(indgen(3,/l64),  name='/a/long64', dims='x')
   rtn = pd->add_signal(indgen(3,/uint), name='/a/ushort', dims='x')
   rtn = pd->add_signal(indgen(3,/ulong),name='/a/ulong',  dims='x')
   rtn = pd->add_signal(indgen(3,/ul64), name='/a/ulong64',dims='x')
   rtn = pd->add_signal(cindgen(3),      name='/a/complex',  dims='x')
   rtn = pd->add_signal(dcindgen(3),     name='/a/dcomplex', dims='x')

   ; Close the File 
   
   rtn = pd->close()
   
   obj_destroy, pd
      
   return          

   endif

   if (test eq 11) then begin

$rm testnew11.nc

   print, '   '
   print, '------------ TEST 11 --------------'
   print, 'Create new file. Write 2D variables with data of all types and same dimension. Also attach device to the file.'
   print, '------------------------------------'

   pd = obj_new('putdata', filename='testnew11.nc', directory=directory, title='Test #11', class='raw', $
                shot=123456L, debug=debug, verbose=verbose)
		    
   rtn = pd->add_dim(values=[1.0,2.0,3.0], name='/a/x')
   rtn = pd->add_dim(values=[1.0,2.0,3.0,4.0,5.0], name='/a/y')

   rtn = pd->add_device(name='abc123', serial="abc321", type="def456", id='identity #1', $
                        resolution=16, range=[100.0,200], channels=32)

   rtn = pd->add_signal(findgen(3,5), name='/a/float', dims='x,y', $
                        scale=10.0, offset=-1.0, $
                        units = 'm.A N*s-kg', label = 'test label', title = 'test title', $
                        comment='test comment', device='abc123', channel=1)

   rtn = pd->add_signal(dindgen(3,5),      name='/a/double', dims='x,y')
   rtn = pd->add_signal(indgen(3,5,/byte), name='/a/byte',   dims='x,y')
   rtn = pd->add_signal(indgen(3,5),       name='/a/short',  dims='x,y')
   rtn = pd->add_signal(indgen(3,5,/long), name='/a/long',   dims='x,y')
   rtn = pd->add_signal(indgen(3,5,/l64),  name='/a/long64', dims='x,y')
   rtn = pd->add_signal(indgen(3,5,/uint), name='/a/ushort', dims='x,y')
   rtn = pd->add_signal(indgen(3,5,/ulong),name='/a/ulong',  dims='x,y')
   rtn = pd->add_signal(indgen(3,5,/ul64), name='/a/ulong64',dims='x,y')
   rtn = pd->add_signal(cindgen(3,5),      name='/a/complex',  dims='x,y')
   rtn = pd->add_signal(dcindgen(3,5),     name='/a/dcomplex', dims='x,y')

   ; Close the File 
   
   rtn = pd->close()
   
   obj_destroy, pd
      
   return          

   endif

   if (test eq 12) then begin

   print, '   '
   print, '------------ TEST 12 --------------'
   print, 'Create new file. Update 11 to write new variable that does not have full extent of dimensions'
   print, '------------------------------------'

   pd = obj_new('putdata', filename='testnew11.nc', directory=directory, $
                /update, debug=debug, verbose=verbose)

   rtn = pd->add_signal(dindgen(2,4),      name='/a/doubleshort2', dims='x,y')
   rtn = pd->add_signal(dindgen(2),      name='/a/doubleshort3', dims='x')

   ; Close the File 
   
   rtn = pd->close()
   
   obj_destroy, pd

   return

   end   

   if (test eq 13) then begin

   print, '   '
   print, '------------ TEST 13 --------------'
   print, 'Create new file. Write variables with errors at the same time as the signal'
   print, '------------------------------------'

   pd = obj_new('putdata', filename='testnew13.nc', directory=directory, title='Test #13', class='raw', $
                shot=123456L, debug=debug, verbose=verbose)
		    
   rtn = pd->add_dim(values=[1.0,2.0,3.0], name='/a/x')

   rtn = pd->add_signal(dindgen(3),      name='/a/double', dims='x', errors=[1.0D,2.0D,3.0D], $
                        ecomment='Error test', elabel='This is an error')

   ; Close the File 
   
   rtn = pd->close()
   
   obj_destroy, pd

   return

  endif

   if (test eq 14) then begin

   print, '   '
   print, '------------ TEST 14 --------------'
   print, 'Create new file. Write variables with errors after writing the signal'
   print, '------------------------------------'

   pd = obj_new('putdata', filename='testnew14.nc', directory=directory, title='Test #14', class='raw', $
                shot=123456L, debug=debug, verbose=verbose)
		    
   rtn = pd->add_dim(values=[1.0,2.0,3.0], name='/a/x')

   rtn = pd->add_signal(dindgen(3),      name='/a/double', dims='x')

   rtn = pd->add_errors([1.0D, 2.0D, 3.0D], dims='x', name='/a/double_errors', signal='double')

   ; Close the File 
   
   rtn = pd->close()
   
   obj_destroy, pd

   return

  endif

   if (test eq 15) then begin

   print, '   '
   print, '------------ TEST 15 --------------'
   print, 'Create new file. Write variables with errors before writing the signal'
   print, '------------------------------------'

   pd = obj_new('putdata', filename='testnew15.nc', directory=directory, title='Test #15', class='raw', $
                shot=123456L, debug=debug, verbose=verbose)
		    
   rtn = pd->add_dim(values=[1.0,2.0,3.0], name='/a/x')

   rtn = pd->add_errors([1.0D, 2.0D, 3.0D], dims='x', name='/a/double_errors')

   rtn = pd->add_signal(dindgen(3),      name='/a/double', dims='x', errors='double_errors')

   ; Close the File 
   
   rtn = pd->close()
   
   obj_destroy, pd

   return

  endif


   if (test eq 16) then begin

   print, '   '
   print, '------------ TEST 16 --------------'
   print, 'Create new files. Write multiple signals at once.'
   print, '------------------------------------'

   pd_A = obj_new('putdata', filename='testnew16A.nc', directory=directory, title='Test #16A', class='raw', $
                shot=123456L, debug=debug, verbose=verbose)
   pd_B = obj_new('putdata', filename='testnew16B.nc', directory=directory, title='Test #16B', class='raw', $
                shot=123456L, debug=debug, verbose=verbose)
   pd_C = obj_new('putdata', filename='testnew16C.nc', directory=directory, title='Test #16C', class='raw', $
                shot=123456L, debug=debug, verbose=verbose)
   pd_D = obj_new('putdata', filename='testnew16D.nc', directory=directory, title='Test #16D', class='raw', $
                shot=123456L, debug=debug, verbose=verbose)
		    
   rtn = pd_A->add_dim(values=[1.0,2.0,3.0], name='/a/x')
   rtn = pd_B->add_dim(values=[1.0,2.0,3.0,4.0], name='/b/x')
   rtn = pd_C->add_dim(values=[1.0,2.0,3.0,4.0,5.0], name='/c/x')
   rtn = pd_D->add_dim(values=[1.0,2.0,3.0,5.0,6.0,2.0], name='/d/x')   

   rtn = pd_C->add_signal(dindgen(5),      name='/c/double', dims='x')
   rtn = pd_A->add_signal(dindgen(3),      name='/a/double', dims='x')
   rtn = pd_B->add_signal(dindgen(4),      name='/b/double', dims='x')
   rtn = pd_D->add_signal(dindgen(6),      name='/d/double', dims='x')

   ; Close the Files   
   rtn = pd_A->close()
   rtn = pd_B->close()
   rtn = pd_C->close()
   rtn = pd_D->close()
   
   obj_destroy, pd_A
   obj_destroy, pd_B
   obj_destroy, pd_C
   obj_destroy, pd_D
  endif


end
