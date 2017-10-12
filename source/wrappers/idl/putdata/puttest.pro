pro puttest, test 

; Testing IDAM PUT Tools
   
; *** ensure the correct module is loaded, e.g. idam/1.6.3

   debug   = 1
   verbose = 1
   
;-----------------------------------------------------------------------------------------
; #1  Open and Close with Documentation Attributes

   if(test eq 1) then begin
   
$rm test1a.nc 

      fileId=0L
      
      rc = putdata( 'test1a.nc', stepId='create', directory='/home/lkogan/NetCDF/idam/latest/source/idl/netcdf4', $
                    fileId=fileId, conventions='Fusion-1.0', $
		    class='analysed data', title='Test #1A', shot=123456L, pass=789L, $
		    date='02 April 2009', time='09:43', status=1L, $
		    comment='Comment for test #1A', code='puttest.pro', version=99L,$
		    xml='<xml>We can record whatever we want as XML</xml>', $
		    debug=debug, verbose=verbose)
		    
      print,'Test #1A: Open rc  = ', rc
      print,'          fileId   = ', fileId
      
      rc = putdata( stepId='close', fileId=fileId, debug=debug, verbose=verbose)
      print,'          Close rc = ', rc
      print
      
$rm test1b.nc 

      fileId=0L
      
      rc = putdata( '/home/lkogan/NetCDF/idam/latest/source/idl/netcdf4/test1b.nc', stepId='create', $
                    fileId=fileId, conventions='Fusion-1.0', $
		    class='analysed data', title='Test #1B', shot=123456L, pass=789L, $
		    date='02 April 2009', time='09:43', status=1L, $
		    comment='Comment for test #1B', code='puttest.pro', version=99L,$
		    xml='<xml>We can record whatever we want as XML</xml>', $
		    debug=debug, verbose=verbose)
		    
      print,'Test #1B: Open rc  = ', rc
      print,'          fileId   = ', fileId
      
      rc = putdata( stepId='close', fileId=fileId, debug=debug, verbose=verbose)
      print,'          Close rc = ', rc
      print 
      
$rm test1c.nc 

      fileId=0L
      
      rc = putdata( 'test1c.nc', stepId='create', $
                    fileId=fileId, conventions='Fusion-1.0', $
		    class='analysed data', title='Test #1C', shot=123456L, pass=789L, $
		    date='02 April 2009', time='09:43', status=1L, $
		    comment='Comment for test #1C', code='puttest.pro', version=99L,$
		    xml='<xml>We can record whatever we want as XML</xml>', $
		    debug=debug, verbose=verbose)
		    
      print,'Test #1C: Open rc  = ', rc
      print,'          fileId   = ', fileId
      
      rc = putdata( stepId='close', fileId=fileId, debug=debug, verbose=verbose)
      print,'          Close rc = ', rc
      print 
      
wait, 5      
rs=getdata('dump::','netcdf::test1c.nc')
print,string(rs.data)                    
		    
      return   
   endif 
   
;-----------------------------------------------------------------------------------------
; #2  Update the file test1a.nc (Cannot extend the space allocated to previous data entities)

   if(test eq 2) then begin
   
      fileId=0L
      
      rc = putdata( 'test1a.nc', stepId='update', directory='/home/lkogan/NetCDF/idam/latest/source/idl/netcdf4', $
                    fileId=fileId, conventions='Fusion-1.0', $
		    class='raw data', title='Test #2A', shot=654321L, pass=987L, $
		    date='01 April 2009', time='10:52', status=2L, $
		    comment='Comment for test #2A', code='puttest', version=88L,$
		    xml='<xml>Modified XML</xml>', $
		    debug=debug, verbose=verbose)
		    
      print,'Test #2A: Open rc  = ', rc
      print,'          fileId   = ', fileId
      
      rc = putdata( stepId='close', fileId=fileId, debug=debug, verbose=verbose)
      print,'          Close rc = ', rc
      print
 
      rc = putdata( '/home/lkogan/NetCDF/idam/latest/source/idl/netcdf4/test1b.nc', stepId='update', $
                    fileId=fileId, conventions='Fusion-1.0', $
		    class='raw data', title='Test #2B', shot=654321L, pass=987L, $
		    date='01 April 2009', time='10:52', status=2L, $
		    comment='Comment for test #2B', code='puttest', version=88L,$
		    xml='<xml>Modified XML</xml>', $
		    debug=debug, verbose=verbose)
		    
      print,'Test #2B: Open rc  = ', rc
      print,'          fileId   = ', fileId
      
      rc = putdata( stepId='close', fileId=fileId, debug=debug, verbose=verbose)
      print,'          Close rc = ', rc
      print
      
      fileId=0L
      
      rc = putdata( 'test1c.nc', stepId='update', $
                    fileId=fileId, conventions='Fusion-1.0', $
		    class='raw data', title='Test #2C', shot=654321L, pass=987L, $
		    date='01 April 2009', time='10:52', status=2L, $
		    comment='Comment for test #2C', code='puttest', version=88L,$
		    xml='<xml>Modified XML</xml>', $
		    debug=debug, verbose=verbose)
		    
      print,'Test #2C: Open rc  = ', rc
      print,'          fileId   = ', fileId
      
      rc = putdata( stepId='close', fileId=fileId, debug=debug, verbose=verbose)
      print,'          Close rc = ', rc
      print           		    
      return   
   endif    
   
;-----------------------------------------------------------------------------------------
; #3  Add Devices documentation
   
   if(test eq 3) then begin

$rm test3.nc

      fileId=0L
      
      rc = putdata( 'test3.nc', stepId='create', title='Test #3', class='raw data', date='01 April 2009', time='10:52', $
                     conventions='Fusion-1.0', fileId=fileId, debug=debug, verbose=verbose)
    
      rc = putdata( 'dataq1', stepId='device', serial="abc123", type="def456", id='identity #1', $
                    resolution=16, range=[100.0,200], channels=32, debug=debug, verbose=verbose)
      rc = putdata( 'dataq2', stepId='device', serial="abc234", type="def567", id='identity #2', $
                    resolution=16L, range=[100.0D,200D], channels=32L, debug=debug, verbose=verbose)
		    
      rc = putdata( 'new stuff on dataq1', stepId='Attribute', group='/devices/dataq1', name='notes', debug=debug, verbose=verbose)
      rc = putdata( 'new stuff on dataq2', stepId='Attribute', group='/devices/dataq2', name='notes', debug=debug, verbose=verbose)
		    

; Close the File 
 
      rc = putdata(  stepId='close', debug=debug, verbose=verbose) 
      
      return     

   endif  

   
;-----------------------------------------------------------------------------------------
; #4  Create a minimal file and Add Group Level Attributes

   if(test eq 4) then begin

$rm test4.nc
   
      fileId=0L
      
      rc = putdata( 'test4.nc', stepId='create', title='Test #4', class='raw data', conventions='Fusion-1.0', fileId=fileId, debug=debug, verbose=verbose)

; string values
  
      rc = putdata( 'Hello World',  stepId='Attribute', group='/a',  name='stringscalar',  debug=debug, verbose=verbose)

; scalar values
  
      rc = putdata(  3.1415927,  stepId='Attribute', group='/a',  name='floatscalar',  debug=debug, verbose=verbose)
      rc = putdata(  3.1415927D, stepId='Attribute', group='/a',  name='doublescalar', debug=debug, verbose=verbose)
      rc = putdata(  byte(1),    stepId='Attribute', group='/a',  name='bytescalar',   debug=debug, verbose=verbose)
      rc = putdata(  2,          stepId='Attribute', group='/a',  name='shortscalar',  debug=debug, verbose=verbose)
      rc = putdata(  long(3),    stepId='Attribute', group='/a',  name='intscalar',    debug=debug, verbose=verbose)
      rc = putdata(  long64(4),  stepId='Attribute', group='/a',  name='long64scalar', debug=debug, verbose=verbose)
      rc = putdata(  uint(5),    stepId='Attribute', group='/a',  name='ushortscalar', debug=debug, verbose=verbose)
      rc = putdata(  ulong(6),   stepId='Attribute', group='/a',  name='uintscalar',   debug=debug, verbose=verbose)
      rc = putdata(  ulong64(7), stepId='Attribute', group='/a',  name='ulong64scalar',debug=debug, verbose=verbose)

; Write and Overwrite a float scalar

      rc = putdata(  3.1415927,   stepId='Attribute', group='/b',  name='floatscalar', debug=debug, verbose=verbose)
      rc = putdata(  2.718281828, stepId='Attribute', group='/b',  name='floatscalar', debug=debug, verbose=verbose)

