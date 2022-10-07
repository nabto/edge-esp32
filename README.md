# Nabto Edge ESP32 Integration (BETA)

This repository includes examples of how to make a Nabto Edge integration using the esp32. For a general introduction to Nabto Edge, please see https://docs.nabto.com as well as the [integration guide](https://docs.nabto.com/developer/guides/concepts/integration/intro.html).

## examples/thermostat

The thermostat example is a port of the thermostat found in the Nabto Edge Embedded SDK.

### Using the Thermostat example

This guide requires that the tools has been setup as described on
https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/ it's
also a good idea to have basic knowledge about ESP-IDF and their tools.

Goto the folder examples/thermostat

First we need to configure the example. We need to configure the product_id,
device_id, wifi network and wifi password. start the configuration utility by
running `idf.py menuconfig` goto `Component Config -> Nabto Example`. The
product_id and device_id is created on the Nabto Cloud Console
https://console.cloud.nabto.com

When the example has been configured the code is built with `idf.py build` and
flashed to the esp32 using `idf.py flash` once the code has been flashed see the
output from the demo using `idf.py monitor`

Once the example is running it says

```
I (31455) device_event_handler: The fingerprint ... is not known by the Nabto Edge Cloud. fix it on
https://console.cloud.nabto.com
```

It means that the fingerprint for the device needs to be registered in the nabto
cloud console. Copy the fingerprint and configure the device with that
fingerprint in the Nabto Cloud Console.

When everything is ok the example writes: `I (44705) device_event_handler:
Attached to the basestation`.

The thermostat example can be tested using either the IOS or Android app.

Android: https://play.google.com/store/apps/details?id=com.nabto.edge.thermostatdemo
IOS: https://apps.apple.com/us/app/nabto-edge-thermostat/id1643535407


## WORK IN PROGRESS

The below sections are work in progress and is not meant to be followed yet.

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
