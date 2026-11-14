#include "streamoo_platform.h"
#include "sdkconfig.h"

#if defined(CONFIG_STREAMOO_CRYPTO_EARLY_INIT)
#if !defined(CONFIG_MBEDTLS_HARDWARE_AES) || !defined(CONFIG_MBEDTLS_HARDWARE_SHA)
#error "Early crypto preparation requires hardware AES and SHA"
#endif
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "aes/esp_aes.h"
#include "psa/crypto.h"
#include "esp_heap_caps.h"
#include "esp_log.h"

/* Original boot probe using a public, all-zero AES-128-CBC vector (1024B).
 * Reference digests generated independently with OpenSSL / Python hashlib.
 * 1024B exceeds both SDK small-data AES thresholds (128/512B), so this
 * exercises GDMA even when small-data optimization is enabled. The SDK keeps
 * its shared crypto channels after contexts/buffers are released. */
esp_err_t streamoo_platform_crypto_prepare(void)
{
    static const uint8_t plain_hash[32]={
        0x5f,0x70,0xbf,0x18,0xa0,0x86,0x00,0x70,0x16,0xe9,0x48,0xb0,0x4a,0xed,0x3b,0x82,
        0x10,0x3a,0x36,0xbe,0xa4,0x17,0x55,0xb6,0xcd,0xdf,0xaf,0x10,0xac,0xe3,0xc6,0xef};
    static const uint8_t cipher_hash[32]={
        0xbd,0x7f,0x57,0x41,0x08,0x74,0x76,0x7e,0x57,0x5b,0x8e,0xb8,0xdf,0x84,0x3f,0x15,
        0x5c,0x69,0x54,0x17,0x5f,0xb6,0xb8,0xae,0x5c,0x6e,0x7d,0x48,0x3e,0x5a,0xb3,0x56};
    const char *stage="PSA init";
    int status=psa_crypto_init();
    if(status!=PSA_SUCCESS){
        ESP_LOGE("platform","Early crypto failed: stage=%s status=%d",stage,status);
        return ESP_FAIL;
    }
    uint8_t *memory=heap_caps_calloc(2,1024,MALLOC_CAP_INTERNAL|MALLOC_CAP_DMA);
    if(!memory)return ESP_ERR_NO_MEM;
    uint8_t *plain=memory,*cipher=memory+1024;
    uint8_t digest[32],iv[16]={0},key[16]={0};
    size_t length=0;
    stage="SHA input";
    status=psa_hash_compute(PSA_ALG_SHA_256,plain,1024,digest,sizeof digest,&length);
    bool ok=status==PSA_SUCCESS && length==sizeof digest && !memcmp(digest,plain_hash,sizeof digest);
    if(ok){
        esp_aes_context aes;
        esp_aes_init(&aes);
        stage="AES key";
        status=esp_aes_setkey(&aes,key,128);
        if(status==0){
            stage="AES CBC";
            status=esp_aes_crypt_cbc(&aes,ESP_AES_ENCRYPT,1024,iv,plain,cipher);
        }
        esp_aes_free(&aes);
        ok=status==0;
    }
    if(ok){
        stage="AES/SHA output";
        status=psa_hash_compute(PSA_ALG_SHA_256,cipher,1024,digest,sizeof digest,&length);
        ok=status==PSA_SUCCESS && length==sizeof digest && !memcmp(digest,cipher_hash,sizeof digest);
    }
    heap_caps_free(memory);
    if(!ok){
        ESP_LOGE("platform","Early crypto failed: stage=%s status=%d (known-answer check)",stage,status);
        return ESP_FAIL;
    }
    ESP_LOGI("platform","Early crypto: hardware AES/SHA known-answer PASS before I2S; 1024B GDMA probe");
    return ESP_OK;
}
#else
esp_err_t streamoo_platform_crypto_prepare(void){return ESP_OK;}
#endif
