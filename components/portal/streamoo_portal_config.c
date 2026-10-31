#include "streamoo_portal_config.h"
#include "cJSON.h"
#include "streamoo_clock.h"
#include "esp_heap_caps.h"
#include "esp_http_server.h"
#include <inttypes.h>
#include <stdio.h>
#include <string.h>
static streamoo_portal_actions_t actions;
static bool configured, serving;
esp_err_t streamoo_portal_configure(const streamoo_portal_actions_t *a) {
  if (serving)
    return ESP_ERR_INVALID_STATE;
  if (!a || !a->wifi_load || !a->wifi_submit || !a->playlist ||
      !a->playlist_info || !a->timezone_get || !a->timezone_set)
    return ESP_ERR_INVALID_ARG;
  actions = *a;
  configured = true;
  return ESP_OK;
}
void streamoo_portal_config_stopped(void) { serving = false; }
static esp_err_t reply(httpd_req_t *r, const char *status, const char *json) {
  httpd_resp_set_status(r, status);
  /* Error handlers return ESP_FAIL after the response: SDK closes the session
   * without purging an arbitrarily large, rejected body. */
  httpd_resp_set_hdr(r, "Connection", "close");
  httpd_resp_set_type(r, "application/json; charset=utf-8");
  httpd_resp_set_hdr(r, "Cache-Control", "no-store");
  httpd_resp_set_hdr(r, "X-Content-Type-Options", "nosniff");
  esp_err_t sent = httpd_resp_send(r, json, HTTPD_RESP_USE_STRLEN);
  return sent == ESP_OK && status[0] >= '4' ? ESP_FAIL : sent;
}
static bool request_allowed(httpd_req_t *r) {
  char proof[4], origin[128], host[96], expected[104];
  if (httpd_req_get_hdr_value_len(r, "X-StreamOO-Request") != 1 ||
      httpd_req_get_hdr_value_str(r, "X-StreamOO-Request", proof,
                                  sizeof proof) != ESP_OK ||
      strcmp(proof, "1")) {
    reply(r, "403 Forbidden", "{\"error\":\"request_proof_required\"}");
    return false;
  }
  size_t origin_length = httpd_req_get_hdr_value_len(r, "Origin");
  if (origin_length) {
    size_t host_length = httpd_req_get_hdr_value_len(r, "Host");
    if (origin_length >= sizeof origin || !host_length ||
        host_length >= sizeof host ||
        httpd_req_get_hdr_value_str(r, "Origin", origin, sizeof origin) !=
            ESP_OK ||
        httpd_req_get_hdr_value_str(r, "Host", host, sizeof host) != ESP_OK) {
      reply(r, "403 Forbidden", "{\"error\":\"origin\"}");
      return false;
    }
    snprintf(expected, sizeof expected, "http://%s", host);
    if (strcmp(origin, expected)) {
      reply(r, "403 Forbidden", "{\"error\":\"origin\"}");
      return false;
    }
  }
  return true;
}
static uint8_t *body(httpd_req_t *r, size_t limit) {
  if (!r->content_len || r->content_len > limit) {
    reply(r, "413 Content Too Large", "{\"error\":\"body_size\"}");
    return NULL;
  }
  uint8_t *data =
      heap_caps_malloc(r->content_len + 1, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  if (!data) {
    reply(r, "503 Service Unavailable", "{\"error\":\"memory\"}");
    return NULL;
  }
  size_t done = 0;
  while (done < r->content_len) {
    int n = httpd_req_recv(r, (char *)data + done, r->content_len - done);
    if (n <= 0) {
      heap_caps_free(data);
      reply(r, "408 Request Timeout", "{\"error\":\"incomplete_body\"}");
      return NULL;
    }
    done += (size_t)n;
  }
  data[done] = 0;
  return data;
}
static bool content_type(httpd_req_t *r, const char *expected) {
  char type[64];
  if (httpd_req_get_hdr_value_str(r, "Content-Type", type, sizeof type) !=
      ESP_OK)
    return false;
  size_t n = strlen(expected);
  return !strncmp(type, expected, n) && (!type[n] || type[n] == ';');
}
static esp_err_t wifi_get(httpd_req_t *r) {
  streamoo_wifi_profiles_t saved = {0};
  if (!actions.wifi_load(&saved))
    return reply(r, "503 Service Unavailable",
                 "{\"error\":\"profiles_unavailable\"}");
  cJSON *root = cJSON_CreateObject(),
        *array = root ? cJSON_AddArrayToObject(root, "profiles") : NULL;
  bool ok = array != NULL;
  for (unsigned i = 0; i < saved.count && ok; ++i) {
    cJSON *entry = cJSON_CreateObject();
    ok = entry &&
         cJSON_AddStringToObject(entry, "ssid", saved.profiles[i].ssid) &&
         cJSON_AddBoolToObject(entry, "has_password",
                               saved.profiles[i].password[0] != 0);
    if (ok)
      cJSON_AddItemToArray(array, entry);
    else
      cJSON_Delete(entry);
  }
  char json[2048];
  ok = ok && cJSON_PrintPreallocated(root, json, sizeof json, false);
  cJSON_Delete(root);
  memset(&saved, 0, sizeof saved);
  return ok ? reply(r, "200 OK", json)
            : reply(r, "503 Service Unavailable", "{\"error\":\"memory\"}");
}
static esp_err_t wifi_post(httpd_req_t *r) {
  if (!request_allowed(r))
    return ESP_FAIL;
  if (!content_type(r, "application/json"))
    return reply(r, "415 Unsupported Media Type",
                 "{\"error\":\"json_required\"}");
  uint8_t *data = body(r, 4096);
  if (!data)
    return ESP_FAIL;
  streamoo_wifi_profiles_t old = {0}, profiles = {0};
  bool valid = actions.wifi_load(&old) &&
               streamoo_wifi_profiles_parse((char *)data, r->content_len, &old,
                                            &profiles);
  memset(data, 0, r->content_len);
  heap_caps_free(data);
  memset(&old, 0, sizeof old);
  if (!valid) {
    memset(&profiles, 0, sizeof profiles);
    return reply(r, "400 Bad Request", "{\"error\":\"invalid_profiles\"}");
  }
  esp_err_t error = actions.wifi_submit(&profiles);
  memset(&profiles, 0, sizeof profiles);
  return error == ESP_OK ? reply(r, "202 Accepted", "{\"accepted\":true}")
                         : reply(r, "409 Conflict",
                                 "{\"error\":\"connection_test_in_progress\"}");
}
static esp_err_t report(httpd_req_t *r, const char *status,
                        const streamoo_playlist_report_t *p, bool committed) {
  char json[256];
  snprintf(json, sizeof json,
           "{\"rows\":%" PRIu32 ",\"normalized\":%" PRIu32
           ",\"duplicates\":%" PRIu32 ",\"error_line\":%" PRIu32
           ",\"error\":\"%s\",\"committed\":%s}",
           p->rows, p->normalized, p->duplicates, p->error_line,
           p->error ? p->error : "", committed ? "true" : "false");
  return reply(r, status, json);
}
static esp_err_t playlist_request(httpd_req_t *r) {
  if (!request_allowed(r))
    return ESP_FAIL;
  if (!content_type(r, "text/plain"))
    return reply(r, "415 Unsupported Media Type",
                 "{\"error\":\"text_required\"}");
  uint8_t *data = body(r, STREAMOO_PLAYLIST_MAX_BYTES);
  if (!data)
    return ESP_FAIL;
  bool commit = r->user_ctx != NULL;
  streamoo_playlist_report_t parsed = {0};
  esp_err_t error = actions.playlist(data, r->content_len, commit, &parsed);
  heap_caps_free(data);
  if (error == ESP_OK)
    return report(r, "200 OK", &parsed, commit);
  if (error == ESP_ERR_INVALID_ARG)
    return report(r, "400 Bad Request", &parsed, false);
  return reply(r, "503 Service Unavailable",
               "{\"error\":\"playlist_storage_unavailable\"}");
}
static esp_err_t playlist_get(httpd_req_t *r) {
  streamoo_playlist_report_t p = {0};
  return actions.playlist_info(&p)
             ? report(r, "200 OK", &p, false)
             : reply(r, "503 Service Unavailable",
                     "{\"error\":\"playlist_storage_unavailable\"}");
}
static esp_err_t clock_get(httpd_req_t *r) {
  if(!actions.timezone_get)return reply(r,"503 Service Unavailable","{\"error\":\"clock_unavailable\"}");
  cJSON *root=cJSON_CreateObject();
  cJSON *zones=root ? cJSON_AddArrayToObject(root,"zones"):NULL;
  const char *current=actions.timezone_get();
  bool ok=zones && current && cJSON_AddStringToObject(root,"timezone",current);
  for(unsigned i=0;i<STREAMOO_CLOCK_ZONE_COUNT && ok;++i){
    cJSON *item=cJSON_CreateString(streamoo_clock_zone_name((streamoo_clock_zone_t)i));
    if(!item)ok=false;else cJSON_AddItemToArray(zones,item);
  }
  char json[1024];ok=ok && cJSON_PrintPreallocated(root,json,sizeof json,false);cJSON_Delete(root);
  return ok ? reply(r,"200 OK",json):reply(r,"503 Service Unavailable","{\"error\":\"clock_unavailable\"}");
}
static esp_err_t clock_post(httpd_req_t *r) {
  if(!request_allowed(r))return ESP_FAIL;
  if(!content_type(r,"application/json"))return reply(r,"415 Unsupported Media Type","{\"error\":\"json_required\"}");
  uint8_t *data=body(r,128);if(!data)return ESP_FAIL;
  cJSON *root=memchr(data,0,r->content_len) ? NULL:cJSON_ParseWithLengthOpts((char *)data,r->content_len+1,NULL,true);
  cJSON *field=root ? cJSON_GetObjectItemCaseSensitive(root,"timezone"):NULL;
  streamoo_clock_zone_t zone;
  bool valid=cJSON_IsObject(root) && cJSON_GetArraySize(root)==1 && cJSON_IsString(field) &&
      streamoo_clock_zone_parse(field->valuestring,&zone);
  esp_err_t error=valid && actions.timezone_set ? actions.timezone_set(field->valuestring):ESP_ERR_INVALID_ARG;
  cJSON_Delete(root);heap_caps_free(data);
  if(!valid)return reply(r,"400 Bad Request","{\"error\":\"invalid_timezone\"}");
  return error==ESP_OK ? reply(r,"200 OK","{\"saved\":true}"):
      reply(r,"503 Service Unavailable","{\"error\":\"clock_save_failed\"}");
}
esp_err_t streamoo_portal_register_configuration(httpd_handle_t server) {
  if (!configured)
    return ESP_OK;
  const httpd_uri_t routes[] = {
      {.uri = "/api/v1/wifi/profiles", .method = HTTP_GET, .handler = wifi_get},
      {.uri = "/api/v1/wifi/config", .method = HTTP_POST, .handler = wifi_post},
      {.uri = "/api/v1/stations/import/preview",
       .method = HTTP_POST,
       .handler = playlist_request},
      {.uri = "/api/v1/stations/import",
       .method = HTTP_POST,
       .handler = playlist_request,
       .user_ctx = (void *)1},
      {.uri = "/api/v1/stations/import/status",
       .method = HTTP_GET,
       .handler = playlist_get},
  };
  for (unsigned i = 0; i < sizeof routes / sizeof routes[0]; ++i) {
    esp_err_t error = httpd_register_uri_handler(server, &routes[i]);
    if (error != ESP_OK)
      return error;
  }
  const httpd_uri_t clock_routes[]={
      {.uri="/api/v1/clock",.method=HTTP_GET,.handler=clock_get},
      {.uri="/api/v1/clock",.method=HTTP_POST,.handler=clock_post}};
  for(unsigned i=0;i<2;++i){
    esp_err_t error=httpd_register_uri_handler(server,&clock_routes[i]);
    if(error!=ESP_OK)return error;
  }
  serving = true;
  return ESP_OK;
}