; Test Complex (User Defined Type) Attributes

      test = COMPLEX(6.0,7.0)
      rc = putdata(  test, stepId='Attribute', group='/',     name='complex1', debug=debug, verbose=verbose)
      rc = putdata(  test, stepId='Attribute', group='/a',    name='complex1', debug=debug, verbose=verbose)
      rc = putdata(  test, stepId='Attribute', group='/a/b',  name='complex1', debug=debug, verbose=verbose)
      
      test = DCOMPLEX(8.0,9.0)
      rc = putdata(  test, stepId='Attribute', group='/',     name='dcomplex1', debug=debug, verbose=verbose)   
      rc = putdata(  test, stepId='Attribute', group='/a',    name='dcomplex1', debug=debug, verbose=verbose)
      rc = putdata(  test, stepId='Attribute', group='/a/b',  name='dcomplex1', debug=debug, verbose=verbose)
      
; Close the File 
 
      rc = putdata(  stepId='close', debug=debug, verbose=verbose) 
      
      return     

   endif
   
   
;-----------------------------------------------------------------------------------------
; #5  Add Group Level Attribute Arrays (not string arrays - yet - not implemented!)

   if(test eq 5) then begin

      fileId=0L
      
      rc = putdata( 'test4.nc', stepId='update', conventions='Fusion-1.0', fileId=fileId, debug=debug, verbose=verbose)
   
      rc = putdata(  [1.0,2.0,3.0,4.0,5.0],      stepId='Attribute', group='/a/b', name='floatarray',   debug=debug, verbose=verbose)
      rc = putdata(  [1.0D,2.0D,3.0D,4.0D,5.0D], stepId='Attribute', group='/a/b', name='doublearray',  debug=debug, verbose=verbose)
      rc = putdata(  byte([10,20,30,40,50]),     stepId='Attribute', group='/a/b', name='bytearray',    debug=debug, verbose=verbose)
      rc = putdata(  [1,2,3,4,5],                stepId='Attribute', group='/a/b', name='shortarray',   debug=debug, verbose=verbose)
      rc = putdata(  [1L,2L,3L,4L,5L],           stepId='Attribute', group='/a/b', name='intarray',     debug=debug, verbose=verbose)
      rc = putdata(  long64([1,2,3,4,5]),        stepId='Attribute', group='/a/b', name='long64array',  debug=debug, verbose=verbose)
      rc = putdata(  uint([1,2,3,4,5]),          stepId='Attribute', group='/a/b', name='ushortarray',  debug=debug, verbose=verbose)
      rc = putdata(  ulong([1,2,3,4,5]),         stepId='Attribute', group='/a/b', name='uintarray',    debug=debug, verbose=verbose)
      rc = putdata(  ulong64([1,2,3,4,5]),       stepId='Attribute', group='/a/b', name='ulong64array', debug=debug, verbose=verbose)

      test = [COMPLEX(16.0,17.0),COMPLEX(18.0,19.0)]
      rc = putdata(  test, stepId='Attribute', group='/a/b',  name='complexarray', debug=debug, verbose=verbose)

      test = [DCOMPLEX(16.5,17.5),DCOMPLEX(18.5,19.5)]
      rc = putdata(  test, stepId='Attribute', group='/a/b',  name='dcomplexarray', debug=debug, verbose=verbose)

; Close the File 
 
      rc = putdata(  stepId='close', debug=debug, verbose=verbose) 
      
      return     

   endif
   
;-----------------------------------------------------------------------------------------
; #6 Add Dimensions
   
   if(test eq 6) then begin

$rm test6.nc

      fileId=0L
      
      rc = putdata( 'test6.nc', stepId='create', title='Test #6', class='raw data', conventions='Fusion-1.0', fileId=fileId, debug=debug, verbose=verbose)
		    
      rc = putdata( 1, stepId='dimension', group='/a', name='unit', 			debug=debug, verbose=verbose)   
      rc = putdata( 5, stepId='dimension', group='/a', name='y',    			debug=debug, verbose=verbose)   
      rc = putdata( 2, stepId='dimension', group='/a', name='x',    			debug=debug, verbose=verbose)   
      rc = putdata( 0, stepId='dimension', group='/a', name='time', /unlimited, 	debug=debug, verbose=verbose)
      rc = putdata(    stepId='dimension', group='/a', name='t',   		 	debug=debug, verbose=verbose)
 
      rc = putdata( 1, stepId='dimension', group='/a/b', name='single',    		debug=debug, verbose=verbose)   
      rc = putdata( 6, stepId='dimension', group='/a/b', name='y',    			debug=debug, verbose=verbose)   
      rc = putdata( 3, stepId='dimension', group='/a/b', name='x',    			debug=debug, verbose=verbose)   

; Close the File 
 
      rc = putdata(  stepId='close', debug=debug, verbose=verbose) 
      
      return     

   endif       
   
;-----------------------------------------------------------------------------------------
; #7 Add Coordinates: Float Arrays
   
   if(test eq 7) then begin

      fileId=0L

      rc = putdata( 'test6.nc', stepId='update', fileId=fileId, debug=debug, verbose=verbose)
            		             		    
      rc = putdata( [1.0,2.0,3.0,4.0,5.0], stepId='coordinate', group='/a',   name='y', units='m', $
                    label='y-label', debug=debug, verbose=verbose)           
if(1) then begin
      rc = putdata( [-1.0D,-2.0D],           stepId='coordinate', group='/a',   name='x', units='A', $ 
                    label='x-label', debug=debug, verbose=verbose)

      rc = putdata( [1.0,2.0,3.0,4.0,5.0,6.0], stepId='coordinate',  group='/a/b', name='y', units='kg', $  
                    label='y-label', debug=debug, verbose=verbose)		    

      rc = putdata( [-1.0D,-2.0D,-3.0D],           stepId='coordinate', group='/a/b', name='x', units='N', $  
                    label='x-label', debug=debug, verbose=verbose)
     
; Coordinates with unlimited lengths
      
      rc = putdata( [-1.0,0.0,1.0,2.0,3.0,4.0], stepId='coordinate', group='/a', name='time', units='s', $ 
                    label='time', class='time', debug=debug, verbose=verbose)
      rc = putdata( [-4.0D,0.0D,4.0D], stepId='coordinate', group='/a', name='t', units='s', $ 
                    label='time', class='time', debug=debug, verbose=verbose)
endif
; Close the File 
 
      rc = putdata(  stepId='close', debug=debug, verbose=verbose) 
      return     

   endif 
   
;-----------------------------------------------------------------------------------------
; #8 Add Coordinates: Various Scalar Types
   
   if(test eq 8) then begin

      fileId=0L

      rc = putdata( 'test6.nc', stepId='update', fileId=fileId, debug=debug, verbose=verbose)
            		             		    
      rc = putdata( 3.14159, stepId='coordinate', group='/a',   name='unit', units='counts', $
                    label='unitary pi value', debug=debug, verbose=verbose)           
		    
      rc = putdata( 2.71828D, stepId='coordinate', group='/a/b',   name='single', units='counts', $
                    label='unitary e value', debug=debug, verbose=verbose)  
		    
      rc = putdata( 1, stepId='dimension', group='/a/b/c', name='byte',     debug=debug, verbose=verbose)   
      rc = putdata( 1, stepId='dimension', group='/a/b/c', name='short',    debug=debug, verbose=verbose)   
      rc = putdata( 1, stepId='dimension', group='/a/b/c', name='int',      debug=debug, verbose=verbose)   
      rc = putdata( 1, stepId='dimension', group='/a/b/c', name='long',     debug=debug, verbose=verbose)   
      rc = putdata( 1, stepId='dimension', group='/a/b/c', name='ushort',   debug=debug, verbose=verbose)   
      rc = putdata( 1, stepId='dimension', group='/a/b/c', name='uint',     debug=debug, verbose=verbose)   
      rc = putdata( 1, stepId='dimension', group='/a/b/c', name='ulong',    debug=debug, verbose=verbose)   
      rc = putdata( 1, stepId='dimension', group='/a/b/c', name='complex',  debug=debug, verbose=verbose)   
      rc = putdata( 1, stepId='dimension', group='/a/b/c', name='dcomplex', debug=debug, verbose=verbose)   
 
      rc = putdata( byte(10),    stepId='coordinate', group='/a/b/c', name='byte',   debug=debug, verbose=verbose)   
      rc = putdata( 20,          stepId='coordinate', group='/a/b/c', name='short',  debug=debug, verbose=verbose)   
      rc = putdata( 30L,         stepId='coordinate', group='/a/b/c', name='int',    debug=debug, verbose=verbose)   
      rc = putdata( long64(40),  stepId='coordinate', group='/a/b/c', name='long',   debug=debug, verbose=verbose)   
      rc = putdata( uint(50),    stepId='coordinate', group='/a/b/c', name='ushort', debug=debug, verbose=verbose)   
      rc = putdata( ulong(60),   stepId='coordinate', group='/a/b/c', name='uint',   debug=debug, verbose=verbose)   
      rc = putdata( ulong64(70), stepId='coordinate', group='/a/b/c', name='ulong',  debug=debug, verbose=verbose)   
      rc = putdata( complex(80.0,90.0),  stepId='coordinate', group='/a/b/c', name='complex',   debug=debug, verbose=verbose)   
      rc = putdata( dcomplex(80.0,90.0), stepId='coordinate', group='/a/b/c', name='dcomplex',  debug=debug, verbose=verbose)   
		                  
