//
// Created by wowa on 21.05.19.
//
#include <stdio.h>
#include <string.h>
#include <sys/unistd.h>
#include <sys/stat.h>

#include <esp_log.h>
#include <util.h>
#include <esp_http_client.h>
#include <soc/rtc_wdt.h>
#include <storage.h>
#include "mbedtls/base64.h"

#include "../../components/cjson/include/cjson/cJSON.h"
#include "../../components/cjson/include/cjson/cJSON_Utils.h"

#include "settings.h"
#include "ubirch-protocol-c8y.h"

#define LOG_LOCAL_LEVEL ESP_LOG_DEBUG

static const char *TAG = "C8Y";

extern unsigned char UUID[16];

struct c8y_status {
	bool bootstrapped;
	bool registered;
	bool created;
	char *username;
	char *password;
	char *id;
} c8y_status;

esp_err_t c8y_load_status(void) {
	ESP_LOGI(TAG, "load status");
	esp_err_t err;

	bool *p_bool = &c8y_status.bootstrapped;
	size_t size = sizeof(bool);
	err = kv_load("c8y_storage", "c8y_boot", (void **) &p_bool, &size);
	if (memory_error_check(err)) return err;
	p_bool = &c8y_status.registered;
	err = kv_load("c8y_storage", "c8y_regi", (void **) &p_bool, &size);
	if (memory_error_check(err)) return err;
	p_bool = &c8y_status.created;
	err = kv_load("c8y_storage", "c8y_crea", (void **) &p_bool, &size);
	if (memory_error_check(err)) return err;
	size = 0;
	err = kv_load("c8y_storage", "c8y_user", (void **) &c8y_status.username, &size);
	if (memory_error_check(err)) return err;
	// todo somehow the username is not 0 teminated and breaks
	char *tmp = (char *) realloc(c8y_status.username, size + 1);
	if (tmp == NULL) {
		ESP_LOGE(__func__, "tmp == NULL");
		return ESP_FAIL;
	}
	c8y_status.username = tmp;
	c8y_status.username[size] = '\0';
	size = 0;
	err = kv_load("c8y_storage", "c8y_pass", (void **) &c8y_status.password, &size);
	if (memory_error_check(err)) return err;
	// todo somehow the password is not 0 teminated and breaks
	tmp = (char *) realloc(c8y_status.password, size + 1);
	if (tmp == NULL) {
		ESP_LOGE(__func__, "tmp == NULL");
		return ESP_FAIL;
	}
	c8y_status.password = tmp;
	c8y_status.password[size] = '\0';

	size = 0;
	err = kv_load("c8y_storage", "c8y_id", (void **) &c8y_status.id, &size);
	if (memory_error_check(err)) return err;
	tmp = (char *) realloc(c8y_status.id, size + 1);
	if (tmp == NULL) {
		ESP_LOGE(__func__, "tmp == NULL");
		return ESP_FAIL;
	}
	c8y_status.id = tmp;
	c8y_status.id[size] = '\0';

	ESP_LOGD("LOAD", "user: %s, pass: %s, id: %s", c8y_status.username, c8y_status.password, c8y_status.id);
	return err;
}

esp_err_t c8y_store_status(void) {
	ESP_LOGI(TAG, "store status");
	esp_err_t err;

	size_t size = sizeof(bool);
	err = kv_store("c8y_storage", "c8y_boot", (void *) &c8y_status.bootstrapped, size);
	if (memory_error_check(err)) return err;
	err = kv_store("c8y_storage", "c8y_regi", (void *) &c8y_status.registered, size);
	if (memory_error_check(err)) return err;
	err = kv_store("c8y_storage", "c8y_crea", (void *) &c8y_status.created, size);
	if (memory_error_check(err)) return err;
	size = strlen(c8y_status.username);
	err = kv_store("c8y_storage", "c8y_user", (void *) c8y_status.username, size);
	if (memory_error_check(err)) return err;
	size = strlen(c8y_status.password);
	err = kv_store("c8y_storage", "c8y_pass", (void *) c8y_status.password, size);
	if (memory_error_check(err)) return err;
	size = strlen(c8y_status.id);
	err = kv_store("c8y_storage", "c8y_id", (void *) c8y_status.id, size);
	if (memory_error_check(err)) return err;

	return err;
}


#define COUNTER_SIZE 0 //(sizeof(_c8y_atomic_counter_t))


