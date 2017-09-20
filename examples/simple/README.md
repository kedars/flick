Simple program demonstrating GET and POST handlers.

* Build program as:
```
 $ cd flick
 $ make EXAMPLE=examples/simple
```
* Run the program as:
```
 $ sudo ./simple
```
* Test the program
```
$ curl http://127.0.0.1/hello
Hello World!
$

$ curl -d 'Howdy' http://127.0.0.1/echo
Howdy

$curl --verbose -d 'Howdy' http://127.0.0.1/echo
*   Trying 127.0.0.1...
* TCP_NODELAY set
* Connected to 127.0.0.1 (127.0.0.1) port 80 (#0)
> POST /echo HTTP/1.1
> Host: 127.0.0.1
> User-Agent: curl/7.54.0
> Accept: */*
> Content-Length: 5
> Content-Type: application/x-www-form-urlencoded
>
* upload completely sent off: 5 out of 5 bytes
< HTTP/1.1 200 OK
< Content-Type: application/json
< Content-Length: 5
<
* Connection #0 to host 127.0.0.1 left intact
Howdy
```
