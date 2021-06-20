//
// Copyright 2020 dushin.net
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
#include <esp_wifi.h>
#include <esp_event_loop.h>
#include <esp_log.h>
#include <esp_system.h>
#include <nvs_flash.h>
#include <sys/param.h>

#include <esp_http_server.h>

#include <esp_camera.h>
#include <driver/gpio.h>
#include <esp_timer.h>
#include <esp_system.h>
#include <nvs.h>
#include <nvs_flash.h>


// //WROVER-KIT PIN Map
// #define CAM_PIN_PWDN    -1 //power down is not used
// #define CAM_PIN_RESET   -1 //software reset will be performed
// #define CAM_PIN_XCLK    21
// #define CAM_PIN_SIOD    26
// #define CAM_PIN_SIOC    27

// #define CAM_PIN_D7      35
// #define CAM_PIN_D6      34
// #define CAM_PIN_D5      39
// #define CAM_PIN_D4      36
// #define CAM_PIN_D3      19
// #define CAM_PIN_D2      18
// #define CAM_PIN_D1       5
// #define CAM_PIN_D0       4
// #define CAM_PIN_VSYNC   25
// #define CAM_PIN_HREF    23
// #define CAM_PIN_PCLK    22

// AI-THINKER
#define CAM_PIN_PWDN     32
#define CAM_PIN_RESET    -1
#define CAM_PIN_XCLK      0
#define CAM_PIN_SIOD     26
#define CAM_PIN_SIOC     27

#define CAM_PIN_D7       35
#define CAM_PIN_D6       34
#define CAM_PIN_D5       39
#define CAM_PIN_D4       36
#define CAM_PIN_D3       21
#define CAM_PIN_D2       19
#define CAM_PIN_D1       18
#define CAM_PIN_D0        5
#define CAM_PIN_VSYNC    25
#define CAM_PIN_HREF     23
#define CAM_PIN_PCLK     22

#define EXAMPLE_WIFI_SSID CONFIG_WIFI_SSID
#define EXAMPLE_WIFI_PASS CONFIG_WIFI_PASSWORD

static const char *TAG = "esp32cam";

static void write_string(const char *key, const char *value);
static char *read_string(const char *key);
static void reset();
static void reboot();


static camera_config_t camera_config = {
    .pin_pwdn  = CAM_PIN_PWDN,
    .pin_reset = CAM_PIN_RESET,
    .pin_xclk = CAM_PIN_XCLK,
    .pin_sscb_sda = CAM_PIN_SIOD,
    .pin_sscb_scl = CAM_PIN_SIOC,

    .pin_d7 = CAM_PIN_D7,
    .pin_d6 = CAM_PIN_D6,
    .pin_d5 = CAM_PIN_D5,
    .pin_d4 = CAM_PIN_D4,
    .pin_d3 = CAM_PIN_D3,
    .pin_d2 = CAM_PIN_D2,
    .pin_d1 = CAM_PIN_D1,
    .pin_d0 = CAM_PIN_D0,
    .pin_vsync = CAM_PIN_VSYNC,
    .pin_href = CAM_PIN_HREF,
    .pin_pclk = CAM_PIN_PCLK,

    //XCLK 20MHz or 10MHz for OV2640 double FPS (Experimental)
    .xclk_freq_hz = 20000000,
    .ledc_timer = LEDC_TIMER_0,
    .ledc_channel = LEDC_CHANNEL_0,

    .pixel_format = PIXFORMAT_JPEG,//YUV422,GRAYSCALE,RGB565,JPEG
    .frame_size = FRAMESIZE_UXGA,//QQVGA-QXGA Do not use sizes above QVGA when not JPEG

    .jpeg_quality = 12, //0-63 lower number means higher quality
    .fb_count = 1 //if more than one, i2s runs in continuous mode. Use only with JPEG
};

