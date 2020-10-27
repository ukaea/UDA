function getgeomdata, workname, worksource, _extra=_extra

  errmsg = 'MAST-U GEOMETRY PLUGIN IS NOT INSTALLED'

  print, 'GETGEOMDATA: '+errmsg

  return, {erc: -1, errmsg: errmsg}

end
