"""Run the real boot probe against OpenSSL; inject errors/corruption at its boundary."""
from pathlib import Path
import subprocess
import tempfile
import unittest

ROOT=Path(__file__).resolve().parents[1]
HEADER=r'''
#pragma once
#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
typedef int esp_err_t;
#define ESP_OK 0
#define ESP_FAIL -1
#define ESP_ERR_NO_MEM -2
#define MALLOC_CAP_INTERNAL 1
#define MALLOC_CAP_DMA 2
void *heap_caps_calloc(size_t,size_t,unsigned);
void heap_caps_free(void *);
static inline void test_log(const char *tag,const char *format,...) __attribute__((format(printf,2,3)));
static inline void test_log(const char *tag,const char *format,...){(void)tag;(void)format;}
#define ESP_LOGI(...) test_log(__VA_ARGS__)
#define ESP_LOGE(...) test_log(__VA_ARGS__)
typedef int psa_status_t;
#define PSA_SUCCESS 0
#define PSA_ALG_SHA_256 0x02000009
psa_status_t psa_crypto_init(void);
psa_status_t psa_hash_compute(int,const uint8_t *,size_t,uint8_t *,size_t,size_t *);
typedef struct {uint8_t key[16];} esp_aes_context;
#define ESP_AES_ENCRYPT 1
void esp_aes_init(esp_aes_context *);
void esp_aes_free(esp_aes_context *);
int esp_aes_setkey(esp_aes_context *,const uint8_t *,unsigned);
int esp_aes_crypt_cbc(esp_aes_context *,int,size_t,uint8_t *,const uint8_t *,uint8_t *);
'''
HARNESS=r'''
#include "common.h"
#include "streamoo_platform.h"
#include <openssl/evp.h>
#include <openssl/sha.h>
static int mode,init,allocated,freed,hashes,aes_started,aes_freed,ciphers;
void *heap_caps_calloc(size_t count,size_t size,unsigned caps){
 assert(count==2&&size==1024&&caps==(MALLOC_CAP_INTERNAL|MALLOC_CAP_DMA));
 if(mode==2)return NULL;
 ++allocated;return calloc(count,size);
}
void heap_caps_free(void *memory){assert(memory);++freed;free(memory);}
psa_status_t psa_crypto_init(void){++init;return mode==1?-1:PSA_SUCCESS;}
psa_status_t psa_hash_compute(int alg,const uint8_t *input,size_t size,uint8_t *out,size_t capacity,size_t *length){
 assert(init==1&&alg==PSA_ALG_SHA_256&&size==1024&&capacity==32);
 ++hashes;if((hashes==1&&mode==3)||(hashes==2&&mode==10))return -2;
 assert(SHA256(input,size,out));*length=32;
 if(hashes==1&&mode==4)out[0]^=1;
 if((hashes==1&&mode==5)||(hashes==2&&mode==9))*length=31;
 return PSA_SUCCESS;
}
void esp_aes_init(esp_aes_context *aes){++aes_started;memset(aes,0,sizeof(*aes));}
void esp_aes_free(esp_aes_context *aes){++aes_freed;memset(aes,0,sizeof(*aes));}
int esp_aes_setkey(esp_aes_context *aes,const uint8_t *key,unsigned bits){
 assert(bits==128);memcpy(aes->key,key,16);return mode==6?-3:0;
}
int esp_aes_crypt_cbc(esp_aes_context *aes,int operation,size_t size,uint8_t *iv,const uint8_t *input,uint8_t *out){
 assert(operation==ESP_AES_ENCRYPT&&size==1024&&hashes==1);++ciphers;
 if(mode==7)return -4;
 EVP_CIPHER_CTX *ctx=EVP_CIPHER_CTX_new();assert(ctx);int used=0,last=0;
 assert(EVP_EncryptInit_ex(ctx,EVP_aes_128_cbc(),NULL,aes->key,iv)==1);
 assert(EVP_CIPHER_CTX_set_padding(ctx,0)==1);
 assert(EVP_EncryptUpdate(ctx,out,&used,input,(int)size)==1);
 assert(EVP_EncryptFinal_ex(ctx,out+used,&last)==1&&used+last==(int)size);
 EVP_CIPHER_CTX_free(ctx);
 if(mode==8)out[500]^=1; /* full-output digest must catch non-prefix corruption */
 return 0;
}
int main(int argc,char **argv){
 assert(argc==2);mode=atoi(argv[1]);int result=streamoo_platform_crypto_prepare();
#ifdef TEST_ENABLED
 assert(init==1);assert(result==(mode==0?ESP_OK:mode==2?ESP_ERR_NO_MEM:ESP_FAIL));
 assert(allocated==freed&&aes_started==aes_freed);
 if(mode==0)assert(hashes==2&&ciphers==1&&allocated==1);
#else
 assert(result==ESP_OK&&!init&&!allocated&&!hashes&&!aes_started);
#endif
 return 0;
}
'''

class CryptoPrepareTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.temp=tempfile.TemporaryDirectory();cls.addClassCleanup(cls.temp.cleanup)
        base=Path(cls.temp.name);(base/'aes').mkdir();(base/'psa').mkdir()
        (base/'common.h').write_text(HEADER)
        for header in ('esp_err.h','esp_heap_caps.h','esp_log.h','aes/esp_aes.h','psa/crypto.h'):
            (base/header).write_text('#include "common.h"\n')
        (base/'harness.c').write_text(HARNESS)
        cls.binaries=[]
        for enabled in (False,True):
            (base/'sdkconfig.h').write_text('#pragma once\n'+('''
#define CONFIG_STREAMOO_CRYPTO_EARLY_INIT 1
#define CONFIG_MBEDTLS_HARDWARE_AES 1
#define CONFIG_MBEDTLS_HARDWARE_SHA 1
''' if enabled else ''))
            binary=base/str(enabled)
            subprocess.run(['cc','-std=c11','-Wall','-Wextra','-Werror',*(['-DTEST_ENABLED'] if enabled else []),
                '-I'+str(base),'-I'+str(ROOT/'components/platform/include'),
                str(ROOT/'components/platform/streamoo_crypto_prepare.c'),str(base/'harness.c'),
                '-lcrypto','-o',str(binary)],check=True)
            cls.binaries.append(binary)

    def run_case(self,mode,enabled=True):
        subprocess.run([str(self.binaries[enabled]),str(mode)],check=True,timeout=5)

    def test_known_answer_full_cbc_and_sha_with_real_openssl(self):self.run_case(0)
    def test_disabled_profile_performs_no_crypto_or_allocation(self):self.run_case(0,False)
    def test_failures_lengths_and_corruption_release_every_resource(self):
        for mode in range(1,11):
            with self.subTest(mode=mode):self.run_case(mode)
