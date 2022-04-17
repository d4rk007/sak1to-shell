# sak1to-shell
Multi-threaded, multi-os/platform (Linux/Windows) c2 server and Windows reverse TCP shell client both written in C.


Compiling reverse TCP shell client with i686-w64-mingw32-gcc:
```
i686-w64-mingw32-gcc sakito_revshell.c lib/windows/sakito_win_core.c -o sakito_revshell.exe -s -ffunction-sections -fdata-sections -fno-exceptions -fmerge-all-constants -static-libgcc -lws2_32 -D PORT=<port_number> -D HOST=\"<server_ipv4_address>\"
```

Compiling reverse TCP shell with cl.exe within Developer command prompt:
```
cl.exe sakito_revshell.c lib/windows/sakito_win_core.c /D PORT=<port_number> /D HOST=\"<server_ipv4_address>\"
```

Compiling Linux server with GCC:
```
gcc -pthread sakito_server.c lib/sakito_server_tools.c lib/linux/sakito_slin_utils.c -o sakito_server -D PORT=<port_number>
```

Compiling Windows server with cl.exe within Developer command prompt:
```
cl.exe sakito_server.c lib/windows/sakito_win_core.c lib/sakito_server_tools.c lib/windows/sakito_swin_utils.c /D PORT=<port_number>
```


Command list:

- list: list available connections.

- interact [id]: interact with client.

- download [filename]: download a file from the client's filesystem to the server's filesystem.

- upload [filename]: upload a file from the server's filesystem to the client's filesystem.

- background: background client.

- exit: terminate client or host.

- cd [dir]: change directory on client or host.

![alt text](https://www.wallpaperbetter.com/wallpaper/156/434/483/cherry-blossom-flowers-painting-pink-1080P-wallpaper-middle-size.jpg)

