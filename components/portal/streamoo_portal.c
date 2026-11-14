#include "streamoo_portal.h"
#include "esp_http_server.h"
#include "esp_log.h"
esp_err_t streamoo_portal_register_configuration(httpd_handle_t);
void streamoo_portal_config_stopped(void);
static httpd_handle_t server;
static streamoo_portal_status_fn read_status;
static void *status_context;
extern const unsigned char html_start[] __asm__("_binary_index_html_start"), html_end[] __asm__("_binary_index_html_end");
extern const unsigned char css_start[] __asm__("_binary_portal_css_start"), css_end[] __asm__("_binary_portal_css_end");
extern const unsigned char js_start[] __asm__("_binary_portal_js_start"), js_end[] __asm__("_binary_portal_js_end");
static esp_err_t headers(httpd_req_t *r,const char *type)
{
    if (httpd_resp_set_type(r,type)!=ESP_OK ||
        httpd_resp_set_hdr(r,"Cache-Control","no-store")!=ESP_OK ||
        httpd_resp_set_hdr(r,"X-Content-Type-Options","nosniff")!=ESP_OK ||
        httpd_resp_set_hdr(r,"Referrer-Policy","no-referrer")!=ESP_OK ||
        httpd_resp_set_hdr(r,"Content-Security-Policy",
            "default-src 'self'; base-uri 'none'; object-src 'none'; frame-ancestors 'none'; form-action 'self'")!=ESP_OK)
        return ESP_FAIL;
    return ESP_OK;
}
static esp_err_t page(httpd_req_t *r)
{
    if (headers(r,"text/html; charset=utf-8")!=ESP_OK) return ESP_FAIL;
    return httpd_resp_send(r,(const char *)html_start,html_end-html_start);
}
static esp_err_t css(httpd_req_t *r)
{
    if (headers(r,"text/css; charset=utf-8")!=ESP_OK) return ESP_FAIL;
    return httpd_resp_send(r,(const char *)css_start,css_end-css_start);
}
static esp_err_t js(httpd_req_t *r)
{
    if (headers(r,"application/javascript; charset=utf-8")!=ESP_OK) return ESP_FAIL;
    return httpd_resp_send(r,(const char *)js_start,js_end-js_start);
}
static esp_err_t status(httpd_req_t *r)
{
    streamoo_portal_status_t snapshot={0}; char json[768];
    if (headers(r,"application/json; charset=utf-8")!=ESP_OK) return ESP_FAIL;
    if (!read_status(status_context,&snapshot) || !streamoo_portal_status_json(&snapshot,json,sizeof json)) {
        httpd_resp_set_status(r,"503 Service Unavailable");
        return httpd_resp_send(r,"{\"error\":\"status_unavailable\"}",HTTPD_RESP_USE_STRLEN);
    }
    return httpd_resp_send(r,json,HTTPD_RESP_USE_STRLEN);
}
esp_err_t streamoo_portal_start(streamoo_portal_status_fn getter,void *context)
{
    if (!getter) return ESP_ERR_INVALID_ARG;
    if (server) return ESP_ERR_INVALID_STATE;
    httpd_config_t config=HTTPD_DEFAULT_CONFIG();
    config.task_priority=2; config.stack_size=8192;
    config.max_open_sockets=4; config.max_uri_handlers=20; config.lru_purge_enable=true;
    config.recv_wait_timeout=3; config.send_wait_timeout=3;
    read_status=getter; status_context=context;
    esp_err_t error=httpd_start(&server,&config);
    if (error!=ESP_OK) { server=NULL; return error; }
    const httpd_uri_t routes[]={
        {.uri="/",.method=HTTP_GET,.handler=page},
        {.uri="/portal.css",.method=HTTP_GET,.handler=css},
        {.uri="/portal.js",.method=HTTP_GET,.handler=js},
        {.uri="/api/v1/status",.method=HTTP_GET,.handler=status},
    };
    for (unsigned i=0;i<4 && error==ESP_OK;++i) error=httpd_register_uri_handler(server,&routes[i]);
    if (error==ESP_OK) error=streamoo_portal_register_configuration(server);
    if (error!=ESP_OK) { streamoo_portal_stop(); return error; }
    ESP_LOGI("portal","Configuration portal ready: http://192.168.4.1/; sockets=4 priority=2");
    return ESP_OK;
}
esp_err_t streamoo_portal_stop(void)
{
    if (!server) return ESP_OK;
    esp_err_t error=httpd_stop(server);
    if (error==ESP_OK) { server=NULL; read_status=NULL; status_context=NULL; streamoo_portal_config_stopped(); }
    else ESP_LOGE("portal","Stop failed: %s; retaining server context",esp_err_to_name(error));
    return error;
}
