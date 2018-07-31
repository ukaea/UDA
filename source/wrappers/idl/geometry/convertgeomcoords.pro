function convertloop, data, cartcoords=cartcoords, cylcoords=cylcoords

  forward_function convertloop

  tag_names_data = tag_names(data)

  if keyword_set(cartcoords) then begin
     ind_r = where(strmatch(tag_names_data, 'R') eq 1, count_r)
     ind_z = where(strmatch(tag_names_data, 'Z') eq 1, count_z)
     ind_phi = where(strmatch(tag_names_data, 'PHI') eq 1, count_phi)

     is_coord = ((count_r gt 0) and (count_z gt 0) and (count_phi gt 0))

  endif else begin
     ind_x = where(strmatch(tag_names_data, 'X') eq 1, count_x)
     ind_y = where(strmatch(tag_names_data, 'Y') eq 1, count_y)
     ind_z = where(strmatch(tag_names_data, 'Z') eq 1, count_z)

     is_coord = ((count_x gt 0) and (count_y gt 0) and (count_z gt 0))

  endelse


  if is_coord and keyword_set(cartcoords) then begin
     struct_array = []

     for j = 0, n_elements(data)-1 do begin

        x_coord = 0
        y_coord = 0
        cylindrical_cartesian, (data[j]).r, (data[j]).z, (data[j]).phi, x=x_coord, y=y_coord


        new_struct = {}
        for i = 0, n_elements(tag_names_data)-1 do begin
           if tag_names_data[i] ne 'R' and tag_names_data[i] ne 'PHI' then begin
              new_struct = create_struct(new_struct, tag_names_data[i], (data[j]).(i))
           endif
        endfor

        new_struct = create_struct(new_struct, 'X', x_coord) 
        new_struct = create_struct(new_struct, 'Y', y_coord) 

        struct_array = [struct_array, new_struct]
     endfor

     return, struct_array

  endif else if is_coord and keyword_set(cylcoords) then begin
     struct_array = []

     for j = 0, n_elements(data)-1 do begin
        r_coord = 0
        phi_coord = 0
        cartesian_cylindrical, (data[j]).x, (data[j]).y, (data[j]).z, r=r_coord, phi=phi_coord
        this_struct = {}

        for i = 0, n_elements(tag_names_data)-1 do begin
           if tag_names_data[i] ne 'X' and tag_names_data[i] ne 'Y' then begin
              this_struct = create_struct(this_struct, tag_names_data[i], (data[j]).(i))
           endif
        endfor

        this_struct = create_struct(this_struct, 'R', r_coord) 
        this_struct = create_struct(this_struct, 'PHI', phi_coord) 

        struct_array = [struct_array, this_struct]
     endfor

     return, struct_array

  endif else begin
     new_struct = {}
     for i = 0, n_elements(tag_names_data)-1 do begin
        if not_structure(data.(i)) then begin
           new_struct = create_struct(new_struct, tag_names_data[i], data.(i))
           continue
        endif
        new_struct = create_struct(new_struct, tag_names_data[i], convertloop(data.(i), cartcoords=cartcoords, cylcoords=cylcoords))
     endfor
  endelse

  return, new_struct
end

function convertgeomcoords, datastruct, signal, cartcoords=cartcoords, cylcoords=cylcoords
  
  newstruct = datastruct

  sigsconvert = []

  if strmatch(signal, '/magnetics/pickup*') or strmatch(signal, '/magnetics/mirnov*') $
     or strmatch(signal, 'b_*') or strmatch(signal, 'm_*') or strmatch(signal, '/bolo*') $
     or strmatch(signal, 'h_*') or strmatch(signal, 's_*') or strmatch(signal, '/magnetics/halo*') $
     or strmatch(signal, '/magnetics/saddlecoils*') then begin
    
     newstruct = convertloop(datastruct, cartcoords=cartcoords, cylcoords=cylcoords)

  endif else begin
     print, 'WARNING: Coordinate conversion not implemented for ', signal
  endelse

  return, newstruct

end
