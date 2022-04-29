# Final project
- Ryan Hennessee

## To compile
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