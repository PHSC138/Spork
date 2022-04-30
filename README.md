# CS 525 Final project: Spork
- Ryan Hennessee

## Making cURLpp
- Curlpp is a required library for this project, it is thread safe which makes it ideal for use in a parallel program.
- This library is a C++ wrapper for libcurl 
- cURLpp has been added as a submodule for convinience, it can be found here [https://github.com/jpbarrette/curlpp/tree/v0.8.1](https://github.com/jpbarrette/curlpp/tree/v0.8.1)
- `cd curlpp`
    - Navigate to curlpp directory
- `mkdir build`
    - Create build directory
- `cd build`
    - Enter build directory
- `cmake ../`
    - Cmake with the CMakeLists.txt in the parent directory
- `make`
    - Make the shared libraries
- `cp libcurlpp.so* ../../`
    - Copy them to Spork top level

## To compile spork
- Please follow the steps above to build cURLpp and put the `.so` files into the Spork directory
- `mkdir build`
    - Make a build directory for Spork
- `cmake ../`
    - Cmake
- `make`

## To run
- An example `payload_file.json` is provided, it will query `127.0.0.1:4000` with the 500 worst passwords stored in `500-worst.txt`.
- To see the requests start the provided `echo_server.py` python server `./echo_server 4000` in another window
- Run using `./spork ../payload_file.json > out.txt 2>&1`

## More info on the structure of the payload_file
- The following options are currently supported for the payload file:
```json
{
    "threads": 8,
    "address": "127.0.0.1:4000/login",
    "protocol": "http",
    "protocol_options": {
        "http_method": "get / post",
        "http_data": {
            "username": "$0",
            "password": "$1",
            "extra": "any static value"
            ...
        }
    },
    "payload": [ // list of data
        "../usernames.txt",
        "../500-worst.txt", // path to data source (separated by newlines)
        ...
    ]
}
```

## Future plans
- Support raw with the following format
```json
{
    "threads": 8,
    "address": "127.0.0.1:4000/login",
    "protocol": "http / raw",
    "protocol_options": {
        "http_method": "get / post / etc",
        "http_data": {
            "username": "$0",
            "password": "$1",
            ...
        },
        "raw_protocol": "tcp / udp",
        "raw_data": [
            "Prefix",
            "$0",
            ...
            "postfix"
        ]
    },
    "payload": [ // list of data
        "../500-worst.txt", // path to data source (separated by newlines)
        ...
    ]
}
```