static inline size_t c8y_buffer_capacity(const c8y_response *response) {
//	ESP_LOGD(__func__, "free: %d", response->free);
	return response->free;
}

static inline void c8y_buffer_consumed(c8y_response *response, size_t size) {
	response->used += size;
	response->free -= size;
}

static inline bool c8y_reserve_buffer(c8y_response *response, size_t size) {
	if (response->free >= size) {
		return true;
	}
	return c8y_expand_buffer(response, size);
}

static inline char *c8y_buffer(c8y_response *response) {
	return response->buffer + response->used;
}


c8y_response *c8y_response_new(size_t initial_buffer_size) {
	// allocate memory for new c8y_response
	c8y_response *response = (c8y_response *) malloc(sizeof(c8y_response));
	if (response == NULL) {
		ESP_LOGE(__func__, "response == NULL");
		return NULL;
	}

	// allocate memory for initial buffer
	if (!c8y_response_init(response, initial_buffer_size)) {
		free(response);
		return NULL;
	}
	return response;
}

bool c8y_response_init(c8y_response *response, size_t initial_buffer_size) {
	char *buffer = (char *) malloc(initial_buffer_size);
	if (buffer == NULL) {
		ESP_LOGE(__func__, "buffer == NULL");
		return false;
	}

	response->buffer = buffer;
	response->used = COUNTER_SIZE;
	response->free = initial_buffer_size - response->used;
	response->initial_buffer_size = initial_buffer_size;
	return true;
}


bool c8y_expand_buffer(c8y_response *response, size_t size) {
	if (response->used == 0) {
		// rewind buffer
		response->free += response->used - COUNTER_SIZE;
		response->used = COUNTER_SIZE;

		if (response->free >= size) {
			return true;
		}
	}

	size_t next_size = (response->used + response->free) * 2;  // include COUNTER_SIZE
	while (next_size < size + response->used) {
		next_size *= 2;
	}

	char *tmp = (char *) realloc(response->buffer, next_size);
	if (tmp == NULL) {
		ESP_LOGE(__func__, "tmp == NULL");
		return false;
	}

	response->buffer = tmp;
	response->free = next_size - response->used;
	return true;
}


void c8y_response_free(c8y_response *response) {
	free(response);
}


void c8y_get_authorization(char **auth) {
	char token[strlen(c8y_status.username) + strlen(c8y_status.password) + 1];
	strcpy(token, c8y_status.username);
	strcat(token, ":");
	strcat(token, c8y_status.password);
//	ESP_LOGD("TOKEN", "%s", token);
	unsigned char *token64 = NULL;
	size_t token_len;
	mbedtls_base64_encode(token64, 0, &token_len,
	                      (const unsigned char *) token, strlen(token));
	token64 = malloc(token_len);
	mbedtls_base64_encode(token64, token_len, &token_len,
	                      (const unsigned char *) token, strlen(token));

	*auth = malloc(token_len + 6);
	strcpy(*auth, "Basic "); // 6 char
	strcat(*auth, (const char *) token64);
//	ESP_LOGD("AUTHORIZATION","%s", *auth);
	free(token64);
}


/*!
 * Event handler for the ubirch response. Feeds response data into a msgpack unpacker to be parsed.
 *
 * @param evt, which calls this handler
 * @return error state if any or ESP_OK
 */
static esp_err_t _c8y_http_event_handler(esp_http_client_event_t *evt) {
	if (evt->event_id == HTTP_EVENT_ON_DATA) {
		ESP_LOGD(__func__, "HTTP received %d byte", evt->data_len);
		ESP_LOG_BUFFER_HEXDUMP(__func__, evt->data, (uint16_t) evt->data_len, ESP_LOG_DEBUG);

		// only feed data if the c8y_response is available
		if (evt->user_data != NULL) {
			c8y_response *response = evt->user_data;

			if (esp_http_client_is_chunked_response(evt->client)) {
//				ESP_LOG_BUFFER_HEXDUMP("CHUNKED", evt->data, (uint16_t) evt->data_len, ESP_LOG_DEBUG);
				if (c8y_buffer_capacity(response) < evt->data_len) {
					c8y_reserve_buffer(response, (uint16_t) evt->data_len);
				}
				memcpy(c8y_buffer(response), evt->data, (uint16_t) evt->data_len);
				c8y_buffer_consumed(response, (uint16_t) evt->data_len);
			} else {
				ESP_LOGW(__func__, "something went wrong");
			}
		}
	}
	return ESP_OK;
}


