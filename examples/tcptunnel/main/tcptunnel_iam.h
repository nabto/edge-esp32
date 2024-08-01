#pragma once


#include <nabto/nabto_device.h>
#include <modules/iam/nm_iam_serializer.h>


struct nm_iam_configuration;
struct nm_iam_state;

struct nm_iam_state* tcptunnel_create_default_iam_state(NabtoDevice* device);
struct nm_iam_configuration* tcptunnel_create_iam_config();

char* tcptunnel_iam_create_pairing_string(struct nm_iam* iam, const char* productId, const char* deviceId);

