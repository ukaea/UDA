function listgeomgroups, shot=shot, pulno=pulno, exp_number=exp_number, all=all

  ; basic command (this is all you need for all keyword)
  comm = 'GEOM::listGeomGroups()'

  ; shot number
  if exists(pulno) then shot = pulno
  if exists(exp_number) then shot = exp_number

  if undefined(shot) then shot = ''

  grouplist = getstruct(comm, shot)

  if grouplist.erc ne 0 then begin
     print, grouplist.errmsg
     return, -1
  endif

  return, grouplist.data

end