void c8y_http_client_init(const char *uuid) {
	// create a client json
	cJSON *client = cJSON_CreateObject();
	if (client == NULL) return;
	if (cJSON_AddStringToObject(client, "name", uuid) == NULL) return;
	cJSON *device = NULL;
	device = cJSON_AddObjectToObject(client, "c8y_IsDevice");
	if (device == NULL) return;
	cJSON *hardware = NULL;
	hardware = cJSON_AddObjectToObject(client, "c8y_Hardware");
	if (hardware == NULL) return;
	if (cJSON_AddStringToObject(hardware, "model", "ESP32D0WDQ6") == NULL) return;
	if (cJSON_AddStringToObject(hardware, "revision", "1.0") == NULL) return;
	if (cJSON_AddStringToObject(hardware, "serialNumber", uuid) == NULL) return;

	char *client_id = cJSON_PrintUnformatted(client);
	cJSON_Delete(client);
	ESP_LOGI(__func__, "%s", client_id);
}


esp_err_t c8y_terminate_string(c8y_response *response) {
	if (response == NULL)
		return ESP_FAIL;
	response->buffer[response->used] = '\0';
	response->used += 1;
	response->free -= 1;
	return ESP_OK;
}

/*
 * {
 *      "password":"1Ex1_StR1w",
 *      "tenantId":"ubirch",
 *      "self":"http://management.cumulocity.com/devicecontrol/deviceCredentials/30AEA423-4968-1223-3445-566778899AAB",
 *      "id":"30AEA423-4968-1223-3445-566778899AAB",
 *      "username":"device_30AEA423-4968-1223-3445-566778899AAB"
 * }
 * */

esp_err_t c8y_bootstrap_parse_response(c8y_response *response) {
	c8y_terminate_string(response);
	ESP_LOGD(__func__, "OK = %s\r\n", response->buffer);
	cJSON *json_response = cJSON_Parse(response->buffer);
	//get the password
	cJSON *temp_object = cJSON_GetObjectItemCaseSensitive(json_response, "password");
	if (cJSON_IsString(temp_object) && (temp_object->valuestring != NULL)) {
		char *c8y_password = malloc(strlen(temp_object->valuestring));
		strcpy(c8y_password, temp_object->valuestring);
		ESP_LOGD(__func__, "password: %s", c8y_password);
		c8y_status.password = c8y_password;
	} else
		return ESP_FAIL;
	//get the username
	cJSON *temp_object2 = cJSON_GetObjectItemCaseSensitive(json_response, "username");
	if (cJSON_IsString(temp_object2) && (temp_object2->valuestring != NULL)) {
		char *c8y_username = malloc(strlen(temp_object2->valuestring));
		strcpy(c8y_username, temp_object2->valuestring);
		ESP_LOGD(__func__, "username: %s", c8y_username);
		c8y_status.username = c8y_username;
	} else
		return ESP_FAIL;

	char *json_string_result = cJSON_Print(json_response);
	ESP_LOGD("JSON", ": %s", json_string_result);
	cJSON_Delete(json_response);
	free(json_string_result);
	return ESP_OK;
}


esp_err_t c8y_bootstrap_post(const char *url, const char *data, const size_t length, c8y_response *response) {
	ESP_LOGD(__func__, "url: %s, len: %d)", url, length);

	esp_http_client_config_t config = {
			.url = url,
			.event_handler = _c8y_http_event_handler,
			.user_data = response
	};
	esp_http_client_handle_t client = esp_http_client_init(&config);

	// POST
	esp_http_client_set_url(client, url);
	esp_http_client_set_method(client, HTTP_METHOD_POST);
	esp_http_client_set_header(client, "Authorization", AUTHORIZATION);
	esp_http_client_set_header(client, "Content-Type", "application/vnd.com.nsn.cumulocity.deviceCredentials+json");
	esp_http_client_set_header(client, "Accept", "application/vnd.com.nsn.cumulocity.deviceCredentials+json");

	cJSON *json = cJSON_CreateObject();
	cJSON_AddStringToObject(json, "id", data);
	char *json_string = cJSON_PrintUnformatted(json);
	ESP_LOGD(__func__, "JSON %s", json_string);
	cJSON_Delete(json);

	esp_http_client_set_post_field(client, json_string, (int) (strlen(json_string)));
	esp_err_t err = esp_http_client_perform(client);
	if (err == ESP_OK) {
		const int http_status = esp_http_client_get_status_code(client);
		const int content_length = esp_http_client_get_content_length(client);
		ESP_LOGI(__func__, "HTTP POST status = %d, content_length = %d", http_status, content_length);
		err = (http_status >= 200 && http_status <= 299) ? ESP_OK : ESP_FAIL;
	} else {
		ESP_LOGE(__func__, "HTTP POST request failed: %s", esp_err_to_name(err));
	}

	if (err == ESP_OK) {
		c8y_status.bootstrapped = true;
		c8y_bootstrap_parse_response(response);
	}

	esp_http_client_cleanup(client);
	return err;
}