; Close the File 
 
      rc = putdata(  stepId='close', debug=debug, verbose=verbose) 
      return     

   endif    

;-----------------------------------------------------------------------------------------
; #9 Add Coordinates: Various array types
   
   if(test eq 9) then begin

$rm test9.nc

      fileId=0L
      
      rc = putdata( 'test9.nc', stepId='create', title='Test #9', class='raw data', conventions='Fusion-1.0', fileId=fileId, debug=debug, verbose=verbose)
		    
      rc = putdata( 3, stepId='dimension', group='/a', name='float',    debug=debug, verbose=verbose)   
      rc = putdata( 3, stepId='dimension', group='/a', name='double',   debug=debug, verbose=verbose)   
      rc = putdata( 3, stepId='dimension', group='/a', name='byte',     debug=debug, verbose=verbose)   
      rc = putdata( 3, stepId='dimension', group='/a', name='short',    debug=debug, verbose=verbose)   
      rc = putdata( 3, stepId='dimension', group='/a', name='int',      debug=debug, verbose=verbose)   
      rc = putdata( 3, stepId='dimension', group='/a', name='long',     debug=debug, verbose=verbose)   
      rc = putdata( 3, stepId='dimension', group='/a', name='ushort',   debug=debug, verbose=verbose)   
      rc = putdata( 3, stepId='dimension', group='/a', name='uint',     debug=debug, verbose=verbose)   
      rc = putdata( 3, stepId='dimension', group='/a', name='ulong',    debug=debug, verbose=verbose)   
      rc = putdata( 3, stepId='dimension', group='/a', name='complex',  debug=debug, verbose=verbose)   
      rc = putdata( 3, stepId='dimension', group='/a', name='dcomplex', debug=debug, verbose=verbose)   
 
      rc = putdata( findgen(3),       stepId='coordinate', group='/a', name='float',  debug=debug, verbose=verbose)   
      rc = putdata( dindgen(3),       stepId='coordinate', group='/a', name='double', debug=debug, verbose=verbose)   
      rc = putdata( indgen(3,/byte),  stepId='coordinate', group='/a', name='byte',   debug=debug, verbose=verbose)   
      rc = putdata( indgen(3),        stepId='coordinate', group='/a', name='short',  debug=debug, verbose=verbose)   
      rc = putdata( indgen(3,/long),  stepId='coordinate', group='/a', name='int',    debug=debug, verbose=verbose)   
      rc = putdata( indgen(3,/l64),   stepId='coordinate', group='/a', name='long',   debug=debug, verbose=verbose)   
      rc = putdata( indgen(3,/uint),  stepId='coordinate', group='/a', name='ushort', debug=debug, verbose=verbose)   
      rc = putdata( indgen(3,/ulong), stepId='coordinate', group='/a', name='uint',   debug=debug, verbose=verbose)   
      rc = putdata( indgen(3,/ul64),  stepId='coordinate', group='/a', name='ulong',  debug=debug, verbose=verbose)   
      rc = putdata( cindgen(3),       stepId='coordinate', group='/a', name='complex',   debug=debug, verbose=verbose)   
      rc = putdata( dcindgen(3),      stepId='coordinate', group='/a', name='dcomplex',  debug=debug, verbose=verbose)   
		                  
; Close the File 
 
      rc = putdata(  stepId='close', debug=debug, verbose=verbose) 
      return     

   endif 
   
;-----------------------------------------------------------------------------------------
; #10 Add Coordinates: Various array types with Unlimited Length
   
   if(test eq 10) then begin

$rm test10.nc

      fileId=0L
      
      rc = putdata( 'test10.nc', stepId='create', title='Test #10', class='raw data', conventions='Fusion-1.0', fileId=fileId, debug=debug, verbose=verbose)
		    
      rc = putdata( 0, stepId='dimension', group='/a', name='float',    /unlimited, debug=debug, verbose=verbose)   
      rc = putdata( 0, stepId='dimension', group='/a', name='double',   /unlimited, debug=debug, verbose=verbose)   
      rc = putdata( 0, stepId='dimension', group='/a', name='byte',     /unlimited, debug=debug, verbose=verbose)   
      rc = putdata( 0, stepId='dimension', group='/a', name='short',    /unlimited, debug=debug, verbose=verbose)   
      rc = putdata( 0, stepId='dimension', group='/a', name='int',      /unlimited, debug=debug, verbose=verbose)   
      rc = putdata( 0, stepId='dimension', group='/a', name='long',     /unlimited, debug=debug, verbose=verbose)   
      rc = putdata( 0, stepId='dimension', group='/a', name='ushort',   /unlimited, debug=debug, verbose=verbose)   
      rc = putdata( 0, stepId='dimension', group='/a', name='uint',     /unlimited, debug=debug, verbose=verbose)   
      rc = putdata( 0, stepId='dimension', group='/a', name='ulong',    /unlimited, debug=debug, verbose=verbose)   
      rc = putdata( 0, stepId='dimension', group='/a', name='complex',  /unlimited, debug=debug, verbose=verbose)   
      rc = putdata( 0, stepId='dimension', group='/a', name='dcomplex', /unlimited, debug=debug, verbose=verbose)   
 
      rc = putdata( findgen(3),       stepId='coordinate', group='/a', name='float',  debug=debug, verbose=verbose)   
      rc = putdata( dindgen(3),       stepId='coordinate', group='/a', name='double', debug=debug, verbose=verbose)   
      rc = putdata( indgen(3,/byte),  stepId='coordinate', group='/a', name='byte',   debug=debug, verbose=verbose)   
      rc = putdata( indgen(3),        stepId='coordinate', group='/a', name='short',  debug=debug, verbose=verbose)   
      rc = putdata( indgen(3,/long),  stepId='coordinate', group='/a', name='int',    debug=debug, verbose=verbose)   
      rc = putdata( indgen(3,/l64),   stepId='coordinate', group='/a', name='long',   debug=debug, verbose=verbose)   
      rc = putdata( indgen(3,/uint),  stepId='coordinate', group='/a', name='ushort', debug=debug, verbose=verbose)   
      rc = putdata( indgen(3,/ulong), stepId='coordinate', group='/a', name='uint',   debug=debug, verbose=verbose)   
      rc = putdata( indgen(3,/ul64),  stepId='coordinate', group='/a', name='ulong',  debug=debug, verbose=verbose)   
      rc = putdata( cindgen(3),       stepId='coordinate', group='/a', name='complex',   debug=debug, verbose=verbose)   
      rc = putdata( dcindgen(3),      stepId='coordinate', group='/a', name='dcomplex',  debug=debug, verbose=verbose)   
		                  
; Close the File 
 
      rc = putdata(  stepId='close', debug=debug, verbose=verbose) 
      return     

   endif           
             
;-----------------------------------------------------------------------------------------
; #11 Coordinates: Domain Data

   if(test eq 11) then begin

$rm test11.nc

      fileId=0L
      
      rc = putdata( 'test11.nc', stepId='create', title='Test #11', class='raw data', conventions='Fusion-1.0', fileId=fileId, debug=debug, verbose=verbose)
		    
      rc = putdata( 3, stepId='dimension', group='/a', name='m', debug=debug, verbose=verbose) 
      rc = putdata( 3, stepId='dimension', group='/a', name='n', debug=debug, verbose=verbose) 
      rc = putdata( 3, stepId='dimension', group='/a', name='o', debug=debug, verbose=verbose) 
      rc = putdata( 3, stepId='dimension', group='/a', name='p', debug=debug, verbose=verbose) 
      
      start     = 0.0D
      increment = 0.5D
      count     = ulong(3)
        
      rc = putdata(                    start, increment,        stepId='coordinate', group='/a', name='m',  debug=debug, verbose=verbose)   
      rc = putdata(                    start, increment, count, stepId='coordinate', group='/a', name='n',  debug=debug, verbose=verbose)   
      rc = putdata( [-1.0,-2.0, -3.0], start, increment,        stepId='coordinate', group='/a', name='o',  debug=debug, verbose=verbose)   
      rc = putdata( [-1.0,-2.0, -3.0], start, increment, count, stepId='coordinate', group='/a', name='p',  debug=debug, verbose=verbose)   
 
; Close the File 
 
      rc = putdata(  stepId='close', debug=debug, verbose=verbose) 
      
      return     

   endif      
   
             
