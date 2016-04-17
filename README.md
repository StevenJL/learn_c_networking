# Learning C Network Programming

Learn C network programming with these examples.

# Examples

## Wuts My Ip
### Usage

On host

```
gcc wuts_my_ip.c -o server
./server
```

On remote client (that's on the same network as the host). Connect to the server
on the host using `telnet` and the host's private ip address. The 9666 port is
hardcoded in the code example.

```
telnet 192.168.128.xxx 9666 
> Listen to John Coltrane... By the way, your ip address is: 192.168.128.237
```

#### How It Works
Learn how this works by reading the [prodigiously documented source code](https://github.com/StevenJL/learn_c_networking/blob/master/wuts_my_ip.c)

## Minimal HTTP Web Server
### Usage

On host

```
gcc minimal_web_server.c -o server
sudo ./server
```

Now visit 127.0.0.1 on the browser and you should see this [simple html page](https://github.com/StevenJL/learn_c_networking/blob/master/mws_root/index.html) returned by the browser.

#### How It Works
Learn how this works by reading the [prodigiously documented source code](https://github.com/StevenJL/learn_c_networking/blob/master/minimal_web_server.c)

# References

Hacking by Jon Erickson: http://www.amazon.com/Hacking-Art-Exploitation-Jon-Erickson/dp/1593271441

Beej's intro to network programming: http://beej.us/guide/bgnet/output/html/multipage/index.html

opengroup.org socket.h documentation: http://pubs.opengroup.org/onlinepubs/009695399/basedefs/sys/socket.h.html

opengroup.org arpa/inet.h documentation: http://pubs.opengroup.org/onlinepubs/009695399/basedefs/arpa/inet.h.html

opengroup.org net/inet.h documentation: http://pubs.opengroup.org/onlinepubs/009695399/basedefs/arpa/inet.h.html