void c8y_bootstrap() {
	ESP_LOGD(__func__, "");
	char *uuid = get_hw_ID_string();
	char url[70];  //todo can be improved, by making it dynamic
	strcpy(url, "https://"); // 8 char
	strcat(url, HOST); // 25 char
	strcat(url, "/devicecontrol/deviceCredentials"); // 32 char


	while (c8y_status.bootstrapped == false) {
		ESP_LOGI("C8Y", "device has no credentials yet");
		c8y_response *response = c8y_response_new(160);
		c8y_bootstrap_post(url, uuid, strlen(uuid), response);
		c8y_response_free(response);
		vTaskDelay(pdMS_TO_TICKS(3000));
	}
}

//############################################################################################
esp_err_t c8y_registered_parse_response(c8y_response *response) {
	c8y_terminate_string(response);
	ESP_LOGD(__func__, "OK = %s", response->buffer);
	cJSON *json_response = cJSON_Parse(response->buffer);

	char *json_string_result = cJSON_Print(json_response);
	ESP_LOGD("JSON", ": %s", json_string_result);
	cJSON_Delete(json_response);
	free(json_string_result);
	return ESP_OK;

}

esp_err_t c8y_registered_get(const char *url, const char *authorization, c8y_response *response) {
	ESP_LOGD(__func__, "url: %s)", url);

	esp_http_client_config_t config = {
			.url = url,
			.event_handler = _c8y_http_event_handler,
			.user_data = response
	};
	esp_http_client_handle_t client = esp_http_client_init(&config);

	// POST
	esp_http_client_set_url(client, url);
	esp_http_client_set_method(client, HTTP_METHOD_GET);
	esp_http_client_set_header(client, "Authorization", authorization);
	esp_http_client_set_header(client, "Accept", "application/vnd.com.nsn.cumulocity.externalId+json");

	esp_err_t err = esp_http_client_perform(client);
	if (err == ESP_OK) {
		const int http_status = esp_http_client_get_status_code(client);
		const int content_length = esp_http_client_get_content_length(client);
		ESP_LOGI(__func__, "HTTP GET status = %d, content_length = %d", http_status, content_length);
		err = (http_status >= 200 && http_status <= 299) ? ESP_OK : ESP_FAIL;
	} else {
		ESP_LOGE(__func__, "HTTP GET request failed: %s", esp_err_to_name(err));
	}

	if (err == ESP_OK) {
		c8y_status.registered = true;
		c8y_registered_parse_response(response);
//		while (1) {
//			// todo
//			if (rtc_wdt_is_on()) rtc_wdt_feed();
//		}
	}

	esp_http_client_cleanup(client);
	return err;
}

esp_err_t c8y_registered() {
	ESP_LOGD(__func__, "");
	char *uuid = get_hw_ID_string();
	char url[108];
	strcpy(url, "https://"); // 8 char
	strcat(url, TENANT); // 6 char
	strcat(url, ".cumulocity.com"); // 15 char
	strcat(url, "/identity/externalIds/c8y_Serial/"); // 33 char
	strcat(url, uuid); // 37 char

	char token[strlen(c8y_status.username) + strlen(c8y_status.password) + 1];
	strcpy(token, c8y_status.username);
	strcat(token, ":");
	strcat(token, c8y_status.password);

	ESP_LOGD("TOKEN", "%s", token);

	char *authorization = NULL;
	c8y_get_authorization(&authorization);

	c8y_response *response = c8y_response_new(160);
	esp_err_t err = c8y_registered_get(url, authorization, response);
	c8y_response_free(response);
	free(authorization);
	vTaskDelay(pdMS_TO_TICKS(3000));

	return err;
}

