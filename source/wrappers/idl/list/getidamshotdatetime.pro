; ----------------------------
; IDL wrapper to mimic the old getidamshotdatetime functions
; that were part of roIdam. Instead now uses the meta
; plugin.
;
; L. Kogan
;
; March 2018
; ----------------------------

function getidamshotdatetime, shot=shot, pulno=pulno, exp_number=exp_number

  ; shot number
  if exists(pulno) then shot = pulno
  if exists(exp_number) then shot = exp_number

  if undefined(shot) then begin
     print, 'getidamshotdatetime >> SHOT NUMBER IS REQUIRED: datetime = getidamshotdatetime(shot=<shotno>)'
     return, -1
  endif

  meta_comm = 'meta::get(context=data,shot='+strtrim(shot, 2)+',/shotdatetime)'

  ; Call
  datetimestruct = getdata(meta_comm, '')

  if datetimestruct.erc ne 0 then begin
     return, -1
  endif

  datetimestruct = datetimestruct.data

  ; Turn structure into getidamshotdatetime  structure
  datetime = { EXP_NUMBER: 0L,    $
               DATE: '',          $
               TIME: '' }

  datetime.exp_number = datetimestruct.shot
  datetime.date = datetimestruct.date
  datetime.time = datetimestruct.time

  return, datetime
end
