## How to genereate server.crt and server.key

```sh
openssl genrsa -out server.key 2048
openssl req -new -key server.key -out server.csr
openssl x509 -req -days 365 -in server.csr -signkey server.key -out server.crt
openssl rsa -in server.key -check
openssl x509 -in server.crt -text -noout
```
