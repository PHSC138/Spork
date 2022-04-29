# CS 525 Final project
- Ryan Hennessee

## Installing curlpp
- Curlpp is a required library for this project, it is thread safe which makes it ideal for use in a parallel program.
- This library is a C++ wrapper for libcurl 
- curlpp can be found here [https://github.com/jpbarrette/curlpp/tree/v0.8.1](https://github.com/jpbarrette/curlpp/tree/v0.8.1)
- Navigate to the download / clone directory
- `mkdir build`
- `cd build`
- `cmake ../`
- `make`
- `sudo make install`


## To compile scaling-spork
- `cmake .`
- `make`

## To run
- An example `payload_file.json` is provided, it will query `127.0.0.1` with the sample passwords in the `top100common.txt`.
- To see the requests start a python http server `python -m http.server 5000`
- Run using `./TODO 8 0 payload_file.json`

## More info on the structure of the payload_file
- The following options are supported for the payload file:
```json
{
    "protocol": "http / raw",
    "protocol_options": {
        "http_method": "get / post / etc",
        "http_data": {
            "username": "$1",
            "password": "$2",
            ...
        },
        "raw_protocol": "tcp / udp",
        "raw_data": [
            "Prefix",
            "$1",
            "postfix"
        ]
    },
    "payload": [ // list of data
        "./rockyou.txt", // path to data source (separated by newlines)
    ]
}
```