//################################################
esp_err_t c8y_create_parse_response(c8y_response *response) {
	c8y_terminate_string(response);
	ESP_LOGD(__func__, "OK = %s", response->buffer);
	cJSON *json_response = cJSON_Parse(response->buffer);
	//get the password
	cJSON *temp_object = cJSON_GetObjectItemCaseSensitive(json_response, "id");
	if (cJSON_IsString(temp_object) && (temp_object->valuestring != NULL)) {
		char *c8y_id = malloc(strlen(temp_object->valuestring));
		strcpy(c8y_id, temp_object->valuestring);
		ESP_LOGD(__func__, "id: %s", c8y_id);
		c8y_status.id = c8y_id;
	} else
		return ESP_FAIL;

	char *json_string_result = cJSON_Print(json_response);
	ESP_LOGD("JSON", ": %s", json_string_result);
	cJSON_Delete(json_response);
	free(json_string_result);
	return ESP_OK;
}

esp_err_t c8y_create_post(const char *url, const char *uuid, const char *authorization, c8y_response *response) {
	ESP_LOGD(__func__, "url: %s)", url);

	esp_http_client_config_t config = {
			.url = url,
			.event_handler = _c8y_http_event_handler,
			.user_data = response
	};
	esp_http_client_handle_t client = esp_http_client_init(&config);

	// POST
	esp_http_client_set_url(client, url);
	esp_http_client_set_method(client, HTTP_METHOD_POST);
	esp_http_client_set_header(client, "Authorization", authorization);
	esp_http_client_set_header(client, "Content-Type", "application/vnd.com.nsn.cumulocity.managedObject+json");
	esp_http_client_set_header(client, "Accept", "application/vnd.com.nsn.cumulocity.managedObject+json");

	// create a client json
	cJSON *json_client = cJSON_CreateObject();
	if (json_client == NULL) return ESP_FAIL;
	if (cJSON_AddStringToObject(json_client, "name", uuid) == NULL) return ESP_FAIL;
	cJSON *device = NULL;
	device = cJSON_AddObjectToObject(json_client, "c8y_IsDevice");
	if (device == NULL) return ESP_FAIL;
	cJSON *hardware = NULL;
	hardware = cJSON_AddObjectToObject(json_client, "c8y_Hardware");
	if (hardware == NULL) return ESP_FAIL;
	if (cJSON_AddStringToObject(hardware, "model", "ESP32D0WDQ6") == NULL) return ESP_FAIL;
	if (cJSON_AddStringToObject(hardware, "revision", "1.0") == NULL) return ESP_FAIL;
	if (cJSON_AddStringToObject(hardware, "serialNumber", uuid) == NULL) return ESP_FAIL;
	char *json_string = cJSON_PrintUnformatted(json_client);
	ESP_LOGD(__func__, "CLIENT %s", json_string);
	cJSON_Delete(json_client);

	esp_http_client_set_post_field(client, json_string, (int) (strlen(json_string)));
	esp_err_t err = esp_http_client_perform(client);
	if (err == ESP_OK) {
		const int http_status = esp_http_client_get_status_code(client);
		const int content_length = esp_http_client_get_content_length(client);
		ESP_LOGI(__func__, "HTTP POST status = %d, content_length = %d", http_status, content_length);
		err = (http_status == 201) ? ESP_OK : ESP_FAIL;
	} else {
		ESP_LOGE(__func__, "HTTP POST request failed: %s", esp_err_to_name(err));
	}

	if (err == ESP_OK) {
		c8y_status.created = true;
		c8y_create_parse_response(response);
	}

	esp_http_client_cleanup(client);
	return err;
}

void c8y_create() {
	ESP_LOGD(__func__, "");
	char *uuid = get_hw_ID_string();
	char url[64];
	strcpy(url, "https://"); // 8 char
	strcat(url, TENANT); // 6 char
	strcat(url, ".cumulocity.com"); // 15 char
	strcat(url, "/inventory/managedObjects"); // 25 char

	char *authorization = NULL;
	c8y_get_authorization(&authorization);

	c8y_response *response = c8y_response_new(160);
	esp_err_t err = c8y_create_post(url, uuid, authorization, response);
	c8y_response_free(response);
	free(authorization);
	vTaskDelay(pdMS_TO_TICKS(3000));
}

