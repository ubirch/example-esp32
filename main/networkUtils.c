
#include <string.h>

#include <esp_log.h>
#include <mbedtls/entropy.h>
#include <mbedtls/ctr_drbg.h>
#include <mbedtls/ssl.h>
#include <mbedtls/net.h>
#include <mbedtls/error.h>
#include <lwip/sockets.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#include "networkUtils.h"


/* Root cert for howsmyssl.com, taken from server_root_cert.pem

   The PEM file was extracted from the output of this command:
   openssl s_client -showcerts -connect www.howsmyssl.com:443 </dev/null

   The CA root cert is the last cert given in the chain of certs.

   To embed it in the app binary, the PEM file is named
   in the component.mk COMPONENT_EMBED_TXTFILES variable.
*/
//extern const uint8_t TestCA_crt_pem_start[] asm("_binary_TestCA_crt_pem_start");
//extern const uint8_t TestCA_crt_pem_end[]   asm("_binary_TestCA_crt_pem_end");
extern const uint8_t TestCA_crt_pem_start[] asm("_binary_ubirchdemoubirchcom_crt_pem_start");
extern const uint8_t TestCA_crt_pem_end[]   asm("_binary_ubirchdemoubirchcom_crt_pem_end");

extern EventGroupHandle_t wifi_event_group;
extern bool connected_with_IP;
extern const int CONNECTED_BIT;


/*
 * HTTPS request
 */
