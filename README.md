

# nabto5-esp-eye example integrations

This repository includes examples of how to make a Nabto Edge integration using the esp32. For a general introduction to Nabto Edge, please see https://docs.nabto.com as well as the [integration guide](https://docs.nabto.com/developer/guides/integration/intro.html).

## simple 

A minimal integration example of setting up a Nabto Edge CoAP device. No user authentication is setup in the example.

## tcptunnel

An example creating a small webserver which can be reached by a Nabto Edge Tunnel. It includes setup/configuration of access control through IAM and policies.

## common

Common integration "glue" component that glues the Nabto5 into the ESP32 environment. The component is used by the above examples.

## nabto-embedded-sdk

This is a git submodule pointing to the Nabto Edge Embedded SDK repo.