;-----------------------------------------------------------------------------------------
; #12 Measurement Data: Scalar types with fixed length dimension

   if(test eq 12) then begin

$rm test12.nc

      fileId=0L
      
      rc = putdata( 'test12.nc', stepId='create', title='Test #12', class='raw data', conventions='Fusion-1.0', fileId=fileId, debug=debug, verbose=verbose)
		    
      rc = putdata( 1, stepId='dimension', group='/a', name='x', debug=debug, verbose=verbose)               
      rc = putdata( 3.1415927, stepId='coordinate', group='/a', name='x',  debug=debug, verbose=verbose)   

      rc = putdata( 'abc123', stepId='device', serial="abc321", type="def456", id='identity #1', $
                    resolution=16, range=[100.0,200], channels=32, debug=debug, verbose=verbose)

      rc = putdata( 1.0E3, stepId='variable', group='/a', name='float', dimensions='x', $
                    scale=10.0, offset=-1.0, $
		    units = 'counts.m/s^2', label = 'test label', title = 'test title', $
                    comment='test comment', device='abc123', channel=1, fileid=fileid, $  
		    debug=debug, verbose=verbose)   
      print,'FILE ID =', fileid 
      
      rc = putdata( 2.0D3,      stepId='variable', group='/a', name='double', dimensions='x', debug=debug, verbose=verbose)
      rc = putdata( byte(30),   stepId='variable', group='/a', name='byte',   dimensions='x', debug=debug, verbose=verbose)
      rc = putdata( 40,         stepId='variable', group='/a', name='short',  dimensions='x', debug=debug, verbose=verbose)
      rc = putdata( 50L,        stepId='variable', group='/a', name='long',   dimensions='x', debug=debug, verbose=verbose)
      rc = putdata( long64(60), stepId='variable', group='/a', name='long64', dimensions='x', debug=debug, verbose=verbose)
      rc = putdata( uint(70),   stepId='variable', group='/a', name='ushort', dimensions='x', debug=debug, verbose=verbose)
      rc = putdata( ulong(80),  stepId='variable', group='/a', name='ulong',  dimensions='x', debug=debug, verbose=verbose)
      rc = putdata( ulong64(90),stepId='variable', group='/a', name='ulong64',dimensions='x', debug=debug, verbose=verbose)
      rc = putdata( complex(100.0,110.0), stepId='variable', group='/a', name='complex', dimensions='x', debug=debug, verbose=verbose)
      rc = putdata( dcomplex(120.0,130.0), stepId='variable', group='/a', name='dcomplex', dimensions='x', debug=debug, verbose=verbose)
 
; Close the File 
 
      rc = putdata(  stepId='close', debug=debug, verbose=verbose) 
      
      return     

   endif    
   
;-----------------------------------------------------------------------------------------
; #13 Measurement Data: Rank 1 arrays of fixed length

   if(test eq 13) then begin

$rm test13.nc

      fileId=0L
      
      rc = putdata( 'test13.nc', stepId='create', title='Test #13', class='raw data', conventions='Fusion-1.0', fileId=fileId, debug=debug, verbose=verbose)
		    
      rc = putdata( 3, stepId='dimension', group='/a', name='x', debug=debug, verbose=verbose)               
      rc = putdata( [1.0,2.0,3.0], stepId='coordinate', group='/a', name='x',  debug=debug, verbose=verbose)   

      rc = putdata( 'abc123', stepId='device', serial="abc321", type="def456", id='identity #1', $
                    resolution=16, range=[100.0,200], channels=32, debug=debug, verbose=verbose)

      rc = putdata( [4.0,5.0,6.0], stepId='variable', group='/a', name='float', dimensions='x', $
                    scale=10.0, offset=-1.0, $
		    units = 'm.A N*s-kg', label = 'test label', title = 'test title', $
                    comment='test comment', device='abc123', channel=1, fileid=fileid, $  
		    debug=debug, verbose=verbose)   
      print,'FILE ID =', fileid  
      
      rc = putdata( dindgen(3),      stepId='variable', group='/a', name='double', dimensions='x', debug=debug, verbose=verbose)
      rc = putdata( indgen(3,/byte), stepId='variable', group='/a', name='byte',   dimensions='x', debug=debug, verbose=verbose)
      rc = putdata( indgen(3),       stepId='variable', group='/a', name='short',  dimensions='x', debug=debug, verbose=verbose)
      rc = putdata( indgen(3,/long), stepId='variable', group='/a', name='long',   dimensions='x', debug=debug, verbose=verbose)
      rc = putdata( indgen(3,/l64),  stepId='variable', group='/a', name='long64', dimensions='x', debug=debug, verbose=verbose)
      rc = putdata( indgen(3,/uint), stepId='variable', group='/a', name='ushort', dimensions='x', debug=debug, verbose=verbose)
      rc = putdata( indgen(3,/ulong),stepId='variable', group='/a', name='ulong',  dimensions='x', debug=debug, verbose=verbose)
      rc = putdata( indgen(3,/ul64), stepId='variable', group='/a', name='ulong64',dimensions='x', debug=debug, verbose=verbose)
      rc = putdata( cindgen(3),      stepId='variable', group='/a', name='complex',  dimensions='x', debug=debug, verbose=verbose)
      rc = putdata( dcindgen(3),     stepId='variable', group='/a', name='dcomplex', dimensions='x', debug=debug, verbose=verbose)
      
; Close the File 
 
      rc = putdata(  stepId='close', debug=debug, verbose=verbose) 
      
      return     

   endif               
   
;-----------------------------------------------------------------------------------------
; #14 Measurement Data: Rank 2 arrays of fixed length

   if(test eq 14) then begin

$rm test14.nc

      fileId=0L
      
      rc = putdata( 'test14.nc', stepId='create', title='Test #14', class='raw data', conventions='Fusion-1.0', fileId=fileId, debug=debug, verbose=verbose)
		    
      rc = putdata( 3, stepId='dimension', group='/a', name='x', debug=debug, verbose=verbose)               
      rc = putdata( 2, stepId='dimension', group='/a', name='y', debug=debug, verbose=verbose)               
      rc = putdata( [1.0,2.0,3.0], stepId='coordinate', group='/a', name='x',  debug=debug, verbose=verbose)   
      rc = putdata( [4.0,5.0],     stepId='coordinate', group='/a', name='y',  debug=debug, verbose=verbose)   

      rc = putdata( 'abc123', stepId='device', serial="abc321", type="def456", id='identity #1', $
                    resolution=16, range=[100.0,200], channels=32, debug=debug, verbose=verbose)

      rc = putdata( findgen(3,2), stepId='variable', group='/a', name='float', dimensions='x,y', $
                    scale=10.0, offset=-1.0, $
		    units = 'm', label = 'test label', title = 'test title', $
                    comment='test comment', device='abc123', channel=1, fileid=fileid, $  
		    debug=debug, verbose=verbose)   
      print,'FILE ID =', fileid  
      
      rc = putdata( dindgen(3,2),      stepId='variable', group='/a', name='double', dimensions='x,y', debug=debug, verbose=verbose)
      rc = putdata( indgen(3,2,/byte), stepId='variable', group='/a', name='byte',   dimensions='x,y', debug=debug, verbose=verbose)
      rc = putdata( indgen(3,2),       stepId='variable', group='/a', name='short',  dimensions='x,y', debug=debug, verbose=verbose)
      rc = putdata( indgen(3,2,/long), stepId='variable', group='/a', name='long',   dimensions='x,y', debug=debug, verbose=verbose)
      rc = putdata( indgen(3,2,/l64),  stepId='variable', group='/a', name='long64', dimensions='x,y', debug=debug, verbose=verbose)
      rc = putdata( indgen(3,2,/uint), stepId='variable', group='/a', name='ushort', dimensions='x,y', debug=debug, verbose=verbose)
      rc = putdata( indgen(3,2,/ulong),stepId='variable', group='/a', name='ulong',  dimensions='x,y', debug=debug, verbose=verbose)
      rc = putdata( indgen(3,2,/ul64), stepId='variable', group='/a', name='ulong64',dimensions='x,y', debug=debug, verbose=verbose)
      rc = putdata( cindgen(3,2),      stepId='variable', group='/a', name='complex',  dimensions='x,y', debug=debug, verbose=verbose)
      rc = putdata( dcindgen(3,2),     stepId='variable', group='/a', name='dcomplex', dimensions='x,y', debug=debug, verbose=verbose)
      
; Close the File 
 
      rc = putdata(  stepId='close', debug=debug, verbose=verbose) 
      
      return     

   endif    
   
;-----------------------------------------------------------------------------------------
; #15 Measurement Data: Rank 3 arrays of fixed length

   if(test eq 15) then begin