esp_err_t capture_httpd_handler(httpd_req_t *req)
{
    camera_fb_t * fb = NULL;
    esp_err_t res = ESP_OK;
    size_t fb_len = 0;
    int64_t fr_start = esp_timer_get_time();

    char *flash = read_string("flash");
    int do_flash = flash != NULL && strcmp(flash, "on") == 0;
    free(flash);
    size_t buf_len = httpd_req_get_url_query_len(req) + 1;
    if (buf_len > 1) {
        char *buf = malloc(buf_len);
        if (httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK) {
            ESP_LOGI(TAG, "Found URL query => %s", buf);
            char param[32];
            /* Get value of expected key from query string */
            if (httpd_query_key_value(buf, "flash", param, sizeof(param)) == ESP_OK) {
                ESP_LOGI(TAG, "Found URL query parameter => flash=%s", param);
                if (strcmp(param, "on") == 0) {
                    ESP_LOGI(TAG, "Setting flash on");
                    do_flash  = 1;
                }
            }
        }
        free(buf);
    }

    if (do_flash) {
        ESP_LOGI(TAG, "Camera enabling flash");
        gpio_set_direction(4, GPIO_MODE_OUTPUT);
        gpio_set_level(4, 1);
    }

    fb = esp_camera_fb_get();

    if (do_flash) {
        gpio_set_level(4, 0);
    }

    if (!fb) {
        ESP_LOGE(TAG, "Camera capture failed");
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }
    res = httpd_resp_set_type(req, "image/jpeg");
    if (res == ESP_OK){
        res = httpd_resp_set_hdr(req, "Content-Disposition", "inline; filename=capture.jpg");
    }

    if (res == ESP_OK){
        if (fb->format == PIXFORMAT_JPEG){
            fb_len = fb->len;
            res = httpd_resp_send(req, (const char *)fb->buf, fb->len);
        } else {
            ESP_LOGE(TAG, "Unsupported format: 0x%x", fb->format);
            res = ESP_FAIL;
        }
    }
    esp_camera_fb_return(fb);
    int64_t fr_end = esp_timer_get_time();
    ESP_LOGI(TAG, "JPG: %uKB %ums", (uint32_t)(fb_len/1024), (uint32_t)((fr_end - fr_start)/1000));
    return res;
}

httpd_uri_t capture = {
    .uri       = "/capture",
    .method    = HTTP_GET,
    .handler   = capture_httpd_handler
};

esp_err_t config_wifi_post_httpd_handler(httpd_req_t *req)
{
    size_t buf_len = httpd_req_get_url_query_len(req) + 1;
    if (buf_len > 1) {
        char *buf = malloc(buf_len);
        if (httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK) {
            ESP_LOGI(TAG, "Found URL query => %s", buf);
            char param[32];
            /* Get value of expected key from query string */
            if (httpd_query_key_value(buf, "mode", param, sizeof(param)) == ESP_OK) {
                ESP_LOGI(TAG, "Found URL query parameter => mode=%s", param);
                if (strcmp(param, "STA") == 0) {
                    ESP_LOGI(TAG, "Setting STA mode");
                    write_string("wifi_mode", "STA");
                } else if (strcmp(param, "AP") == 0) {
                    ESP_LOGI(TAG, "Setting AP mode");
                    write_string("wifi_mode", "AP");
                } else {
                    ESP_LOGE(TAG, "Invalid mode => mode=%s", param);
                    httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid mode:  Must be STA or AP");
                    free(buf);
                    return ESP_FAIL;
                }
            }
            if (httpd_query_key_value(buf, "ssid", param, sizeof(param)) == ESP_OK) {
                ESP_LOGI(TAG, "Found URL query parameter => ssid=%s", param);
                write_string("wifi_ssid", param);
            }
            if (httpd_query_key_value(buf, "password", param, sizeof(param)) == ESP_OK) {
                //ESP_LOGI(TAG, "Found URL query parameter => password=%s", param);
                write_string("wifi_password", param);
            }
            if (httpd_query_key_value(buf, "device_name", param, sizeof(param)) == ESP_OK) {
                ESP_LOGI(TAG, "Found URL query parameter => device_name=%s", param);
                write_string("device_name", param);
            }
        }
        free(buf);
    }
    httpd_resp_send(req, "ok", 2);
    return ESP_OK;
}

httpd_uri_t post_config_wifi = {
    .uri       = "/config/wifi",
    .method    = HTTP_POST,
    .handler   = config_wifi_post_httpd_handler,
    .user_ctx  = NULL
};

