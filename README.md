# librestd

A low dependencies and self contained library to create C++ RESTful API services ( **UNDER HEAVY DEVELOPMENT** ).

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
