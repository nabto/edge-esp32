# Nabto Edge ESP32 Integration

This repository includes examples of how to make a Nabto Edge integration using the [Espressif ESP32](https://www.espressif.com/en/products/modules/esp32) module.

For more information about using the Nabto Edge Embedded SDK with ESP32, read our [ESP32 Platform Guide](https://docs.nabto.com/developer/platforms/embedded/esp32.html). For a general introduction to Nabto Edge, please see https://docs.nabto.com.

## Logging in examples

Each component uses its own log module. The Nabto Embedded SDK core logs are mapped to ESP logs with the Nabto `trace` level mapped to the ESP `verbose` level.

To set the log level for the entire application, use `idf.py menuconfig`, navigate to "Component config" -> "Log output" -> "Default log verbosity".

To set the log level for individual log modules, open the main function source code for the particular example and set the log level for the particular log module eg.:

```
esp_log_level_set("iam", ESP_LOG_VERBOSE);
```
Then set the maximum log verbosity to `Verbose` using `idf.py menuconfig` and navigating to "Component config" -> "Log output" -> "Maximum log verbosity".

## examples/thermostat

The thermostat example is a port of the thermostat example found in the Nabto Edge Embedded SDK.

### Using the Thermostat example

This guide requires that the tools has been setup as described on
https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/ it's
also a good idea to have basic knowledge about ESP-IDF and their tools.


For the demos we need the [Edge Key
Tool](https://github.com/nabto/edge-key-tool) and we need access to the [Nabto
Cloud Console](https://console.cloud.nabto.com)

Goto the folder examples/thermostat

First we need to configure the example. We need to configure the product_id,
device_id, wifi network and wifi password. start the configuration utility by
running `idf.py menuconfig` goto `Example Configuration`. The product_id and
device_id is created on the Nabto Cloud Console https://console.cloud.nabto.com
and the private_key is created using the Edge Key Tool, the fingerprint from the
Edge Key Tool should be inserted into the device in the console.

When the example has been configured the code is built with `idf.py build` and
flashed to the esp32 using `idf.py flash` once the code has been flashed see the
output from the demo using `idf.py monitor`

When everything is ok the example writes: `I (44705) device_event_handler:
Attached to the basestation`.

The thermostat example can be tested using either the
[IOS](https://apps.apple.com/us/app/nabto-edge-thermostat/id1643535407) or
[Android](https://play.google.com/store/apps/details?id=com.nabto.edge.thermostatdemo)
app.

## examples/simple_coap

This example is the ESP32 equivalent of the simple_coap example found at
`https://github.com/nabto/nabto-embedded-sdk/tree/master/examples/simple_coap`.
This is a simple example showing how to start a device which attaches to the
basestation and creates a simple CoAP endpoint which returns "HelloWorld" the
demo does not use any IAM(Identity and Access Management) etc.

To use this example first configure the example in `Component Config -> Nabto
Example` set the wifi name, password and the Nabto product id and device id.
Refer to the examples/thermostat section for how to obtain these id's and how to
configure the fingerprint.

When the example is running use the `simple_coap_client`
(https://github.com/nabto/nabto-client-sdk-examples) executable to test the
example. A simple Android or IOS client can also be created.

## examples/tcptunnel

An example creating a small webserver which can be reached by a Nabto Edge TCP
Tunnel. It includes setup/configuration of access control through IAM and
policies. The tunnel can be used with the nabto edge tcp tunnel client
https://github.com/nabto/nabto-client-edge-tunnel or another suitable app.

## Usage In Own Projects

For using the Nabto Edge For ESP32 in your own project the following approach
can be used.

Clone this repository and use the nabto_device component.

git clone --recursive https://github.com/nabto/edge-esp32

in the current esp32 project add the path to the checkout


```
set(EXTRA_COMPONENT_DIRS ../edge-esp32/components)
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


## Flash and memory usage by nabto.

For the measurements we measure the difference between the
`examples/wifi/getting_started/station` example and the Nabto Edge Thermostat
example.

### Baseline resource usage measurements

The `examples/wifi/getting_started/station` example has been modified to print
out heap information in app_main() the following snippet has been added

```
void app_main(void) {
    ...
    ...
    for (;;) {
        vTaskDelay(10000/portTICK_PERIOD_MS);
        heap_caps_print_heap_info(MALLOC_CAP_8BIT);
        fflush(stdout);
    }
}
```

Flash usage (`idf.py size`):
```
Total sizes:
Used static DRAM:   31072 bytes ( 149664 remain, 17.2% used)
      .data size:   15088 bytes
      .bss  size:   15984 bytes
Used static IRAM:   85166 bytes (  45906 remain, 65.0% used)
      .text size:   84139 bytes
   .vectors size:    1027 bytes
Used Flash size :  615825 bytes
           .text:  488759 bytes
         .rodata:  126810 bytes
Total image size:  716079 bytes (.bin may be padded larger)
```

Heap Usage (`heap_caps_print_heap_info(MALLOC_CAP_8BIT)`):
```
Heap summary for capabilities 0x00000004:
  At 0x3ffae6e0 len 6432 free 4 allocated 4648 min_free 4
    largest_free_block 0 alloc_blocks 39 free_blocks 0 total_blocks 39
  At 0x3ffb7960 len 165536 free 107844 allocated 55448 min_free 103976
    largest_free_block 104448 alloc_blocks 155 free_blocks 5 total_blocks 160
  At 0x3ffe0440 len 15072 free 13448 allocated 0 min_free 13448
    largest_free_block 13312 alloc_blocks 0 free_blocks 1 total_blocks 1
  At 0x3ffe4350 len 113840 free 112216 allocated 0 min_free 112216
    largest_free_block 110592 alloc_blocks 0 free_blocks 1 total_blocks 1
  Totals:
    free 233512 allocated 60096 min_free 229644 largest_free_block 110592
```

### Nabto Edge ESP32 Thermostat Example Resource usage

Flash usage (`idf.py size`)
```
Total sizes:
Used static DRAM:   34016 bytes ( 146720 remain, 18.8% used)
      .data size:   15592 bytes
      .bss  size:   18424 bytes
Used static IRAM:   85878 bytes (  45194 remain, 65.5% used)
      .text size:   84851 bytes
   .vectors size:    1027 bytes
Used Flash size :  926669 bytes
           .text:  751227 bytes
         .rodata:  175186 bytes
Total image size: 1028139 bytes (.bin may be padded larger)
```

Heap Usage (`heap_caps_print_heap_info(MALLOC_CAP_8BIT)`):

Just attached to the basestation
```
Heap summary for capabilities 0x00000004:
  At 0x3ffae6e0 len 6432 free 4 allocated 4648 min_free 4
    largest_free_block 0 alloc_blocks 39 free_blocks 0 total_blocks 39
  At 0x3ffb84e0 len 162592 free 348 allocated 157032 min_free 4
    largest_free_block 156 alloc_blocks 897 free_blocks 6 total_blocks 903
  At 0x3ffe0440 len 15072 free 10468 allocated 2908 min_free 64
    largest_free_block 6400 alloc_blocks 18 free_blocks 5 total_blocks 23
  At 0x3ffe4350 len 113840 free 112216 allocated 0 min_free 110516
    largest_free_block 110592 alloc_blocks 0 free_blocks 1 total_blocks 1
  Totals:
    free 123036 allocated 164588 min_free 110588 largest_free_block 110592
```

Attached and One connection
```
Heap summary for capabilities 0x00000004:
  At 0x3ffae6e0 len 6432 free 4 allocated 4648 min_free 4
    largest_free_block 0 alloc_blocks 39 free_blocks 0 total_blocks 39
  At 0x3ffb84e0 len 162592 free 88 allocated 157264 min_free 4
    largest_free_block 48 alloc_blocks 904 free_blocks 3 total_blocks 907
  At 0x3ffe0440 len 15072 free 3432 allocated 9888 min_free 64
    largest_free_block 1472 alloc_blocks 32 free_blocks 9 total_blocks 41
  At 0x3ffe4350 len 113840 free 95492 allocated 16720 min_free 90392
    largest_free_block 94208 alloc_blocks 1 free_blocks 1 total_blocks 2
  Totals:
    free 99016 allocated 188520 min_free 90464 largest_free_block 94208
```

Attached and Two connections
```
Heap summary for capabilities 0x00000004:
  At 0x3ffae6e0 len 6432 free 4 allocated 4648 min_free 4
    largest_free_block 0 alloc_blocks 39 free_blocks 0 total_blocks 39
  At 0x3ffb84e0 len 162592 free 72 allocated 157264 min_free 4
    largest_free_block 48 alloc_blocks 908 free_blocks 2 total_blocks 910
  At 0x3ffe0440 len 15072 free 1408 allocated 11872 min_free 4
    largest_free_block 1120 alloc_blocks 42 free_blocks 8 total_blocks 50
  At 0x3ffe4350 len 113840 free 73516 allocated 38664 min_free 65948
    largest_free_block 69632 alloc_blocks 9 free_blocks 4 total_blocks 13
  Totals:
    free 75000 allocated 212448 min_free 65960 largest_free_block 69632
```

### Nabto Edge Thermostat Example Resource Usage Summary

Flash usage: `926669 bytes - 615825 bytes = 310844 bytes`

RAM usage:
  * Just attached: ~100kB
  * Per connection: ~24kB

## NVS storage

The demos store their application data in the NVS storage. General
consideration: in a production setup it is crucial to know exactly what data is
stored where such that firmware updates does not break the deployed devices or
their configuration.

### Production factory defaults

In a production setup NVS images can be created which contains product_id,
device_id and a private_key.

The fingerprints can then be obtained outside of the device and uploaded to the
nabto cloud. Alternatively the privatekey can be generated inside the device and
the fingerprint thus needs to be extracted and uploaded to the nabto cloud.