$rm test15.nc

      fileId=0L
      
      rc = putdata( 'test15.nc', stepId='create', title='Test #15', class='raw data', conventions='Fusion-1.0', fileId=fileId, debug=debug, verbose=verbose)
		    
      rc = putdata( 3, stepId='dimension', group='/a', name='x', debug=debug, verbose=verbose)               
      rc = putdata( 2, stepId='dimension', group='/a', name='y', debug=debug, verbose=verbose)               
      rc = putdata( 2, stepId='dimension', group='/a', name='z', debug=debug, verbose=verbose)               
      rc = putdata( [1.0,2.0,3.0], stepId='coordinate', group='/a', name='x',  debug=debug, verbose=verbose)   
      rc = putdata( [4.0,5.0],     stepId='coordinate', group='/a', name='y',  debug=debug, verbose=verbose)   
      rc = putdata( [6.0,7.0],     stepId='coordinate', group='/a', name='z',  debug=debug, verbose=verbose)   

      rc = putdata( 'abc123', stepId='device', serial="abc321", type="def456", id='identity #1', $
                    resolution=16, range=[100.0,200], channels=32, debug=debug, verbose=verbose)

      rc = putdata( findgen(3,2,2), stepId='variable', group='/a', name='float', dimensions='x,y,z', $
                    scale=10.0, offset=-1.0, $
		    units = 's', label = 'test label', title = 'test title', $
                    comment='test comment', device='abc123', channel=1, fileid=fileid, $  
		    debug=debug, verbose=verbose)   
      print,'FILE ID =', fileid  
      
      rc = putdata( dindgen(3,2,2),      stepId='variable', group='/a', name='double', dimensions='x,y,z', debug=debug, verbose=verbose)
      rc = putdata( indgen(3,2,2,/byte), stepId='variable', group='/a', name='byte',   dimensions='x,y,z', debug=debug, verbose=verbose)
      rc = putdata( indgen(3,2,2),       stepId='variable', group='/a', name='short',  dimensions='x,y,z', debug=debug, verbose=verbose)
      rc = putdata( indgen(3,2,2,/long), stepId='variable', group='/a', name='long',   dimensions='x,y,z', debug=debug, verbose=verbose)
      rc = putdata( indgen(3,2,2,/l64),  stepId='variable', group='/a', name='long64', dimensions='x,y,z', debug=debug, verbose=verbose)
      rc = putdata( indgen(3,2,2,/uint), stepId='variable', group='/a', name='ushort', dimensions='x,y,z', debug=debug, verbose=verbose)
      rc = putdata( indgen(3,2,2,/ulong),stepId='variable', group='/a', name='ulong',  dimensions='x,y,z', debug=debug, verbose=verbose)
      rc = putdata( indgen(3,2,2,/ul64), stepId='variable', group='/a', name='ulong64',dimensions='x,y,z', debug=debug, verbose=verbose)
      rc = putdata( cindgen(3,2,2),      stepId='variable', group='/a', name='complex',  dimensions='x,y,z', debug=debug, verbose=verbose)
      rc = putdata( dcindgen(3,2,2),     stepId='variable', group='/a', name='dcomplex', dimensions='x,y,z', debug=debug, verbose=verbose)
      
; Close the File 
 
      rc = putdata(  stepId='close', debug=debug, verbose=verbose) 
      
      return     

   endif  
   
;-----------------------------------------------------------------------------------------
; #16 Measurement Data: Scalar types with unlimited length dimension

   if(test eq 16) then begin

$rm test16.nc

      rc = putdata( 'test16.nc', stepId='create', title='Test #16', class='raw data', conventions='Fusion-1.0', debug=debug, verbose=verbose)
		    
      rc = putdata( stepId='dimension', group='/a', name='x', /unlimited, debug=debug, verbose=verbose)               

      rc = putdata( 1.0D3,      stepId='variable', group='/a', name='float',  dimensions='x', debug=debug, verbose=verbose)     
      rc = putdata( 2.0D3,      stepId='variable', group='/a', name='double', dimensions='x', debug=debug, verbose=verbose)
      rc = putdata( byte(30),   stepId='variable', group='/a', name='byte',   dimensions='x', debug=debug, verbose=verbose)
      rc = putdata( 40,         stepId='variable', group='/a', name='short',  dimensions='x', debug=debug, verbose=verbose)
      rc = putdata( 50L,        stepId='variable', group='/a', name='long',   dimensions='x', debug=debug, verbose=verbose)
      rc = putdata( long64(60), stepId='variable', group='/a', name='long64', dimensions='x', debug=debug, verbose=verbose)
      rc = putdata( uint(70),   stepId='variable', group='/a', name='ushort', dimensions='x', debug=debug, verbose=verbose)
      rc = putdata( ulong(80),  stepId='variable', group='/a', name='ulong',  dimensions='x', debug=debug, verbose=verbose)
      rc = putdata( ulong64(90),stepId='variable', group='/a', name='ulong64',dimensions='x', debug=debug, verbose=verbose)
      rc = putdata( complex(100.0,110.0), stepId='variable', group='/a', name='complex', dimensions='x', debug=debug, verbose=verbose)
      rc = putdata( dcomplex(120.0,130.0), stepId='variable', group='/a', name='dcomplex', dimensions='x', debug=debug, verbose=verbose)

; Close the File 
 
      rc = putdata(  stepId='close', debug=debug, verbose=verbose) 
      
      return     

   endif       
   
;-----------------------------------------------------------------------------------------
; #17 Measurement Data: Rank 1 arrays of unlimited length

   if(test eq 17) then begin

$rm test17.nc

      rc = putdata( 'test17.nc', stepId='create', title='Test #17', class='raw data', conventions='Fusion-1.0', debug=debug, verbose=verbose)
		    
      rc = putdata( stepId='dimension', group='/a', name='x', /unlimited, debug=debug, verbose=verbose)               

      rc = putdata( findgen(3),      stepId='variable', group='/a', name='float',  dimensions='x', debug=debug, verbose=verbose)      
      rc = putdata( dindgen(3),      stepId='variable', group='/a', name='double', dimensions='x', debug=debug, verbose=verbose)
      rc = putdata( indgen(3,/byte), stepId='variable', group='/a', name='byte',   dimensions='x', debug=debug, verbose=verbose)
      rc = putdata( indgen(3),       stepId='variable', group='/a', name='short',  dimensions='x', debug=debug, verbose=verbose)
      rc = putdata( indgen(3,/long), stepId='variable', group='/a', name='long',   dimensions='x', debug=debug, verbose=verbose)
      rc = putdata( indgen(3,/l64),  stepId='variable', group='/a', name='long64', dimensions='x', debug=debug, verbose=verbose)
      rc = putdata( indgen(3,/uint), stepId='variable', group='/a', name='ushort', dimensions='x', debug=debug, verbose=verbose)
      rc = putdata( indgen(3,/ulong),stepId='variable', group='/a', name='ulong',  dimensions='x', debug=debug, verbose=verbose)
      rc = putdata( indgen(3,/ul64), stepId='variable', group='/a', name='ulong64',dimensions='x', debug=debug, verbose=verbose)
      rc = putdata( cindgen(3),      stepId='variable', group='/a', name='complex',  dimensions='x', debug=debug, verbose=verbose)
      rc = putdata( dcindgen(3),     stepId='variable', group='/a', name='dcomplex', dimensions='x', debug=debug, verbose=verbose)
      
; Close the File 
 
      rc = putdata(  stepId='close', debug=debug, verbose=verbose) 
      
      return     

   endif               
      
;-----------------------------------------------------------------------------------------
; #18 Measurement Data: Rank 2 arrays of unlimited length

   if(test eq 18) then begin

