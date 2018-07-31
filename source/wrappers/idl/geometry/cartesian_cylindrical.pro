pro cartesian_cylindrical, x, y, z, r=r, phi=phi
  r = sqrt(x*x + y*y)

  if abs(x) gt 0 then phi = atan(y/x) $
  else phi = 0.0D

  if x gt 0.0 and y lt 0.0 then phi = 2*!PI + phi $
  else if x lt 0.0 and y lt 0.0 then phi = !PI + phi $
  else if x lt 0.0 and y gt 0.0 then phi = !PI + phi

  ; convert to degrees
  phi = phi * 180.0 / !PI

end
