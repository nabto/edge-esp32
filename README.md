

# nabto5-esp-eye example integraitons

This repository includes examples of how to make a nabto edge integration using the esp32

## simple 

Is a very very minimal integration example of setting up a nabto edge coap.
No user authentication is setup in the example

## tcptunnel

Is an example creating a small webserver which can be reached by nabto tunnel services
It includes setup/configuration of iam and policy services

## common

This is NOT an example. This is a common integration "glue" component that glues the Nabto5 into the ESP32 environment. The component is used by all the examples

## nabto-embedded-sdk

This is a git submodule pointing to the nabto embedded sdk sourcecode repository
