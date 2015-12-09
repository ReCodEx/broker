# TaskBroker

## How to run it

- Install ZeroMQ, version at least 4.0. Package name should be `libzmq3-dev`
- Install ZeroMQ C++ binding. Clone repository from [https://github.com/zeromq/cppzmq](https://github.com/zeromq/cppzmq) to same parent folder as this repo. Eq if you have this repo in path `/opt/src/basic-broker`, so `cppzmq` folder should be in the directory `/etc/src/`.
- Install CMake, GNU Make, G++ > 5
- Build as `cmake . && make`
- Run `./basic-broker`