//################################################
esp_err_t c8y_register_parse_response(c8y_response *response) {
	c8y_terminate_string(response);
	ESP_LOGD(__func__, "OK = %s", response->buffer);
	cJSON *json_response = cJSON_Parse(response->buffer);
//	//get the password
//	cJSON *temp_object = cJSON_GetObjectItemCaseSensitive(json_response, "id");
//	if (cJSON_IsString(temp_object) && (temp_object->valuestring != NULL)) {
//		char *c8y_id = malloc(strlen(temp_object->valuestring));
//		strcpy(c8y_id, temp_object->valuestring);
//		ESP_LOGD(__func__, "id: %s", c8y_id);
//		c8y_status.id = c8y_id;
//	} else
//		return ESP_FAIL;

	char *json_string_result = cJSON_Print(json_response);
	ESP_LOGD("JSON", ": %s", json_string_result);
	cJSON_Delete(json_response);
	free(json_string_result);
	return ESP_OK;
}

esp_err_t c8y_register_post(const char *url, const char *uuid, const char *authorization, c8y_response *response) {
	ESP_LOGD(__func__, "url: %s)", url);

	esp_http_client_config_t config = {
			.url = url,
			.event_handler = _c8y_http_event_handler,
			.user_data = response
	};
	esp_http_client_handle_t client = esp_http_client_init(&config);

	// POST
	esp_http_client_set_url(client, url);
	esp_http_client_set_method(client, HTTP_METHOD_POST);
	esp_http_client_set_header(client, "Authorization", authorization);
	esp_http_client_set_header(client, "Content-Type", "application/vnd.com.nsn.cumulocity.externalId+json");
	esp_http_client_set_header(client, "Accept", "application/vnd.com.nsn.cumulocity.externalId+json");

	// create a register json
	cJSON *json_register = cJSON_CreateObject();
	if (json_register == NULL) return ESP_FAIL;
	if (cJSON_AddStringToObject(json_register, "type", "c8y_Serial") == NULL) return ESP_FAIL;
	if (cJSON_AddStringToObject(json_register, "externalId", uuid) == NULL) return ESP_FAIL;
	char *json_string = cJSON_PrintUnformatted(json_register);
	ESP_LOGD(__func__, "CLIENT %s", json_string);
	cJSON_Delete(json_register);

	esp_http_client_set_post_field(client, json_string, (int) (strlen(json_string)));
	esp_err_t err = esp_http_client_perform(client);
	if (err == ESP_OK) {
		const int http_status = esp_http_client_get_status_code(client);
		const int content_length = esp_http_client_get_content_length(client);
		ESP_LOGI(__func__, "HTTP POST status = %d, content_length = %d", http_status, content_length);
		err = (http_status == 201) ? ESP_OK : ESP_FAIL;
	} else {
		ESP_LOGE(__func__, "HTTP POST request failed: %s", esp_err_to_name(err));
	}

	if (err == ESP_OK) {
		c8y_status.created = true;
		c8y_register_parse_response(response);
//		while (1) {
//			// todo
//			if (rtc_wdt_is_on()) rtc_wdt_feed();
//		}
	}

	esp_http_client_cleanup(client);
	return err;
}

void c8y_register() {
	ESP_LOGD(__func__, "");
	char *uuid = get_hw_ID_string();
	char url[77];
	strcpy(url, "https://"); // 8 char
	strcat(url, TENANT); // 6 char
	strcat(url, ".cumulocity.com"); // 15 char
	strcat(url, "/identity/globalIds/"); // 20 char
	strcat(url, c8y_status.id); // 6 char
	strcat(url, "/externalIds"); // 12 char

	char *authorization = NULL;
	c8y_get_authorization(&authorization);

	c8y_response *response = c8y_response_new(160);
	esp_err_t err = c8y_register_post(url, uuid, authorization, response);
	c8y_response_free(response);
	free(authorization);
	vTaskDelay(pdMS_TO_TICKS(3000));
}

void c8y_test(void) {
	ESP_LOGD(__func__, "");

	c8y_status.bootstrapped = false;
	c8y_status.registered = false;

	if (c8y_load_status() == ESP_OK) {
		ESP_LOGI("C8Y-status", "loaded");
	} else {
		ESP_LOGW("C8Y-status", "not available");
		c8y_status.bootstrapped = false;
	}
//	c8y_status.bootstrapped = false;
// todo make the bootstrap automatic

	c8y_http_client_init(get_hw_ID_string());
	c8y_bootstrap();
	if (c8y_registered() != ESP_OK) {
		c8y_create();
		c8y_register();
	}
	c8y_store_status();

	while (1) {
		if (rtc_wdt_is_on()) rtc_wdt_feed();
	}

}

void message_chained(ubirch_protocol_c8y *proto) {

}