void https_get_task(void *pvParameters)
{
//    char *TAG = "https";
//    ESP_LOGI(TAG, "https_get_task");
//
////    sprintf(TAG,"https");
////    strcpy(TAG,"https");
//    char buf[512];
//    int ret, flags, len;
//
//    mbedtls_entropy_context entropy;
//    mbedtls_ctr_drbg_context ctr_drbg;
//    mbedtls_ssl_context ssl;
//    mbedtls_x509_crt cacert;
//    mbedtls_ssl_config conf;
//    mbedtls_net_context server_fd;
//
//    mbedtls_ssl_init(&ssl);
//    mbedtls_x509_crt_init(&cacert);
//    mbedtls_ctr_drbg_init(&ctr_drbg);
//    ESP_LOGI(TAG, "Seeding the random number generator");
//
//    mbedtls_ssl_config_init(&conf);
//
//    mbedtls_entropy_init(&entropy);
//    if((ret = mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy,
//                                    NULL, 0)) != 0)
//    {
//        ESP_LOGE(TAG, "mbedtls_ctr_drbg_seed returned %d", ret);
//        abort();
//    }
//
//    ESP_LOGI(TAG, "Loading the CA root certificate...");
//
//    ret = mbedtls_x509_crt_parse(&cacert, TestCA_crt_pem_start,
//                                 TestCA_crt_pem_end-TestCA_crt_pem_start);
//
//    if(ret < 0)
//    {
//        ESP_LOGE(TAG, "mbedtls_x509_crt_parse returned -0x%x\n\n", -ret);
//        abort();
//    }
//
//    ESP_LOGI(TAG, "Setting hostname for TLS session...");
//
//    /* Hostname set here should match CN in server certificate */
//    if((ret = mbedtls_ssl_set_hostname(&ssl, UHTTP_HOST)) != 0)
//    {
//        ESP_LOGE(TAG, "mbedtls_ssl_set_hostname returned -0x%x", -ret);
//        abort();
//    }
//
//    ESP_LOGI(TAG, "Setting up the SSL/TLS structure...");
//
//    if((ret = mbedtls_ssl_config_defaults(&conf,
//                                          MBEDTLS_SSL_IS_CLIENT,
//                                          MBEDTLS_SSL_TRANSPORT_STREAM,
//                                          MBEDTLS_SSL_PRESET_DEFAULT)) != 0)
//    {
//        ESP_LOGE(TAG, "mbedtls_ssl_config_defaults returned %d", ret);
//        goto exit;
//    }
//
//    /* MBEDTLS_SSL_VERIFY_OPTIONAL is bad for security, in this example it will print
//       a warning if CA verification fails but it will continue to connect.
//
//       You should consider using MBEDTLS_SSL_VERIFY_REQUIRED in your own code.
//    */
//    mbedtls_ssl_conf_authmode(&conf, MBEDTLS_SSL_VERIFY_OPTIONAL);
//    mbedtls_ssl_conf_ca_chain(&conf, &cacert, NULL);
//    mbedtls_ssl_conf_rng(&conf, mbedtls_ctr_drbg_random, &ctr_drbg);
//#ifdef CONFIG_MBEDTLS_DEBUG
//    mbedtls_esp_enable_debug_log(&conf, 4);
//#endif
//
//    if ((ret = mbedtls_ssl_setup(&ssl, &conf)) != 0)
//    {
//        ESP_LOGE(TAG, "mbedtls_ssl_setup returned -0x%x\n\n", -ret);
//        goto exit;
//    }
//
//    while(1) {
//        /* Wait for the callback to set the CONNECTED_BIT in the
//           event group.
//        */
//        xEventGroupWaitBits(wifi_event_group, CONNECTED_BIT,
//                            false, true, portMAX_DELAY);
//        ESP_LOGI(TAG, "Connected to AP");
//
//        mbedtls_net_init(&server_fd);
//
//        ESP_LOGI(TAG, "Connecting to %s:%s...", UHTTP_HOST, WEB_PORT);
//
//        if ((ret = mbedtls_net_connect(&server_fd, UHTTP_HOST,
//                                       WEB_PORT, MBEDTLS_NET_PROTO_TCP)) != 0)
//        {
//            ESP_LOGE(TAG, "mbedtls_net_connect returned -%x", -ret);
//            goto exit;
//        }
//
//        ESP_LOGI(TAG, "Connected.");
//
//        mbedtls_ssl_set_bio(&ssl, &server_fd, mbedtls_net_send, mbedtls_net_recv, NULL);
//
//        ESP_LOGI(TAG, "Performing the SSL/TLS handshake...");
//
////        while ((ret = mbedtls_ssl_handshake(&ssl)) != 0)
////        {
////            if (ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE)
////            {
////                ESP_LOGE(TAG, "mbedtls_ssl_handshake returned -0x%x", -ret);
////                goto exit;
////            }
////        }
//
////        ESP_LOGI(TAG, "Verifying peer X.509 certificate...");
////
////        if ((flags = mbedtls_ssl_get_verify_result(&ssl)) != 0)
////        {
////            /* In real life, we probably want to close connection if ret != 0 */
////            ESP_LOGW(TAG, "Failed to verify peer certificate!");
////            bzero(buf, sizeof(buf));
////            mbedtls_x509_crt_verify_info(buf, sizeof(buf), "  ! ", flags);
////            ESP_LOGW(TAG, "verification info: %s", buf);
////        }
////        else {
////            ESP_LOGI(TAG, "Certificate verified.");
////        }
//
////        ESP_LOGI(TAG, "Cipher suite is %s", mbedtls_ssl_get_ciphersuite(&ssl));
//
//        ESP_LOGI(TAG, "Writing HTTP request...");
//
//        size_t written_bytes = 0;
//        do {
//            ret = mbedtls_ssl_write(&ssl,
//                                    (const unsigned char *)REQUEST + written_bytes,
//                                    strlen(REQUEST) - written_bytes);
//            if (ret >= 0) {
//                ESP_LOGI(TAG, "%d bytes written", ret);
//                written_bytes += ret;
//            } else if (ret != MBEDTLS_ERR_SSL_WANT_WRITE && ret != MBEDTLS_ERR_SSL_WANT_READ) {
//                ESP_LOGE(TAG, "mbedtls_ssl_write returned -0x%x", -ret);
//                goto exit;
//            }
//        } while(written_bytes < strlen(REQUEST));
//
//        ESP_LOGI(TAG, "Reading HTTP response...");
//
//        do
//        {
//            len = sizeof(buf) - 1;
//            bzero(buf, sizeof(buf));
//            ret = mbedtls_ssl_read(&ssl, (unsigned char *)buf, len);
//
//            if(ret == MBEDTLS_ERR_SSL_WANT_READ || ret == MBEDTLS_ERR_SSL_WANT_WRITE)
//                continue;
//
//            if(ret == MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY) {
//                ret = 0;
//                break;
//            }
//
//            if(ret < 0)
//            {
//                ESP_LOGE(TAG, "mbedtls_ssl_read returned -0x%x", -ret);
//                break;
//            }
//
//            if(ret == 0)
//            {
//                ESP_LOGI(TAG, "connection closed");
//                break;
//            }
//
//            len = ret;
//            ESP_LOGD(TAG, "%d bytes read", len);
//            /* Print response directly to stdout as it is read */
//            for(int i = 0; i < len; i++) {
//                putchar(buf[i]);
//            }
//        } while(1);
//
//        mbedtls_ssl_close_notify(&ssl);
//
//        exit:
//        mbedtls_ssl_session_reset(&ssl);
//        mbedtls_net_free(&server_fd);
//
//        if(ret != 0)
//        {
//            mbedtls_strerror(ret, buf, 100);
//            ESP_LOGE(TAG, "Last error was: -0x%x - %s", -ret, buf);
//        }
//
//        putchar('\n'); // JSON output doesn't have a newline at end
//
//        static int request_count;
//        ESP_LOGI(TAG, "Completed %d requests", ++request_count);
//
//        for(int countdown = 10; countdown >= 0; countdown--) {
//            ESP_LOGI(TAG, "%d...", countdown);
//            vTaskDelay(1000 / portTICK_PERIOD_MS);
//        }
//        ESP_LOGI(TAG, "Starting again!");
//    }
}


/*
 * Send a TCP message via socket
 */
void tcp_client_send_msg(char *msg, uint16_t msg_length) {
    while(!connected_with_IP){
        //wait here
    }
    int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(sock < 0)
        printf("ERROR could not create socket \r\n");
    else
        printf("socket created \r\n");

    struct sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    inet_pton(AF_INET, "192.168.150.110", &serverAddress.sin_addr.s_addr);
    serverAddress.sin_port = htons(9999);

    if(connect(sock, (struct sockaddr *)&serverAddress, sizeof(struct sockaddr_in)) < 0)
        printf("ERROR socket connect,  \r\n");
    else {
        printf("SOCKET connected \r\n");

        if (send(sock, msg, msg_length, 0) < 0)
            printf("ERROR sending %s\r\n", msg);
        else
            printf("SENT %s\r\n", msg);
    }
    close(sock);
}
