#include "streamoo_network_policy.h"
#include <stdio.h>
bool streamoo_network_password_valid(const uint8_t *password,size_t bytes)
{
    if (!password || bytes<8 || bytes>63) return false;
    for (size_t i=0;i<bytes;++i) if (password[i]<32 || password[i]>126) return false;
    return true;
}
bool streamoo_network_ap_name(const uint8_t mac[6],char out[16])
{
    return mac && out && snprintf(out,16,"StreamOO-%02X%02X%02X",mac[3],mac[4],mac[5])==15;
}