esp_err_t config_wifi_get_httpd_handler(httpd_req_t *req)
{
    size_t buf_len = httpd_req_get_url_query_len(req) + 1;
    if (buf_len > 1) {
        char *buf = malloc(buf_len);
        if (httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK) {
            ESP_LOGI(TAG, "Found URL query => %s", buf);
            char param[32];
            /* Get value of expected key from query string */
            if (httpd_query_key_value(buf, "param", param, sizeof(param)) == ESP_OK) {
                ESP_LOGI(TAG, "Found URL query parameter => param=%s", param);
                if (strcmp(param, "mode") == 0) {
                    ESP_LOGI(TAG, "Getting wifi mode");
                    char *param = read_string("wifi_mode");
                    char *response = param == NULL ? "undefined" : param;
                    httpd_resp_send(req, response, strlen(response));
                    free(param);
                } else if (strcmp(param, "ssid") == 0) {
                    ESP_LOGI(TAG, "Getting wifi ssid");
                    char *param = read_string("wifi_ssid");
                    char *response = param == NULL ? "undefined" : param;
                    httpd_resp_send(req, response, strlen(response));
                    free(param);
                } else if (strcmp(param, "device_name") == 0) {
                    ESP_LOGI(TAG, "Getting device_name");
                    char *param = read_string("device_name");
                    char *response = param == NULL ? "undefined" : param;
                    httpd_resp_send(req, response, strlen(response));
                    free(param);
                } else {
                    ESP_LOGE(TAG, "Invalid param => param=%s", param);
                    httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid param");
                    free(buf);
                    return ESP_FAIL;
                }
            }
        }
        free(buf);
    }
    return ESP_OK;
}

httpd_uri_t get_config_wifi = {
    .uri       = "/config/wifi",
    .method    = HTTP_GET,
    .handler   = config_wifi_get_httpd_handler,
    .user_ctx  = NULL
};

esp_err_t config_camera_post_httpd_handler(httpd_req_t *req)
{
    size_t buf_len = httpd_req_get_url_query_len(req) + 1;
    if (buf_len > 1) {
        char *buf = malloc(buf_len);
        if (httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK) {
            ESP_LOGI(TAG, "Found URL query => %s", buf);
            char param[32];
            /* Get value of expected key from query string */
            if (httpd_query_key_value(buf, "frame_size", param, sizeof(param)) == ESP_OK) {
                ESP_LOGI(TAG, "Found URL query parameter => mode=%s", param);
                // QVGA|CIF|VGA|SVGA|XGA|SXGA|UXGA
                if (
                    strcmp(param, "QVGA") == 0 || strcmp(param, "CIF") == 0
                    || strcmp(param, "VGA") == 0 || strcmp(param, "SVGA") == 0
                    || strcmp(param, "XGA") == 0
                    || strcmp(param, "SXGA") == 0 || strcmp(param, "UXGA") == 0
                ) {
                    ESP_LOGI(TAG, "Setting frame_size");
                    write_string("frame_size", param);
                } else {
                    ESP_LOGE(TAG, "Invalid frame_size => frame_size=%s", param);
                    httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid frame_size:  Must be QVGA|CIF|VGA|SVGA|XGA|SXGA|UXGA");
                    free(buf);
                    return ESP_FAIL;
                }
            }
            if (httpd_query_key_value(buf, "jpeg_quality", param, sizeof(param)) == ESP_OK) {
                ESP_LOGI(TAG, "Found URL query parameter => jpeg_quality=%s", param);
                int jpeg_quality = atoi(param);
                if (jpeg_quality < 1 || jpeg_quality > 63) {
                    ESP_LOGE(TAG, "Invalid jpeg_quality => jpeg_quality=%s", param);
                    httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid jpeg_quality:  Must be between 1 and 63");
                    free(buf);
                    return ESP_FAIL;
                } else {
                    write_string("jpeg_quality", param);
                }
            }
            if (httpd_query_key_value(buf, "flash", param, sizeof(param)) == ESP_OK) {
                ESP_LOGI(TAG, "Found URL query parameter => flash=%s", param);
                if (strcmp(param, "on") == 0) {
                    ESP_LOGI(TAG, "Setting flash on");
                    write_string("flash", "on");
                } else if (strcmp(param, "off") == 0) {
                    ESP_LOGI(TAG, "Setting flash off");
                    write_string("flash", "off");
                } else {
                    ESP_LOGE(TAG, "Invalid flash => flash=%s", param);
                    httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid flash:  Must be between on or off");
                    free(buf);
                    return ESP_FAIL;
                }
            }
        }
        free(buf);
    }
    httpd_resp_send(req, "ok", 2);
    return ESP_OK;
}

httpd_uri_t post_config_camera = {
    .uri       = "/config/camera",
    .method    = HTTP_POST,
    .handler   = config_camera_post_httpd_handler,
    .user_ctx  = NULL
};

