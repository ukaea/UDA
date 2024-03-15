#!/bin/bash

# Root CA

openssl genrsa -out rootCA.key 4096
openssl req -x509 -new -nodes -key rootCA.key -sha256 -days 1024 -subj "/C=GB/O=UKAEA/OU=CD/CN=UDA" -out rootCA.crt

# Server certificate

openssl genrsa -out server.key 4096
openssl req -new -key server.key -subj "/C=GB/O=UKAEA/OU=CD/CN=localhost" -out server.csr
openssl x509 -req -in server.csr -CA rootCA.crt -CAkey rootCA.key -CAcreateserial -out server.crt

# Client certificate

openssl genrsa -out client.key 4096
openssl req -new -key client.key -subj "/C=GB/O=UKAEA/OU=CD/CN=jholloc" -out client.csr
openssl x509 -req -in client.csr -CA rootCA.crt -CAkey rootCA.key -CAcreateserial -out client.crt