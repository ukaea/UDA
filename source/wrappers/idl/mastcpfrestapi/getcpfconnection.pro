function getcpfconnection, verbose=verbose

    cpf_host = getenv('CPF_HOST')
    if cpf_host eq '' then cpf_host='idam3.mast.ccfe.ac.uk'

    cpf_port = getenv('CPF_PORT')
    if cpf_port eq '' then cpf_port=8000

    url_obj = obj_new('IDLnetUrl')
    url_obj->setProperty, verbose=verbose
    url_obj->setProperty, url_scheme="http"
    url_obj->setProperty, url_host=cpf_host
    url_obj->setProperty, url_port=cpf_port

    return, url_obj

end