$rm test18.nc

      rc = putdata( 'test18.nc', stepId='create', title='Test #18', class='raw data', conventions='Fusion-1.0', debug=debug, verbose=verbose)
		    
      rc = putdata( stepId='dimension', group='/a', name='x', /unlimited, debug=debug, verbose=verbose)               
      rc = putdata( stepId='dimension', group='/a', name='y', /unlimited, debug=debug, verbose=verbose)               
      rc = putdata( 3, stepId='dimension', group='/a', name='x3',debug=debug, verbose=verbose)               
      rc = putdata( 2, stepId='dimension', group='/a', name='y2',debug=debug, verbose=verbose)               
      
      rc = putdata( findgen(3,2),      stepId='variable', group='/a', name='float',  dimensions='x,y', debug=debug, verbose=verbose)
      rc = putdata( dindgen(3,2),      stepId='variable', group='/a', name='double', dimensions='x,y', debug=debug, verbose=verbose)
      rc = putdata( indgen(3,2,/byte), stepId='variable', group='/a', name='byte',   dimensions='x,y', debug=debug, verbose=verbose)
      rc = putdata( indgen(3,2),       stepId='variable', group='/a', name='short',  dimensions='x,y', debug=debug, verbose=verbose)
      rc = putdata( indgen(3,2,/long), stepId='variable', group='/a', name='long',   dimensions='x,y', debug=debug, verbose=verbose)
      rc = putdata( indgen(3,2,/l64),  stepId='variable', group='/a', name='long64', dimensions='x,y', debug=debug, verbose=verbose)
      rc = putdata( indgen(3,2,/uint), stepId='variable', group='/a', name='ushort', dimensions='x,y', debug=debug, verbose=verbose)
      rc = putdata( indgen(3,2,/ulong),stepId='variable', group='/a', name='ulong',  dimensions='x,y', debug=debug, verbose=verbose)
      rc = putdata( indgen(3,2,/ul64), stepId='variable', group='/a', name='ulong64',dimensions='x,y', debug=debug, verbose=verbose)
      rc = putdata( cindgen(3,2),      stepId='variable', group='/a', name='complex',  dimensions='x,y', debug=debug, verbose=verbose)
      rc = putdata( dcindgen(3,2),     stepId='variable', group='/a', name='dcomplex', dimensions='x,y', debug=debug, verbose=verbose)
      
      rc = putdata( findgen(3,2),      stepId='variable', group='/a', name='float3',  dimensions='x3,y', debug=debug, verbose=verbose)
      rc = putdata( dindgen(3,2),      stepId='variable', group='/a', name='double3', dimensions='x3,y', debug=debug, verbose=verbose)
      rc = putdata( indgen(3,2,/byte), stepId='variable', group='/a', name='byte3',   dimensions='x3,y', debug=debug, verbose=verbose)
      rc = putdata( indgen(3,2),       stepId='variable', group='/a', name='short3',  dimensions='x3,y', debug=debug, verbose=verbose)
      rc = putdata( indgen(3,2,/long), stepId='variable', group='/a', name='long3',   dimensions='x3,y', debug=debug, verbose=verbose)
      rc = putdata( indgen(3,2,/l64),  stepId='variable', group='/a', name='long643', dimensions='x3,y', debug=debug, verbose=verbose)
      rc = putdata( indgen(3,2,/uint), stepId='variable', group='/a', name='ushort3', dimensions='x3,y', debug=debug, verbose=verbose)
      rc = putdata( indgen(3,2,/ulong),stepId='variable', group='/a', name='ulong3',  dimensions='x3,y', debug=debug, verbose=verbose)
      rc = putdata( indgen(3,2,/ul64), stepId='variable', group='/a', name='ulong643',dimensions='x3,y', debug=debug, verbose=verbose)
      rc = putdata( cindgen(3,2),      stepId='variable', group='/a', name='complex3',  dimensions='x3,y', debug=debug, verbose=verbose)
      rc = putdata( dcindgen(3,2),     stepId='variable', group='/a', name='dcomplex3', dimensions='x3,y', debug=debug, verbose=verbose)

      rc = putdata( findgen(3,2),      stepId='variable', group='/a', name='float2',  dimensions='x,y2', debug=debug, verbose=verbose)
      rc = putdata( dindgen(3,2),      stepId='variable', group='/a', name='double2', dimensions='x,y2', debug=debug, verbose=verbose)
      rc = putdata( indgen(3,2,/byte), stepId='variable', group='/a', name='byte2',   dimensions='x,y2', debug=debug, verbose=verbose)
      rc = putdata( indgen(3,2),       stepId='variable', group='/a', name='short2',  dimensions='x,y2', debug=debug, verbose=verbose)
      rc = putdata( indgen(3,2,/long), stepId='variable', group='/a', name='long2',   dimensions='x,y2', debug=debug, verbose=verbose)
      rc = putdata( indgen(3,2,/l64),  stepId='variable', group='/a', name='long642', dimensions='x,y2', debug=debug, verbose=verbose)
      rc = putdata( indgen(3,2,/uint), stepId='variable', group='/a', name='ushort2', dimensions='x,y2', debug=debug, verbose=verbose)
      rc = putdata( indgen(3,2,/ulong),stepId='variable', group='/a', name='ulong2',  dimensions='x,y2', debug=debug, verbose=verbose)
      rc = putdata( indgen(3,2,/ul64), stepId='variable', group='/a', name='ulong642',dimensions='x,y2', debug=debug, verbose=verbose)
      rc = putdata( cindgen(3,2),      stepId='variable', group='/a', name='complex2',  dimensions='x,y2', debug=debug, verbose=verbose)
      rc = putdata( dcindgen(3,2),     stepId='variable', group='/a', name='dcomplex2', dimensions='x,y2', debug=debug, verbose=verbose)
       
; Close the File 
 
      rc = putdata(  stepId='close', debug=debug, verbose=verbose) 
      
      return     

   endif       

;-----------------------------------------------------------------------------------------
; #19 Measurement Data: Rank 3 arrays of unlimited length

   if(test eq 19) then begin