esp_err_t config_camera_get_httpd_handler(httpd_req_t *req)
{
    size_t buf_len = httpd_req_get_url_query_len(req) + 1;
    if (buf_len > 1) {
        char *buf = malloc(buf_len);
        if (httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK) {
            ESP_LOGI(TAG, "Found URL query => %s", buf);
            char param[32];
            /* Get value of expected key from query string */
            if (httpd_query_key_value(buf, "param", param, sizeof(param)) == ESP_OK) {
                ESP_LOGI(TAG, "Found URL query parameter => param=%s", param);
                if (strcmp(param, "frame_size") == 0) {
                    ESP_LOGI(TAG, "Getting frame_size");
                    char *param = read_string("frame_size");
                    char *response = param == NULL ? "undefined" : param;
                    httpd_resp_send(req, response, strlen(response));
                    free(param);
                } else if (strcmp(param, "jpeg_quality") == 0) {
                    ESP_LOGI(TAG, "Getting jpeg_quality");
                    char *param = read_string("jpeg_quality");
                    char *response = param == NULL ? "undefined" : param;
                    httpd_resp_send(req, response, strlen(response));
                    free(param);
                } else if (strcmp(param, "flash") == 0) {
                    ESP_LOGI(TAG, "Getting flash");
                    char *param = read_string("flash");
                    char *response = param == NULL ? "undefined" : param;
                    httpd_resp_send(req, response, strlen(response));
                    free(param);
                } else {
                    ESP_LOGE(TAG, "Shit Invalid param => param=%s", param);
                    httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid param");
                    free(buf);
                    return ESP_FAIL;
                }
            }
        }
        free(buf);
    }
    return ESP_OK;
}

httpd_uri_t get_config_camera = {
    .uri       = "/config/camera",
    .method    = HTTP_GET,
    .handler   = config_camera_get_httpd_handler,
    .user_ctx  = NULL
};

esp_err_t reset_httpd_handler(httpd_req_t *req)
{
    reset();
    httpd_resp_send(req, "ok", 2);
    return ESP_OK;
}

httpd_uri_t post_reset = {
    .uri       = "/reset",
    .method    = HTTP_POST,
    .handler   = reset_httpd_handler,
    .user_ctx  = NULL
};

esp_err_t reboot_httpd_handler(httpd_req_t *req)
{
    reboot();
    httpd_resp_send(req, "ok", 2);
    return ESP_OK;
}

httpd_uri_t post_reboot = {
    .uri       = "/reboot",
    .method    = HTTP_POST,
    .handler   = reboot_httpd_handler,
    .user_ctx  = NULL
};

httpd_handle_t start_webserver(void)
{
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    // Start the httpd server
    ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);
    if (httpd_start(&server, &config) == ESP_OK) {
        // Set URI handlers
        ESP_LOGI(TAG, "Registering URI handlers");
        httpd_register_uri_handler(server, &capture);
        httpd_register_uri_handler(server, &post_config_wifi);
        httpd_register_uri_handler(server, &get_config_wifi);
        httpd_register_uri_handler(server, &post_config_camera);
        httpd_register_uri_handler(server, &get_config_camera);
        httpd_register_uri_handler(server, &post_reset);
        httpd_register_uri_handler(server, &post_reboot);
        return server;
    }

    ESP_LOGI(TAG, "Error starting server!");
    return NULL;
}

void stop_webserver(httpd_handle_t server)
{
    // Stop the httpd server
    httpd_stop(server);
}

static uint8_t connection_failues = 0;

