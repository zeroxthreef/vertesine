# Vertesine Web Files

All files for the vertesine web backend and frontend. Vertesine is my personal website that I'm currently completely remaking.

It has a templating system that I plan on reusing for more of my web projects so this is also where i'd reuse the system.

This is not finished whatsoever. I still have to credit libraries and people I took code from (especially for snowflake generation). All coming soon. This is just a quick push.

## Building

```
mkdir build && cd build
cmake ..
make -j4
```

## Prerequisites

* jansson
* libsodium (do not get it from apt. Use the newest repo version)
* libcurl
* libgd
* libz
* libb64
* hiredis
* discount(https://www.pell.portland.or.us/~orc/Code/discount/)

## TODO

* big one: move from snowbird to facil.io
* too many things to list. Just refer to the github issues /shrug

## License

* MIT