$rm test19.nc

      rc = putdata( 'test19.nc', stepId='create', title='Test #19', class='raw data', conventions='Fusion-1.0', debug=debug, verbose=verbose)

      rc = putdata( stepId='dimension', group='/a', name='x', /unlimited, debug=debug, verbose=verbose)               
      rc = putdata( stepId='dimension', group='/a', name='y', /unlimited, debug=debug, verbose=verbose)               
      rc = putdata( stepId='dimension', group='/a', name='z', /unlimited, debug=debug, verbose=verbose)               
      rc = putdata( 3, stepId='dimension', group='/a', name='x3',debug=debug, verbose=verbose)               
      rc = putdata( 2, stepId='dimension', group='/a', name='y2',debug=debug, verbose=verbose)               
      rc = putdata( 2, stepId='dimension', group='/a', name='z2',debug=debug, verbose=verbose)               
		    
      rc = putdata( findgen(3,2,2),      stepId='variable', group='/a', name='float',  dimensions='x,y,z', debug=debug, verbose=verbose)      
      rc = putdata( dindgen(3,2,2),      stepId='variable', group='/a', name='double', dimensions='x,y,z', debug=debug, verbose=verbose)
      rc = putdata( indgen(3,2,2,/byte), stepId='variable', group='/a', name='byte',   dimensions='x,y,z', debug=debug, verbose=verbose)
      rc = putdata( indgen(3,2,2),       stepId='variable', group='/a', name='short',  dimensions='x,y,z', debug=debug, verbose=verbose)
      rc = putdata( indgen(3,2,2,/long), stepId='variable', group='/a', name='long',   dimensions='x,y,z', debug=debug, verbose=verbose)
      rc = putdata( indgen(3,2,2,/l64),  stepId='variable', group='/a', name='long64', dimensions='x,y,z', debug=debug, verbose=verbose)
      rc = putdata( indgen(3,2,2,/uint), stepId='variable', group='/a', name='ushort', dimensions='x,y,z', debug=debug, verbose=verbose)
      rc = putdata( indgen(3,2,2,/ulong),stepId='variable', group='/a', name='ulong',  dimensions='x,y,z', debug=debug, verbose=verbose)
      rc = putdata( indgen(3,2,2,/ul64), stepId='variable', group='/a', name='ulong64',dimensions='x,y,z', debug=debug, verbose=verbose)
      rc = putdata( cindgen(3,2,2),      stepId='variable', group='/a', name='complex',  dimensions='x,y,z', debug=debug, verbose=verbose)
      rc = putdata( dcindgen(3,2,2),     stepId='variable', group='/a', name='dcomplex', dimensions='x,y,z', debug=debug, verbose=verbose)
		    
      rc = putdata( findgen(3,2,2),      stepId='variable', group='/a', name='float_1',  dimensions='x3,y,z', debug=debug, verbose=verbose)      
      rc = putdata( dindgen(3,2,2),      stepId='variable', group='/a', name='double_1', dimensions='x3,y,z', debug=debug, verbose=verbose)
      rc = putdata( indgen(3,2,2,/byte), stepId='variable', group='/a', name='byte_1',   dimensions='x3,y,z', debug=debug, verbose=verbose)
      rc = putdata( indgen(3,2,2),       stepId='variable', group='/a', name='short_1',  dimensions='x3,y,z', debug=debug, verbose=verbose)
      rc = putdata( indgen(3,2,2,/long), stepId='variable', group='/a', name='long_1',   dimensions='x3,y,z', debug=debug, verbose=verbose)
      rc = putdata( indgen(3,2,2,/l64),  stepId='variable', group='/a', name='long64_1', dimensions='x3,y,z', debug=debug, verbose=verbose)
      rc = putdata( indgen(3,2,2,/uint), stepId='variable', group='/a', name='ushort_1', dimensions='x3,y,z', debug=debug, verbose=verbose)
      rc = putdata( indgen(3,2,2,/ulong),stepId='variable', group='/a', name='ulong_1',  dimensions='x3,y,z', debug=debug, verbose=verbose)
      rc = putdata( indgen(3,2,2,/ul64), stepId='variable', group='/a', name='ulong64_1',dimensions='x3,y,z', debug=debug, verbose=verbose)
      rc = putdata( cindgen(3,2,2),      stepId='variable', group='/a', name='complex_1',  dimensions='x3,y,z', debug=debug, verbose=verbose)
      rc = putdata( dcindgen(3,2,2),     stepId='variable', group='/a', name='dcomplex_1', dimensions='x3,y,z', debug=debug, verbose=verbose)
		    
      rc = putdata( findgen(3,2,2),      stepId='variable', group='/a', name='float_2',  dimensions='x,y2,z', debug=debug, verbose=verbose)      
      rc = putdata( dindgen(3,2,2),      stepId='variable', group='/a', name='double_2', dimensions='x,y2,z', debug=debug, verbose=verbose)
      rc = putdata( indgen(3,2,2,/byte), stepId='variable', group='/a', name='byte_2',   dimensions='x,y2,z', debug=debug, verbose=verbose)
      rc = putdata( indgen(3,2,2),       stepId='variable', group='/a', name='short_2',  dimensions='x,y2,z', debug=debug, verbose=verbose)
      rc = putdata( indgen(3,2,2,/long), stepId='variable', group='/a', name='long_2',   dimensions='x,y2,z', debug=debug, verbose=verbose)
      rc = putdata( indgen(3,2,2,/l64),  stepId='variable', group='/a', name='long64_2', dimensions='x,y2,z', debug=debug, verbose=verbose)
      rc = putdata( indgen(3,2,2,/uint), stepId='variable', group='/a', name='ushort_2', dimensions='x,y2,z', debug=debug, verbose=verbose)
      rc = putdata( indgen(3,2,2,/ulong),stepId='variable', group='/a', name='ulong_2',  dimensions='x,y2,z', debug=debug, verbose=verbose)
      rc = putdata( indgen(3,2,2,/ul64), stepId='variable', group='/a', name='ulong64_2',dimensions='x,y2,z', debug=debug, verbose=verbose)
      rc = putdata( cindgen(3,2,2),      stepId='variable', group='/a', name='complex_2',  dimensions='x,y2,z', debug=debug, verbose=verbose)
      rc = putdata( dcindgen(3,2,2),     stepId='variable', group='/a', name='dcomplex_2', dimensions='x,y2,z', debug=debug, verbose=verbose)
		    
      rc = putdata( findgen(3,2,2),      stepId='variable', group='/a', name='float_3',  dimensions='x,y,z2', debug=debug, verbose=verbose)      
      rc = putdata( dindgen(3,2,2),      stepId='variable', group='/a', name='double_3', dimensions='x,y,z2', debug=debug, verbose=verbose)
      rc = putdata( indgen(3,2,2,/byte), stepId='variable', group='/a', name='byte_3',   dimensions='x,y,z2', debug=debug, verbose=verbose)
      rc = putdata( indgen(3,2,2),       stepId='variable', group='/a', name='short_3',  dimensions='x,y,z2', debug=debug, verbose=verbose)
      rc = putdata( indgen(3,2,2,/long), stepId='variable', group='/a', name='long_3',   dimensions='x,y,z2', debug=debug, verbose=verbose)
      rc = putdata( indgen(3,2,2,/l64),  stepId='variable', group='/a', name='long64_3', dimensions='x,y,z2', debug=debug, verbose=verbose)
      rc = putdata( indgen(3,2,2,/uint), stepId='variable', group='/a', name='ushort_3', dimensions='x,y,z2', debug=debug, verbose=verbose)
      rc = putdata( indgen(3,2,2,/ulong),stepId='variable', group='/a', name='ulong_3',  dimensions='x,y,z2', debug=debug, verbose=verbose)
      rc = putdata( indgen(3,2,2,/ul64), stepId='variable', group='/a', name='ulong64_3',dimensions='x,y,z2', debug=debug, verbose=verbose)
      rc = putdata( cindgen(3,2,2),      stepId='variable', group='/a', name='complex_3',  dimensions='x,y,z2', debug=debug, verbose=verbose)
      rc = putdata( dcindgen(3,2,2),     stepId='variable', group='/a', name='dcomplex_3', dimensions='x,y,z2', debug=debug, verbose=verbose)

		    
      rc = putdata( findgen(3,2,2),      stepId='variable', group='/a', name='float_4',  dimensions='x3,y2,z', debug=debug, verbose=verbose)      
      rc = putdata( dindgen(3,2,2),      stepId='variable', group='/a', name='double_4', dimensions='x3,y2,z', debug=debug, verbose=verbose)
      rc = putdata( indgen(3,2,2,/byte), stepId='variable', group='/a', name='byte_4',   dimensions='x3,y2,z', debug=debug, verbose=verbose)
      rc = putdata( indgen(3,2,2),       stepId='variable', group='/a', name='short_4',  dimensions='x3,y2,z', debug=debug, verbose=verbose)
      rc = putdata( indgen(3,2,2,/long), stepId='variable', group='/a', name='long_4',   dimensions='x3,y2,z', debug=debug, verbose=verbose)
      rc = putdata( indgen(3,2,2,/l64),  stepId='variable', group='/a', name='long64_4', dimensions='x3,y2,z', debug=debug, verbose=verbose)
      rc = putdata( indgen(3,2,2,/uint), stepId='variable', group='/a', name='ushort_4', dimensions='x3,y2,z', debug=debug, verbose=verbose)
      rc = putdata( indgen(3,2,2,/ulong),stepId='variable', group='/a', name='ulong_4',  dimensions='x3,y2,z', debug=debug, verbose=verbose)
      rc = putdata( indgen(3,2,2,/ul64), stepId='variable', group='/a', name='ulong64_4',dimensions='x3,y2,z', debug=debug, verbose=verbose)
      rc = putdata( cindgen(3,2,2),      stepId='variable', group='/a', name='complex_4',  dimensions='x3,y2,z', debug=debug, verbose=verbose)
      rc = putdata( dcindgen(3,2,2),     stepId='variable', group='/a', name='dcomplex_4', dimensions='x3,y2,z', debug=debug, verbose=verbose)
		    
      rc = putdata( findgen(3,2,2),      stepId='variable', group='/a', name='float_5',  dimensions='x,y2,z2', debug=debug, verbose=verbose)      
      rc = putdata( dindgen(3,2,2),      stepId='variable', group='/a', name='double_5', dimensions='x,y2,z2', debug=debug, verbose=verbose)
      rc = putdata( indgen(3,2,2,/byte), stepId='variable', group='/a', name='byte_5',   dimensions='x,y2,z2', debug=debug, verbose=verbose)
      rc = putdata( indgen(3,2,2),       stepId='variable', group='/a', name='short_5',  dimensions='x,y2,z2', debug=debug, verbose=verbose)
      rc = putdata( indgen(3,2,2,/long), stepId='variable', group='/a', name='long_5',   dimensions='x,y2,z2', debug=debug, verbose=verbose)
      rc = putdata( indgen(3,2,2,/l64),  stepId='variable', group='/a', name='long64_5', dimensions='x,y2,z2', debug=debug, verbose=verbose)
      rc = putdata( indgen(3,2,2,/uint), stepId='variable', group='/a', name='ushort_5', dimensions='x,y2,z2', debug=debug, verbose=verbose)
      rc = putdata( indgen(3,2,2,/ulong),stepId='variable', group='/a', name='ulong_5',  dimensions='x,y2,z2', debug=debug, verbose=verbose)
      rc = putdata( indgen(3,2,2,/ul64), stepId='variable', group='/a', name='ulong64_5',dimensions='x,y2,z2', debug=debug, verbose=verbose)
      rc = putdata( cindgen(3,2,2),      stepId='variable', group='/a', name='complex_5',  dimensions='x,y2,z2', debug=debug, verbose=verbose)
      rc = putdata( dcindgen(3,2,2),     stepId='variable', group='/a', name='dcomplex_5', dimensions='x,y2,z2', debug=debug, verbose=verbose)
		    
      rc = putdata( findgen(3,2,2),      stepId='variable', group='/a', name='float_6',  dimensions='x3,y,z2', debug=debug, verbose=verbose)      
      rc = putdata( dindgen(3,2,2),      stepId='variable', group='/a', name='double_6', dimensions='x3,y,z2', debug=debug, verbose=verbose)
      rc = putdata( indgen(3,2,2,/byte), stepId='variable', group='/a', name='byte_6',   dimensions='x3,y,z2', debug=debug, verbose=verbose)
      rc = putdata( indgen(3,2,2),       stepId='variable', group='/a', name='short_6',  dimensions='x3,y,z2', debug=debug, verbose=verbose)
      rc = putdata( indgen(3,2,2,/long), stepId='variable', group='/a', name='long_6',   dimensions='x3,y,z2', debug=debug, verbose=verbose)
      rc = putdata( indgen(3,2,2,/l64),  stepId='variable', group='/a', name='long64_6', dimensions='x3,y,z2', debug=debug, verbose=verbose)
      rc = putdata( indgen(3,2,2,/uint), stepId='variable', group='/a', name='ushort_6', dimensions='x3,y,z2', debug=debug, verbose=verbose)
      rc = putdata( indgen(3,2,2,/ulong),stepId='variable', group='/a', name='ulong_6',  dimensions='x3,y,z2', debug=debug, verbose=verbose)
      rc = putdata( indgen(3,2,2,/ul64), stepId='variable', group='/a', name='ulong64_6',dimensions='x3,y,z2', debug=debug, verbose=verbose)
      rc = putdata( cindgen(3,2,2),      stepId='variable', group='/a', name='complex_6',  dimensions='x3,y,z2', debug=debug, verbose=verbose)
      rc = putdata( dcindgen(3,2,2),     stepId='variable', group='/a', name='dcomplex_6', dimensions='x3,y,z2', debug=debug, verbose=verbose)
      