static esp_err_t event_handler(void *ctx, system_event_t *event)
{
    httpd_handle_t *server = (httpd_handle_t *) ctx;

    switch(event->event_id) {
        case SYSTEM_EVENT_STA_START:
            ESP_LOGI(TAG, "SYSTEM_EVENT_STA_START");
            ESP_ERROR_CHECK(esp_wifi_connect());
            break;
        case SYSTEM_EVENT_STA_GOT_IP:
            ESP_LOGI(TAG, "SYSTEM_EVENT_STA_GOT_IP");
            ESP_LOGI(TAG, "Got IP: '%s'",
                    ip4addr_ntoa(&event->event_info.got_ip.ip_info.ip));

            /* Start the web server */
            if (*server == NULL) {
                *server = start_webserver();
            }
            gpio_set_level(33, 0);

            break;
        case SYSTEM_EVENT_STA_CONNECTED:
            ESP_LOGI(TAG, "SYSTEM_EVENT_STA_CONNECTED");
            connection_failues = 0;
            break;
        case SYSTEM_EVENT_STA_DISCONNECTED:
            ESP_LOGI(TAG, "SYSTEM_EVENT_STA_DISCONNECTED");
            connection_failues++;
            if (connection_failues > 10) {
                ESP_LOGE(TAG, "Too many connection failures.  Falling back to AP mode.");
                write_string("wifi_mode", "fallback");
                reboot();
            } else {
                ESP_ERROR_CHECK(esp_wifi_connect());
            }

            /* Stop the web server */
            if (*server) {
                stop_webserver(*server);
                *server = NULL;
            }
            break;
        case SYSTEM_EVENT_AP_START:
            ESP_LOGI(TAG, "SYSTEM_EVENT_AP_START");

            /* Start the web server */
            if (*server == NULL) {
                *server = start_webserver();
            }
            gpio_set_level(33, 0);
            break;
        case SYSTEM_EVENT_AP_STOP:
            ESP_LOGI(TAG, "SYSTEM_EVENT_AP_STOP");

            /* Stop the web server */
            if (*server) {
                stop_webserver(*server);
                *server = NULL;
            }
        default:
            ESP_LOGI(TAG, "event_handler: event id: %d", event->event_id);
            break;
    }
    return ESP_OK;
}


static void reset()
{
    nvs_flash_erase();
}

static void reboot()
{
    esp_restart();
}

static void write_string(const char *key, const char *value)
{
    nvs_handle nvs;
    esp_err_t err;
    err = nvs_open(TAG, NVS_READWRITE, &nvs);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open nvs 0x%x", err);
        return;
    }

    err = nvs_set_str(nvs, key, value);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set string 0x%x", err);
        nvs_close(nvs);
        return;
    }

    nvs_close(nvs);
}

static char *read_string(const char *key)
{
    nvs_handle nvs;
    esp_err_t err;
    err = nvs_open(TAG, NVS_READWRITE, &nvs);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open nvs 0x%x", err);
        return NULL;
    }
    size_t len;
    err = nvs_get_str(nvs, key, NULL, &len);
    if (err != ESP_OK) {
        if (err != ESP_ERR_NVS_NOT_FOUND) {
            ESP_LOGE(TAG, "Failed to get string len 0x%x", err);
        }
        nvs_close(nvs);
        return NULL;
    }
    char *buf = malloc(len + 1);
    if (buf == NULL) {
        ESP_LOGE(TAG, "Failed to allocate string len 0x%x", err);
        nvs_close(nvs);
        return NULL;
    }
    err = nvs_get_str(nvs, key, buf, &len);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get string 0x%x", err);
        nvs_close(nvs);
        return NULL;
    }
    nvs_close(nvs);
    return buf;
}

static void set_dhcp_hostname(const char *dhcp_hostname)
{
    esp_err_t err = tcpip_adapter_set_hostname(WIFI_IF_STA, dhcp_hostname);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "DHCP hostname set to %s", dhcp_hostname);
    } else {
        ESP_LOGW(TAG, "Unable to set DHCP hostname to %s.  err=0x%x", dhcp_hostname, err);
    }
}

static char *get_default_device_name()
{
    uint8_t mac[6];
    esp_efuse_mac_get_default(mac);

    size_t tag_len = strlen(TAG);
    char *buf = malloc(tag_len + 12 + 1);
    if (buf == NULL) {
        ESP_LOGE(TAG, "Failed to allocate buf len 0x%x", tag_len + 12 + 1);
        return NULL;
    }
    snprintf(buf, tag_len + 12,
        "%s-%02x%02x%02x%02x%02x%02x", TAG, mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]
    );
    return buf;
}


static char *get_wifi_mode()
{
    char *ret = read_string("wifi_mode");
    if (ret == NULL) {
        ret = malloc(3);
        strcpy(ret, "AP");
    }
    return ret;
}


