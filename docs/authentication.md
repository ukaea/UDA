---
layout: default
title: Authentication
nav_order: 6
---

TODO: Notes on configuring authenticated connections

# Authenticated connections in UDA
{:.no_toc}

An UDA server can be configured to only accept requests from authenticated clients. This can be used to ensure only recognised users are able to access the data available through that server. 

Currently only X509 certificate-based authentication is implemented. This involves both the server and client having access to their own certificate bundles, including the SSL Certificate Authority (CA) certificate used to sign the server and client certificates. 

Certificate revocations can be managed using a certificate revocation list (CRL) on the server. 

## Contents
{:.no_toc}
1. TOC 
{:toc}

## Configuring an SSL-authenticated server

```sh
export UDA_SERVER_SSL_AUTHENTICATE=1
export UDA_SERVER_SSL_CERT="${UDA_ROOT}/etc/.uda/certs/<server_address>.pem"
export UDA_SERVER_SSL_KEY="${UDA_ROOT}/etc/.uda/keys/<server_address>.key.pem"
export UDA_SERVER_CA_SSL_CERT="${UDA_ROOT}/etc/.uda/certs/uda-ca.cert.pem"
export UDA_SERVER_CA_SSL_CRL="${UDA_ROOT}/etc/.uda/crl/uda-ca.crl.pem"
```

## Configuring an authenticated client connection

```sh
SSL_HOME="<certificate_install_dir>/.uda"

export UDA_CLIENT_SSL_KEY="${SSL_HOME}/keys/<username>.key.pem"
export UDA_CLIENT_CA_SSL_CERT="${SSL_HOME}/certs/uda-ca.cert.pem"
export UDA_CLIENT_SSL_CERT="${SSL_HOME}/certs/<username>.cert.pem"
export UDA_CLIENT_SSL_AUTHENTICATE=1

```

