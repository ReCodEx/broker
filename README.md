# Broker
[![Build Status](https://travis-ci.org/ReCodEx/broker.svg?branch=master)](https://travis-ci.org/ReCodEx/broker)
[![License](http://img.shields.io/:license-mit-blue.svg)](http://badges.mit-license.org)
[![Docs](https://img.shields.io/badge/docs-latest-brightgreen.svg)](http://recodex.github.io/broker/)
[![Wiki](https://img.shields.io/badge/docs-wiki-orange.svg)](https://github.com/ReCodEx/GlobalWiki/wiki)

## How to run it

- Install dependencies according to [common](https://github.com/ReCodEx/GlobalWiki/wiki/Build-and-Deployment#common) configuration
- Download `zmq.hpp` and Google test framework using `git submodule update --init`
- Build with cmake: `mkdir build && cd build && cmake .. && make`
- Run `./recodex-broker`

