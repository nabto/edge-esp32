#pragma once

#include <nabto/nabto_device.h>

struct nm_iam_configuration;
struct nm_iam_state;

struct nm_iam_state* tcptunnel_create_default_iam_state(NabtoDevice* device);
struct nm_iam_configuration* tcptunnel_create_iam_config();
