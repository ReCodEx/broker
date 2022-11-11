# Broker
[![Build Status](https://github.com/ReCodEx/broker/workflows/CI/badge.svg)](https://github.com/ReCodEx/broker/actions)
[![codecov](https://codecov.io/gh/ReCodEx/broker/branch/master/graph/badge.svg?token=PWV4OZ75DI)](https://codecov.io/gh/ReCodEx/broker)
[![License](http://img.shields.io/:license-mit-blue.svg)](http://badges.mit-license.org)
[![Docs](https://img.shields.io/badge/docs-latest-brightgreen.svg)](http://recodex.github.io/broker/)
[![Wiki](https://img.shields.io/badge/docs-wiki-orange.svg)](https://github.com/ReCodEx/wiki/wiki)
[![GitHub release](https://img.shields.io/github/release/recodex/broker.svg)](https://github.com/ReCodEx/wiki/wiki/Changelog)
[![COPR](https://copr.fedorainfracloud.org/coprs/semai/ReCodEx/package/recodex-broker/status_image/last_build.png)](https://copr.fedorainfracloud.org/coprs/semai/ReCodEx/)

The broker is a central part of the ReCodEx backend that directs most of the
communication. It was designed to maintain a heavy load of messages by making
only small actions in the main communication thread and asynchronous execution
of other actions.

The responsibilities of broker are:

- allowing workers to register themselves and keep track of their capabilities
- tracking status of each worker and handle cases when they crash
- accepting assignment evaluation requests from the frontend and forwarding them
  to workers
- receiving a job status information from workers and forward them to the
  frontend either via monitor or REST API
- notifying the frontend on errors of the backend

## Installation

### COPR Installation

Follows description for CentOS which will do all steps as described in _Manual Installation_.

```
# yum install yum-plugin-copr
# yum copr enable semai/ReCodEx
# yum install recodex-broker
```

### Manual Installation

#### Dependencies

Broker has similar basic dependencies as worker, for recapitulation:

- ZeroMQ in version at least 4.0, packages `zeromq` and `zeromq-devel`
  (`libzmq3-dev` on Debian)
- YAML-CPP library, `yaml-cpp` and `yaml-cpp-devel` (`libyaml-cpp0.5v5` and
  `libyaml-cpp-dev` on Debian)
- libcurl library `libcurl-devel` (`libcurl4-gnutls-dev` on Debian)

#### Clone broker source code repository

```
$ git clone https://github.com/ReCodEx/broker.git
$ git submodule update --init
```

#### Make Installation

Installation of broker program does following step to your computer:

- create config file `/etc/recodex/broker/config.yml`
- create _systemd_ unit file `/etc/systemd/system/recodex-broker.service`
- put main binary to `/usr/bin/recodex-broker`
- create system user and group `recodex` with nologin shell (if not existing)
- create log directory `/var/log/recodex`
- set ownership of config (`/etc/recodex`) and log (`/var/log/recodex`)
  directories to `recodex` user and group

It is supposed that your current working directory is that one with clonned
worker source codes.

- Prepare environment running `mkdir build && cd build`
- Build sources by `cmake ..` following by `make`
- Build binary package by `make package` (may require root permissions).  Note
  that `rpm` and `deb` packages are build in the same time. You may need to have
  `rpmbuild` command (usually as `rpmbuild` or `rpm` package) or edit
  CPACK_GENERATOR variable _CMakeLists.txt_ file in root of source code tree.
- Install generated package through your package manager (`yum`, `dnf`, `dpkg`).

_Note:_ If you do not want to generate binary packages, you can just install the
project with `make install` (as root). But installation through your
distribution's package manager is preferred way to keep your system clean and
manageable in long term horizon.

#### Usage

Running broker is very similar to the worker setup. There is also provided
systemd unit file for convenient usage. There is only one broker per whole
ReCodEx solution, so there is no need for systemd templates.

- Running broker can be done by following command:
```
# systemctl start recodex-broker.service
```
Check with
```
# systemctl status recodex-broker.service
```
if the broker is running. You should see "active (running)" message.

- Broker can be stopped or restarted accordingly using `systemctl stop` and
  `systemctl restart` commands.
- If you want to run broker after system startup, run:
```
# systemctl enable recodex-broker.service
```

For further information about using systemd please refer to systemd
documentation.

## Configuration


The default location for broker configuration file is
`/etc/recodex/broker/config.yml`.

### Configuration items

- _clients_ -- specifies address and port to bind for clients (frontend
  instance)
	- _address_ -- hostname or IP address as string (`*` for any)
	- _port_ -- desired port
- _workers_ -- specifies address and port to bind for workers
	- _address_ -- hostname or IP address as string (`*` for any)
	- _port_ -- desired port
	- _max_liveness_ -- maximum amount of pings the worker can fail to send
	  before it is considered disconnected
	- _max_request_failures_ -- maximum number of times a job can fail (due to
	  e.g. worker disconnect or a network error when downloading something from
	  the fileserver) and be assigned again 
- _monitor_ -- settings of monitor service connection
	- _address_ -- IP address of running monitor service
	- _port_ -- desired port
- _notifier_ -- details of connection which is used in case of errors and good
  to know states
	- _address_ -- address where frontend API runs
	- _port_ -- desired port
	- _username_ -- username which can be used for HTTP authentication
	- _password_ -- password which can be used for HTTP authentication
- _logger_ -- settings of logging capabilities
	- _file_ -- path to the logging file with name without suffix.
	  `/var/log/recodex/broker` item will produce `broker.log`, `broker.1.log`,
	  ...
	- _level_ -- level of logging, one of `off`, `emerg`, `alert`, `critical`,
	  `err`, `warn`, `notice`, `info` and `debug`
	- _max-size_ -- maximal size of log file before rotating
	- _rotations_ -- number of rotation kept
- _queue_manager_ -- selection of the queue manager implementation responsible for assigning jobs to workers. Currently only `single` (the default) and `multi` queue managers are in production version. Single-queue manager has one queue and dispatches jobs on demand as workers become available. Multi-queue manager has a queue for every worker, jobs are assigned immediately and cannot be re-assigned unless failure occurs. I.e., `single` provides better load balancing, `multi` has lower dispatching overhead.

### Example config file

```{.yml}
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
    username: ""
    password: ""
logger:
    file: "/var/log/recodex/broker"  # w/o suffix - actual names will be
	                                 # broker.log, broker.1.log, ...
    level: "debug"  # level of logging
    max-size: 1048576  # 1 MB; max size of file before log rotation
    rotations: 3  # number of rotations kept
queue_manager: "single"  # name of the manager that handles job dispatching among queues (single is the default)
```

## Documentation

Feel free to read the documentation on [our wiki](https://github.com/ReCodEx/wiki/wiki).
