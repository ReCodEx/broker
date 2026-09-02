# Broker
[![Build Status](https://github.com/ReCodEx/broker/workflows/CI/badge.svg)](https://github.com/ReCodEx/broker/actions)
[![codecov](https://codecov.io/gh/ReCodEx/broker/branch/master/graph/badge.svg?token=PWV4OZ75DI)](https://codecov.io/gh/ReCodEx/broker)
[![License](http://img.shields.io/:license-mit-blue.svg)](http://badges.mit-license.org)
[![Docs](https://img.shields.io/badge/docs-latest-brightgreen.svg)](http://recodex.github.io/broker/)
[![Wiki](https://img.shields.io/badge/docs-wiki-orange.svg)](https://github.com/ReCodEx/wiki/wiki)
[![GitHub release](https://img.shields.io/github/release/recodex/broker.svg)](https://github.com/ReCodEx/wiki/wiki/Changelog)
[![COPR](https://copr.fedorainfracloud.org/coprs/semai/ReCodEx/package/recodex-broker/status_image/last_build.png)](https://copr.fedorainfracloud.org/coprs/semai/ReCodEx/)

The broker is a central part of the ReCodEx backend that directs most of the communication. It was designed to maintain a heavy load of messages by making only small actions in the main communication thread and asynchronous execution of other actions.

The responsibilities of the broker are:

- allowing workers to register themselves and keep track of their capabilities
- tracking status of each worker and handle cases when they crash
- accepting assignment evaluation requests from the frontend and forwarding them to workers
- receiving a job status information from workers and forward them to the frontend either via monitor or REST API
- notifying the frontend on errors of the backend

## Installation

### COPR Installation (recommended for RHEL-based systems)

```
# dnf install yum-plugin-copr
# dnf copr enable semai/ReCodEx
# dnf install recodex-broker
```

### Manual Installation from Git

The broker has similar basic dependencies as the worker. It is recommended to install them first:

- GCC compiler with C++ frontend, make, and cmake build tools
- ZeroMQ version at least 4.0, packages `zeromq` and `zeromq-devel` (`libzmq3-dev` on Debian)
- YAML-CPP library, `yaml-cpp` and `yaml-cpp-devel` (`libyaml-cpp0.5v5` and `libyaml-cpp-dev` on Debian)
- libcurl library, `libcurl-devel` (`libcurl4-gnutls-dev` on Debian)
- Boost library `boost` and `boost-devel` (`libboost-all-dev` on Debian)

Clone the broker source code repository including the submodules:
```
$ git clone https://github.com/ReCodEx/broker.git
$ git submodule update --init
```

Build the broker using CMake:
```
$ mkdir -p build && cd build && cmake .. && make
```

Create installation packages from it (root permissions are required). Note that `rpm` and `deb` packages are built at the same time. You may need to have `rpmbuild` command (usually installed as `rpmbuild` or `rpm` package) or you need to edit CPACK_GENERATOR variable in _CMakeLists.txt_ file in the root of source code tree.

```
$ sudo make package
```

The previous step should have created `recodex-broker-<version>.(deb|rpm)` files in the `build` directory. You may install them manually:
```
$ sudo dpkg -i recodex-broker-<version>.deb
```
or
```
$ sudo rpm -i recodex-broker-<version>.rpm
```
depending whether you are using Debian-based or RHEL-based system.

An alternative is to invoke `make install` instead of `make package` to install the broker directly.


### After Installation

Installation of the broker program performs the following steps on your computer:

- create config file `/etc/recodex/broker/config.yml`
- create _systemd_ unit file `/etc/systemd/system/recodex-broker.service`
- put main binary to `/usr/bin/recodex-broker`
- create system user and group `recodex` with nologin shell (if not existing)
- create log directory `/var/log/recodex`
- set ownership of config (`/etc/recodex`) and log (`/var/log/recodex`) directories to `recodex` user and group

The configuration in `/etc/recodex/broker/config.yml` should be properly updated before starting the broker service (configuration details are described in the next section). Most importantly, the `notifier` section must be filled to point to the core module (API) and its credentials must match the credentials of the core module.

Broker is managed by systemd. You can check the status of the broker service by running:
```
# systemctl start recodex-broker.service
# systemctl status recodex-broker.service
```

If you want to run the broker after system startup, enable it permanently:
```
# systemctl enable recodex-broker.service
```


## Configuration


The default location for the broker configuration file is
`/etc/recodex/broker/config.yml`.

### Configuration items

- _clients_ -- specifies the address and port to bind for clients (frontend instance)
	- _address_ -- hostname or an IP address as a string (`*` for any)
	- _port_ -- desired port
- _workers_ -- specifies the address and port to bind for workers
	- _address_ -- hostname or an IP address as a string (`*` for any)
	- _port_ -- desired port
	- *max_liveness* -- maximum amount of pings the worker can fail to send before it is considered disconnected
	- *max_request_failures* -- maximum number of times a job can fail (due to e.g., worker disconnect or a network error when downloading something from the fileserver) and be assigned again
- _monitor_ -- settings of the monitor service connection
	- _address_ -- IP address of running monitor service
	- _port_ -- desired port
- _notifier_ -- details of the connection that is used in case of errors and noteworthy states
	- _address_ -- address where frontend API runs
	- _port_ -- desired port
	- _username_ -- username which can be used for HTTP authentication (must match `broker` > `auth` in core-api module configuration)
	- _password_ -- password which can be used for HTTP authentication (must match `broker` > `auth` in core-api module configuration)
- _logger_ -- settings of the logging capabilities
	- _file_ -- path to the logging file with a name without a suffix. `/var/log/recodex/broker` item will produce `broker.log`, `broker.1.log`, ...
	- _level_ -- level of logging, one of `off`, `emerg`, `alert`, `critical`, `err`, `warn`, `notice`, `info` and `debug`
	- _max-size_ -- maximal size of log file before rotating
	- _rotations_ -- number of rotations kept
- *queue_manager* -- selection of the queue manager implementation responsible for assigning jobs to workers. Currently only `single` (the default) and `multi` queue managers are in production version. Single-queue manager has one queue and dispatches jobs on demand as workers become available. Multi-queue manager has a queue for every worker, jobs are assigned immediately and cannot be re-assigned unless failure occurs. I.e., `single` provides better load balancing, `multi` has lower dispatching overhead.

### Example config file

```yaml
# Address and port for clients (frontend)
clients:
    address: "*"
    port: 9658
# Address and port for workers
workers:
    address: "*"
    port: 9657
    max_liveness: 10
    max_request_failures: 3
monitor:
    address: "127.0.0.1"
    port: 7894
notifier:
    address: "127.0.0.1"
    port: 8080
    username: "rebroker"
    password: "someSecretPassword"
logger:
    file: "/var/log/recodex/broker"  # w/o suffix - actual names will be broker.log, broker.1.log, ...
    level: "debug"  # level of logging
    max-size: 1048576  # 1 MB; max size of file before log rotation
    rotations: 3  # number of rotations kept
queue_manager: "single"  # name of the manager that handles job dispatching among queues (single is the default)
```

## Documentation

Feel free to read the documentation on [our wiki](https://github.com/ReCodEx/wiki/wiki).
