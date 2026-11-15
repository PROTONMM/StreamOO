#include "streamoo_portal_config.h"
#include "streamoo_settings.h"
#include "cJSON.h"
#include "streamoo_clock.h"
#include "streamoo_config_store.h"
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
      !a->playlist_info || !a->timezone_get || !a->timezone_set || !a->player_command || !a->player_mode || !a->station_select || !a->catalog_replace)
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
/* Bound recursive cJSON work before parsing on the 8 KiB HTTP task stack.
 * These endpoints only accept flat objects. Track string escapes so a literal
 * backslash followed by u0000 remains distinct from a decoded NUL. */
static cJSON *parse_json(const uint8_t *data, size_t bytes) {
  if (memchr(data, 0, bytes))
    return NULL;
  unsigned depth = 0;
  bool quoted = false;
  for (size_t i = 0; i < bytes; ++i) {
    uint8_t c = data[i];
    if (quoted) {
      if (c == '\\') {
        if (++i == bytes)
          return NULL;
        if (data[i] == 'u' && bytes - i >= 5 &&
            !memcmp(data + i + 1, "0000", 4))
          return NULL;
      } else if (c == '"')
        quoted = false;
    } else if (c == '"')
      quoted = true;
    else if (c == '{' || c == '[') {
      if (++depth > 4)
        return NULL;
    } else if (c == '}' || c == ']') {
      if (!depth)
        return NULL;
      --depth;
    }
  }
  if (depth || quoted)
    return NULL;
  return cJSON_ParseWithLengthOpts((const char *)data, bytes + 1, NULL, true);
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
  if (error == ESP_ERR_INVALID_STATE)
    return reply(r, "409 Conflict", "{\"error\":\"catalog_changed\"}");
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
  cJSON *root=parse_json(data, r->content_len);
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
static esp_err_t settings_get(httpd_req_t *r){
  streamoo_settings_t s=streamoo_settings_get();
  cJSON *root=cJSON_CreateObject();char json[512];
  bool ok=root&&cJSON_AddNumberToObject(root,"list_timeout_s",s.list_timeout_s)&&
    cJSON_AddBoolToObject(root,"list_autoselect",s.list_autoselect)&&
    cJSON_AddBoolToObject(root,"autoplay",s.autoplay)&&cJSON_AddStringToObject(root,"hostname",s.hostname)&&
    cJSON_PrintPreallocated(root,json,sizeof json,false);
  cJSON_Delete(root);return ok?reply(r,"200 OK",json):reply(r,"503 Service Unavailable","{\"error\":\"memory\"}");
}
static esp_err_t settings_post(httpd_req_t *r){
  if(!request_allowed(r))return ESP_FAIL;
  if(!content_type(r,"application/json"))return reply(r,"415 Unsupported Media Type","{\"error\":\"json_required\"}");
  uint8_t *data=body(r,512);if(!data)return ESP_FAIL;
  cJSON *root=parse_json(data, r->content_len);
  cJSON *timeout=cJSON_GetObjectItemCaseSensitive(root,"list_timeout_s"),
        *select=cJSON_GetObjectItemCaseSensitive(root,"list_autoselect"),
        *autoplay=cJSON_GetObjectItemCaseSensitive(root,"autoplay"),
        *hostname=cJSON_GetObjectItemCaseSensitive(root,"hostname");
  streamoo_settings_t s={0};
  bool valid=cJSON_IsObject(root)&&cJSON_GetArraySize(root)==4&&cJSON_IsNumber(timeout)&&
    timeout->valuedouble>=1&&timeout->valuedouble<=120&&timeout->valuedouble==(unsigned)timeout->valuedouble&&
    cJSON_IsBool(select)&&cJSON_IsBool(autoplay)&&cJSON_IsString(hostname)&&strlen(hostname->valuestring)<sizeof s.hostname;
  if(valid){s.list_timeout_s=(uint32_t)timeout->valuedouble;s.list_autoselect=cJSON_IsTrue(select);
    s.autoplay=cJSON_IsTrue(autoplay);strcpy(s.hostname,hostname->valuestring);valid=streamoo_settings_valid(&s);}
  cJSON_Delete(root);heap_caps_free(data);
  if(!valid)return reply(r,"400 Bad Request","{\"error\":\"invalid_settings\"}");
  return streamoo_settings_set(&s)==ESP_OK?reply(r,"200 OK","{\"saved\":true}"):
    reply(r,"503 Service Unavailable","{\"error\":\"settings_save_failed\"}");
}
static esp_err_t player_post(httpd_req_t *r) {
  if(!request_allowed(r))return ESP_FAIL;
  if(!content_type(r,"application/json"))return reply(r,"415 Unsupported Media Type","{\"error\":\"json_required\"}");
  uint8_t *data=body(r,128);if(!data)return ESP_FAIL;
  cJSON *root=parse_json(data, r->content_len);
  cJSON *action=root ? cJSON_GetObjectItemCaseSensitive(root,"action"):NULL;
  cJSON *value=root ? cJSON_GetObjectItemCaseSensitive(root,"value"):NULL;
  bool valid=cJSON_IsObject(root) && cJSON_IsString(action);
  streamoo_portal_player_action_t kind=STREAMOO_PORTAL_STOP;uint8_t volume=0;
  if(valid && !strcmp(action->valuestring,"volume")) {
    kind=STREAMOO_PORTAL_VOLUME;
    valid=cJSON_GetArraySize(root)==2 && cJSON_IsNumber(value) && value->valuedouble>=0 &&
        value->valuedouble<=100 && value->valuedouble==value->valueint;
    if(valid)volume=(uint8_t)value->valueint;
  } else if(valid) {
    valid=cJSON_GetArraySize(root)==1 && (!strcmp(action->valuestring,"play") || !strcmp(action->valuestring,"stop"));
    kind=!strcmp(action->valuestring,"play") ? STREAMOO_PORTAL_PLAY:STREAMOO_PORTAL_STOP;
  }
  cJSON_Delete(root);heap_caps_free(data);
  if(!valid)return reply(r,"400 Bad Request","{\"error\":\"invalid_player_command\"}");
  esp_err_t error=actions.player_command(kind,volume);
  if(error==ESP_ERR_INVALID_STATE)return reply(r,"409 Conflict","{\"error\":\"player_unavailable\"}");
  return error==ESP_OK ? reply(r,"202 Accepted","{\"accepted\":true}"):
      reply(r,"503 Service Unavailable","{\"error\":\"player_busy\"}");
}
static bool catalog_access(httpd_req_t *r){
  if(actions.player_mode && actions.player_mode())return true;
  reply(r,"409 Conflict","{\"error\":\"player_mode_required\"}");return false;
}
static bool generation(httpd_req_t *r,uint64_t *out){
  char value[24];
  if(httpd_req_get_hdr_value_str(r,"X-StreamOO-Catalog-Generation",value,sizeof value)!=ESP_OK||!value[0])return false;
  uint64_t number=0;
  for(const unsigned char *p=(const unsigned char *)value;*p;++p){
    if(*p<'0'||*p>'9'||number>(UINT64_MAX-(*p-'0'))/10)return false;
    number=number*10+(*p-'0');
  }
  *out=number;return true;
}
static esp_err_t catalog_get(httpd_req_t *r){
  if(!catalog_access(r))return ESP_FAIL;
  streamoo_catalog_snapshot_t *held=streamoo_config_catalog_acquire();
  if(!held)return reply(r,"503 Service Unavailable","{\"error\":\"catalog_unavailable\"}");
  streamoo_catalog_view_t view=streamoo_config_catalog_view(held);
  char *json=heap_caps_malloc(16384,MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT);
  if(!json){streamoo_config_catalog_release(held);return reply(r,"503 Service Unavailable","{\"error\":\"memory\"}");}
  httpd_resp_set_type(r,"application/json; charset=utf-8");httpd_resp_set_hdr(r,"Cache-Control","no-store");httpd_resp_set_hdr(r,"X-Content-Type-Options","nosniff");
  httpd_resp_set_hdr(r,"Connection","close");
  snprintf(json,16384,"{\"generation\":\"%" PRIu64 "\",\"stations\":[",view.generation);
  esp_err_t error=httpd_resp_send_chunk(r,json,strlen(json));
  for(uint32_t i=0;i<view.count&&error==ESP_OK;++i){
    const streamoo_station_t *station=&view.stations[i];char id[17];snprintf(id,sizeof id,"%016" PRIx64,station->stable_id);
    cJSON *item=cJSON_CreateObject();
    bool ok=item && cJSON_AddStringToObject(item,"id",id) && cJSON_AddStringToObject(item,"name",(const char *)station->name) &&
        cJSON_AddStringToObject(item,"url",(const char *)station->url) && cJSON_AddNumberToObject(item,"ovol",station->ovol) &&
        cJSON_PrintPreallocated(item,json,16384,false);
    cJSON_Delete(item);
    if(!ok){error=ESP_ERR_NO_MEM;break;}
    if(i)error=httpd_resp_send_chunk(r,",",1);
    if(error==ESP_OK)error=httpd_resp_send_chunk(r,json,strlen(json));
  }
  if(error==ESP_OK)error=httpd_resp_send_chunk(r,"]}",2);
  if(error==ESP_OK)error=httpd_resp_send_chunk(r,NULL,0);
  heap_caps_free(json);streamoo_config_catalog_release(held);return error;
}
static esp_err_t catalog_export(httpd_req_t *r){
  if(!catalog_access(r))return ESP_FAIL;
  streamoo_catalog_snapshot_t *held=streamoo_config_catalog_acquire();
  if(!held)return reply(r,"503 Service Unavailable","{\"error\":\"catalog_unavailable\"}");
  streamoo_catalog_view_t view=streamoo_config_catalog_view(held);
  httpd_resp_set_type(r,"text/plain; charset=utf-8");httpd_resp_set_hdr(r,"Cache-Control","no-store");
  httpd_resp_set_hdr(r,"Content-Disposition","attachment; filename=playlist.csv");httpd_resp_set_hdr(r,"Connection","close");
  esp_err_t error=ESP_OK;
  for(uint32_t i=0;i<view.count&&error==ESP_OK;++i){
    const streamoo_station_t *s=&view.stations[i];char end[32];int n=snprintf(end,sizeof end,"\t%" PRId32 "\n",s->ovol);
    error=httpd_resp_send_chunk(r,(const char *)s->name,s->name_bytes);
    if(error==ESP_OK)error=httpd_resp_send_chunk(r,"\t",1);
    if(error==ESP_OK)error=httpd_resp_send_chunk(r,(const char *)s->url,s->url_bytes);
    if(error==ESP_OK)error=httpd_resp_send_chunk(r,end,n);
  }
  if(error==ESP_OK)error=httpd_resp_send_chunk(r,NULL,0);
  streamoo_config_catalog_release(held);return error;
}
static esp_err_t catalog_replace(httpd_req_t *r){
  if(!request_allowed(r)||!catalog_access(r))return ESP_FAIL;
  uint64_t expected;
  if(!generation(r,&expected))return reply(r,"400 Bad Request","{\"error\":\"catalog_generation_required\"}");
  bool editor=content_type(r,"application/vnd.streamoo.stations+tsv");
  if(!editor&&!content_type(r,"text/plain"))return reply(r,"415 Unsupported Media Type","{\"error\":\"text_required\"}");
  uint8_t *data=r->content_len ? body(r,STREAMOO_PLAYLIST_MAX_BYTES+(editor?17*STREAMOO_PLAYLIST_MAX_ROWS:0)):NULL;
  if(r->content_len&&!data)return ESP_FAIL;
  streamoo_playlist_report_t parsed={0};
  esp_err_t error=editor?streamoo_config_catalog_edit(data,r->content_len,expected,&parsed):
    actions.catalog_replace(data,r->content_len,expected,&parsed);heap_caps_free(data);
  if(error==ESP_ERR_INVALID_STATE)return reply(r,"409 Conflict","{\"error\":\"catalog_changed\"}");
  if(error==ESP_ERR_INVALID_ARG)return report(r,"400 Bad Request",&parsed,false);
  return error==ESP_OK ? report(r,"200 OK",&parsed,true):reply(r,"503 Service Unavailable","{\"error\":\"playlist_storage_unavailable\"}");
}
static esp_err_t station_select(httpd_req_t *r){
  if(!request_allowed(r)||!catalog_access(r))return ESP_FAIL;
  uint64_t expected;
  if(!generation(r,&expected))return reply(r,"400 Bad Request","{\"error\":\"catalog_generation_required\"}");
  if(!content_type(r,"application/json"))return reply(r,"415 Unsupported Media Type","{\"error\":\"json_required\"}");
  uint8_t *data=body(r,64);if(!data)return ESP_FAIL;
  cJSON *root=parse_json(data, r->content_len);
  cJSON *value=root ? cJSON_GetObjectItemCaseSensitive(root,"station_id"):NULL;
  bool valid=cJSON_IsObject(root)&&cJSON_GetArraySize(root)==1&&cJSON_IsString(value)&&strlen(value->valuestring)==16;
  uint64_t id=0;
  if(valid)for(unsigned i=0;i<16;++i){
    unsigned char c=(unsigned char)value->valuestring[i];unsigned digit;
    if(c>='0'&&c<='9')digit=c-'0';else if(c>='a'&&c<='f')digit=c-'a'+10;else{valid=false;break;}
    id=(id<<4)|digit;
  }
  cJSON_Delete(root);heap_caps_free(data);
  if(!valid||!id)return reply(r,"400 Bad Request","{\"error\":\"invalid_station\"}");
  esp_err_t error=actions.station_select(id,expected);
  return error==ESP_OK ? reply(r,"202 Accepted","{\"accepted\":true}"):
      reply(r,"409 Conflict","{\"error\":\"catalog_changed\"}");
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
  const httpd_uri_t player_route={.uri="/api/v1/player/control",.method=HTTP_POST,.handler=player_post};
  esp_err_t player_error=httpd_register_uri_handler(server,&player_route);
  if(player_error!=ESP_OK)return player_error;
  const httpd_uri_t catalog_routes[]={
    {.uri="/api/v1/stations",.method=HTTP_GET,.handler=catalog_get},
    {.uri="/api/v1/stations/export",.method=HTTP_GET,.handler=catalog_export},
    {.uri="/api/v1/stations/replace",.method=HTTP_POST,.handler=catalog_replace},
    {.uri="/api/v1/stations/select",.method=HTTP_POST,.handler=station_select}};
  for(unsigned i=0;i<4;++i){esp_err_t e=httpd_register_uri_handler(server,&catalog_routes[i]);if(e!=ESP_OK)return e;}
  const httpd_uri_t settings_routes[]={
    {.uri="/api/v1/settings",.method=HTTP_GET,.handler=settings_get},
    {.uri="/api/v1/settings",.method=HTTP_POST,.handler=settings_post}};
  for(unsigned i=0;i<2;++i){esp_err_t e=httpd_register_uri_handler(server,&settings_routes[i]);if(e!=ESP_OK)return e;}
  serving = true;
  return ESP_OK;
}
