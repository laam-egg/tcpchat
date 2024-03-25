# tcpchat: Simple chat programs using low-level TCP sockets

## Demo

<https://youtu.be/LQhqlUPW-3I>

## Introduction

This is a set of very basic chat programs
in the form of client/server communication
using low-level TCP sockets.

They are written in pure C (no C++ involved),
and leverage the socket API of the Linux
operating system.

This is not at all meant for end users and
production use, but rather programming
hobbyists who want to dive deep into
low-level network programming, as well as
Linux programming, in C.

## Features

- **Platforms:** Linux distros, and maybe
   MacOS as well.
- **Multi-user:** Multiple users can
   connect to the same server and see
   each other's messages.
- **Unicode-aware:** You should be able to
   enter your name and messages in any
   language!
- **Messages are sent over the network in**
   **plain text, no SSL, no encryption:**
   this means your messages could be seen
   by anyone in the same network with a
   decent network packet spying tool.
   Anyway, this is just a toy project!

## Setup

For Ubuntu:

1. Install the necessary C build tools
    and the `ncurses` library:

    ```sh
    sudo apt install build-essential
    sudo apt install libncurses5-dev libncursesw5-dev
    ```

2. To compile the SERVER program, run:

    ```sh
    gcc -o server server.c protocol.c lklist.c
    ```

3. To compile the CLIENT program, run:

    ```sh
    gcc -o client client.c protocol.c -lncursesw
    ```

## Run the Programs

First, run the server. It is hardcoded
to listen on port 12345.

```sh
./server
```

Then, run the client program:

```sh
./client
```

If the client is run on the same computer
as the server, then in the SERVER IP field,
enter `127.0.0.1`. Otherwise, run `ifconfig`
on the server's computer to know the server's
IP address.

Fill in the SERVER PORT field the listening
port as printed to the console by the server
program (e.g. `12345`).

Finally, enter your own name, and you are
good to go !

## License

Copyright (C) 2024 Vũ Tùng Lâm.
Distributed under [the MIT license](./LICENSE.txt).
