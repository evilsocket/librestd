# librestd

A low dependencies and self contained library to create C++ RESTful API services ( **UNDER HEAVY DEVELOPMENT** ).

## Scope

This library and the underlying HTTP server component do **not** support the full HTTP specs but were rather implemented with speed and simplicity in mind. For instance, requests will be read up to 8192 bytes, after which threshold they will be dropped. This means features like file upload (to name one) are not yet supported at the current stage of development.

Nevertheless, as it is possible to see from the `hello_world.cpp` example file, the library offers every basic feature you might need in order to wrap your C++ tools/engines/business logic as REST services running on `localhost` ( or bound to a UNIX domain socket ).

## Compilation

    git clone https://github.com/evilsocket/librestd.git
    cd librestd
    cmake -DCMAKE_BUILD_TYPE=Release .
    make 
    
And install with:

    sudo make install

## Usage

For an usage example see the `hello_world.cpp` file.

## License

`librestd` is released under the GPL 3.0 license and it's copyleft of Simone 'evilsocket' Margaritelli.  

JSON support provided using the [JSON for Modern C++](https://github.com/nlohmann/json) library by Niels Lohmann.
