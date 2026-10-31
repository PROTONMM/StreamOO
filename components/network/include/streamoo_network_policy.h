#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
bool streamoo_network_password_valid(const uint8_t *,size_t);
bool streamoo_network_ap_name(const uint8_t mac[6],char out[16]);
