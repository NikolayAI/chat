# chat

```
gcc -o server ./server.c
gcc -o client ./client.c

./server 127.0.0.1 8080
./client 127.0.0.1 8080 petya
./client 127.0.0.1 8080 sergey
```

For using epoll server:

```
gcc -o server ./server-epoll.c
gcc -o client ./client.c

./server 127.0.0.1 8080
./client 127.0.0.1 8080 petya
./client 127.0.0.1 8080 sergey
```