; Close the File 
 
      rc = putdata(  stepId='close', debug=debug, verbose=verbose) 
      
      return     

   endif   
   
;-----------------------------------------------------------------------------------------
; #20 Add Measurement Data to multiple open files

   if(test eq 20) then begin

$rm test20*.nc

      fileIdA=0L
      fileIdB=0L
      fileIdC=0L
      fileIdD=0L
      
      rc = putdata( 'test20A.nc', stepId='create', title='Test #20A', class='raw data', conventions='Fusion-1.0', fileId=fileIdA, debug=debug, verbose=verbose)
      rc = putdata( 'test20B.nc', stepId='create', title='Test #20B', class='raw data', conventions='Fusion-1.0', fileId=fileIdB, debug=debug, verbose=verbose)
      rc = putdata( 'test20C.nc', stepId='create', title='Test #20C', class='raw data', conventions='Fusion-1.0', fileId=fileIdC, debug=debug, verbose=verbose)
      rc = putdata( 'test20D.nc', stepId='create', title='Test #20D', class='raw data', conventions='Fusion-1.0', fileId=fileIdD, debug=debug, verbose=verbose)
		    
      rc = putdata( 3, stepId='dimension', group='/a', name='x3', fileId=fileIdC, debug=debug, verbose=verbose)               
      rc = putdata( 2, stepId='dimension', group='/a', name='x2', fileId=fileIdB, debug=debug, verbose=verbose)               
      rc = putdata( 4, stepId='dimension', group='/a', name='x4', fileId=fileIdD, debug=debug, verbose=verbose)               
      rc = putdata( 1, stepId='dimension', group='/a', name='x1', fileId=fileIdA, debug=debug, verbose=verbose)               

      rc = putdata( FINDGEN(3), stepId='coordinate', group='/a', name='x3',  fileId=fileIdC, debug=debug, verbose=verbose)   
      rc = putdata( FINDGEN(2), stepId='coordinate', group='/a', name='x2',  fileId=fileIdB, debug=debug, verbose=verbose)   
      rc = putdata( FINDGEN(4), stepId='coordinate', group='/a', name='x4',  fileId=fileIdD, debug=debug, verbose=verbose)   
      rc = putdata( FINDGEN(1), stepId='coordinate', group='/a', name='x1',  fileId=fileIdA, debug=debug, verbose=verbose)   
     
      rc = putdata( FINDGEN(3)*!PI, stepId='variable', group='/a', name='xx3', dimensions='x3', fileId=fileIdC, debug=debug, verbose=verbose)
      rc = putdata( FINDGEN(2)*!PI, stepId='variable', group='/a', name='xx2', dimensions='x2', fileId=fileIdB, debug=debug, verbose=verbose)
      rc = putdata( FINDGEN(4)*!PI, stepId='variable', group='/a', name='xx4', dimensions='x4', fileId=fileIdD, debug=debug, verbose=verbose)
      rc = putdata( FINDGEN(1)*!PI, stepId='variable', group='/a', name='xx1', dimensions='x1', fileId=fileIdA, debug=debug, verbose=verbose)
 
; Close the File 
 
      rc = putdata(  stepId='close', fileId=fileIdC, debug=debug, verbose=verbose) 
      rc = putdata(  stepId='close', fileId=fileIdB, debug=debug, verbose=verbose) 
      rc = putdata(  stepId='close', fileId=fileIdD, debug=debug, verbose=verbose) 
      rc = putdata(  stepId='close', fileId=fileIdA, debug=debug, verbose=verbose) 
      
      return     

   endif   
   
;-----------------------------------------------------------------------------------------
; #21 Measurement Data with Errors: Rank 2 arrays of fixed length

   if(test eq 21) then begin

$rm test21.nc

      fileId=0L
      
      rc = putdata( 'test21.nc', stepId='create', title='Test #21', class='raw data', conventions='Fusion-1.0', fileId=fileId, debug=debug, verbose=verbose)
		    
      rc = putdata( 3, stepId='dimension', group='/a', name='x', debug=debug, verbose=verbose)               
      rc = putdata( 2, stepId='dimension', group='/a', name='y', debug=debug, verbose=verbose)               
      rc = putdata( [1.0,2.0,3.0], stepId='coordinate', group='/a', name='x',  debug=debug, verbose=verbose)   
      rc = putdata( [4.0,5.0],     stepId='coordinate', group='/a', name='y',  debug=debug, verbose=verbose)   

      rc = putdata( dindgen(3,2),      stepId='variable', group='/a', name='edouble', dimensions='x,y', debug=debug, verbose=verbose)
      rc = putdata( indgen(3,2,/byte), stepId='variable', group='/a', name='ebyte',   dimensions='x,y', debug=debug, verbose=verbose)
      rc = putdata( indgen(3,2),       stepId='variable', group='/a', name='eshort',  dimensions='x,y', debug=debug, verbose=verbose)
      rc = putdata( indgen(3,2,/long), stepId='variable', group='/a', name='elong',   dimensions='x,y', debug=debug, verbose=verbose)
      rc = putdata( indgen(3,2,/l64),  stepId='variable', group='/a', name='elong64', dimensions='x,y', debug=debug, verbose=verbose)
      rc = putdata( indgen(3,2,/uint), stepId='variable', group='/a', name='eushort', dimensions='x,y', debug=debug, verbose=verbose)
      rc = putdata( indgen(3,2,/ulong),stepId='variable', group='/a', name='eulong',  dimensions='x,y', debug=debug, verbose=verbose)
      rc = putdata( indgen(3,2,/ul64), stepId='variable', group='/a', name='eulong64',dimensions='x,y', debug=debug, verbose=verbose)
      rc = putdata( cindgen(3,2),      stepId='variable', group='/a', name='ecomplex',  dimensions='x,y', debug=debug, verbose=verbose)
      rc = putdata( dcindgen(3,2),     stepId='variable', group='/a', name='edcomplex', dimensions='x,y', debug=debug, verbose=verbose)

      rc = putdata( dindgen(3,2),      stepId='variable', group='/a', name='double',  error='edouble',   dimensions='x,y', debug=debug, verbose=verbose)
      rc = putdata( indgen(3,2,/byte), stepId='variable', group='/a', name='byte',    error='ebyte',     dimensions='x,y', debug=debug, verbose=verbose)
      rc = putdata( indgen(3,2),       stepId='variable', group='/a', name='short',   error='eshort',    dimensions='x,y', debug=debug, verbose=verbose)
      rc = putdata( indgen(3,2,/long), stepId='variable', group='/a', name='long',    error='elong',     dimensions='x,y', debug=debug, verbose=verbose)
      rc = putdata( indgen(3,2,/l64),  stepId='variable', group='/a', name='long64',  error='elong64',   dimensions='x,y', debug=debug, verbose=verbose)
      rc = putdata( indgen(3,2,/uint), stepId='variable', group='/a', name='ushort',  error='eushort',   dimensions='x,y', debug=debug, verbose=verbose)
      rc = putdata( indgen(3,2,/ulong),stepId='variable', group='/a', name='ulong',   error='eulong',    dimensions='x,y', debug=debug, verbose=verbose)
      rc = putdata( indgen(3,2,/ul64), stepId='variable', group='/a', name='ulong64', error='eulong64',  dimensions='x,y', debug=debug, verbose=verbose)
      rc = putdata( cindgen(3,2),      stepId='variable', group='/a', name='complex', error='ecomplex',  dimensions='x,y', debug=debug, verbose=verbose)
      rc = putdata( dcindgen(3,2),     stepId='variable', group='/a', name='dcomplex',error='edcomplex', dimensions='x,y', debug=debug, verbose=verbose)
      
; Close the File 
 
      rc = putdata( stepId='close', debug=debug, verbose=verbose) 
      
      return     

   endif             
            		           
   return   
end
