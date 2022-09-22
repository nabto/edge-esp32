# Nabto Edge ESP32 Integration (BETA)

This repository includes examples of how to make a Nabto Edge integration using the esp32. For a general introduction to Nabto Edge, please see https://docs.nabto.com as well as the [integration guide](https://docs.nabto.com/developer/guides/concepts/integration/intro.html).

## examples/simple_coap

A minimal integration example of setting up a Nabto Edge CoAP device. No user authentication is setup in the example.

## examples/tcptunnel

An example creating a small webserver which can be reached by a Nabto Edge Tunnel. It includes setup/configuration of access control through IAM and policies.

## common

Common integration "glue" component that glues the Nabto5 into the ESP32 environment. The component is used by the above examples.

## nabto-embedded-sdk

This is a git submodule pointing to the Nabto Edge Embedded SDK repo.


## NVS storage

### Production factory defaults

In a production setup NVS images can be created which contains product_id,
device_id and a private_key.

The fingerprints can then be obtained outside of the device and uploaded to the
nabto cloud. Alternatively the privatekey can be generated inside the device and
the fingerprint then needs to be extracted and uploaded to the nabto cloud.

## Usage

Clone this repository and use the appropriate component.

git clone --recursive https://github.com/nabto/edge-esp32-beta

in the current esp32 project add the path to the checkout


```
set(EXTRA_COMPONENT_DIRS ../edge-esp32-beta/common_components)
cmake_minimum_required(VERSION 3.5)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(example_with_nabto)
```

example main CMakeLists.txt
```
idf_component_register(
  SRCS
    "example.c"
  REQUIRES
    nabto_device)
```