static void initialise_wifi(void *arg)
{
    tcpip_adapter_init();
    ESP_ERROR_CHECK(esp_event_loop_init(event_handler, arg));
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));

    wifi_config_t wifi_config;
    memset(&wifi_config, 0, sizeof(wifi_config_t));

    gpio_set_direction(33, GPIO_MODE_OUTPUT);
    gpio_set_level(33, 1);

    char *wifi_mode = get_wifi_mode();
    if (strcmp(wifi_mode, "fallback") == 0) {
        char *ssid = get_default_device_name();
        strcpy((char *) wifi_config.ap.ssid, ssid);
        free(ssid);
        strcpy((char *) wifi_config.ap.password, TAG);
        wifi_config.ap.authmode = WIFI_AUTH_WPA_WPA2_PSK;
        wifi_config.ap.max_connection = 4;
        ESP_LOGI(TAG, "Setting WiFi configuration SSID %s...", wifi_config.ap.ssid);
        ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
        ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, &wifi_config));
        write_string("wifi_mode", "STA"); // for next reboot
    } else if (strcmp(wifi_mode, "AP") == 0) {
        char *ssid = read_string("wifi_ssid");
        if (ssid == NULL) {
            ssid = get_default_device_name();
        }
        strcpy((char *) wifi_config.ap.ssid, ssid);
        char *password = read_string("wifi_password");
        if (password != NULL) {
            strcpy((char *) wifi_config.ap.password, password);
        } else {
            strcpy((char *) wifi_config.ap.password, TAG);
        }
        wifi_config.ap.authmode = WIFI_AUTH_WPA_WPA2_PSK;
        wifi_config.ap.max_connection = 4;
        ESP_LOGI(TAG, "Setting WiFi configuration SSID %s...", wifi_config.ap.ssid);
        ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
        ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, &wifi_config));
        free(ssid);
        free(password);
    } else {
        char *ssid = read_string("wifi_ssid");
        if (ssid == NULL) {
            ssid = get_default_device_name();
        }
        char *password = read_string("wifi_password");
        strcpy((char *) wifi_config.sta.ssid, ssid);
        if (password != NULL) {
            strcpy((char *) wifi_config.sta.password, password);
        } else {
            ESP_LOGW(TAG, "Assuming connection to open network!");
        }
        ESP_LOGI(TAG, "Setting WiFi configuration SSID %s...", wifi_config.sta.ssid);
        ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
        ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
        free(ssid);
        free(password);
    }

    ESP_ERROR_CHECK(esp_wifi_start());

    if (wifi_mode != NULL && strcmp(wifi_mode, "STA") == 0) {
        const char *device_name = read_string("device_name");
        if (device_name == NULL) {
            device_name = get_default_device_name();
        }
        set_dhcp_hostname(device_name);
    }
    free(wifi_mode);
}

esp_err_t initialize_camera()
{
    ESP_LOGI(TAG, "Initializing camera...");
    char *frame_size = read_string("frame_size");
    // QVGA|CIF|VGA|SVGA|XGA|SXGA|UXGA
    if (frame_size == NULL) {
        camera_config.frame_size = FRAMESIZE_UXGA;
    } else if (strcmp(frame_size, "QVGA") == 0) {
        camera_config.frame_size = FRAMESIZE_QVGA;
    } else if (strcmp(frame_size, "CIF") == 0) {
        camera_config.frame_size = FRAMESIZE_CIF;
    } else if (strcmp(frame_size, "VGA") == 0) {
        camera_config.frame_size = FRAMESIZE_VGA;
    } else if (strcmp(frame_size, "SVGA") == 0) {
        camera_config.frame_size = FRAMESIZE_SVGA;
    } else if (strcmp(frame_size, "XGA") == 0) {
        camera_config.frame_size = FRAMESIZE_XGA;
    } else if (strcmp(frame_size, "SXGA") == 0) {
        camera_config.frame_size = FRAMESIZE_SXGA;
    } else if (strcmp(frame_size, "UXGA") == 0) {
        camera_config.frame_size = FRAMESIZE_UXGA;
    }
    ESP_LOGI(TAG, "frame_size: %s", frame_size == NULL ? "UXGA" : frame_size);
    free(frame_size);

    char *jpeg_quality = read_string("jpeg_quality");
    if (jpeg_quality == NULL) {
        camera_config.jpeg_quality = 12;
    } else {
        camera_config.jpeg_quality = atoi(jpeg_quality);
    }
    ESP_LOGI(TAG, "jpeg_quality: %s", jpeg_quality == NULL ? "12" : jpeg_quality);
    free(jpeg_quality);

    //initialize the camera
    return esp_camera_init(&camera_config);
}

void app_main()
{
    static httpd_handle_t server = NULL;
    ESP_ERROR_CHECK(nvs_flash_init());

    //initialize the camera
    esp_err_t err = initialize_camera();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Camera Init Failed with error 0x%x", err);
        return;
    }

    initialise_wifi(&server);
}
