function listgeomsignals, shot=shot, pulno=pulno, exp_number=exp_number, $
                     group=group, version=version, all=all

  ; basic command (this is all you need for all keyword)
  comm = 'GEOM::listGeomSignals('

  ; shot number
  if exists(pulno) then shot = pulno
  if exists(exp_number) then shot = exp_number

  if undefined(shot) then shot = ''

  ; group
  if exists(group) then comm = comm + ', geomgroup='+strtrim(group)

  ; version
  if exists(version) then comm = _comm + ', version='+strtrim(version)

  comm = comm + ')'

  signallist = getstruct(comm, shot)

  if signallist.erc ne 0 then begin
     print, signallist.errmsg
     return, -1
  endif

  return, signallist.data

end
