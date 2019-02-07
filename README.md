# Vertesine Web Files

All files for the vertesine web backend and frontend. Vertesine is my personal website that I'm currently completely remaking.

It has a templating system that I plan on reusing for more of my web projects so this is also where i'd reuse the system.

## Status

This backend is not live. The in-use backend is a separate project at this time that will be replaced when this is ready.

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

## Using

* highlight.js (generated)
* snowflaked
* rxi's vec
* rxi's map
* rxi's ini
* rxi's log.c
* rxi's snowbird

## TODO

* big one: move from snowbird to facil.io
* too many things to list. Just refer to the github issues /shrug

## License

* MIT
