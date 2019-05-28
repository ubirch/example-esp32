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
#include "../../components/cjson/include/cjson/cJSON.h"
#include "../../components/cjson/include/cjson/cJSON_Utils.h"

#include "settings.h"
#include "ubirch-protocol-c8y.h"

#define LOG_LOCAL_LEVEL ESP_LOG_DEBUG

extern unsigned char UUID[16];

struct c8y_status {
	bool registered;
} c8y_status;


#define COUNTER_SIZE (sizeof(_c8y_atomic_counter_t))


static inline void init_count(void *buffer) {
	*(volatile _c8y_atomic_counter_t *) buffer = 1;
}

static inline void decr_count(void *buffer) {
	// atomic if(--*(_msgpack_atomic_counter_t*)buffer == 0) { free(buffer); }
	if (_c8y_sync_decr_and_fetch((volatile _c8y_atomic_counter_t *) buffer) == 0) {
		free(buffer);
	}
}

static inline void incr_count(void *buffer) {
	// atomic ++*(_msgpack_atomic_counter_t*)buffer;
	_c8y_sync_incr_and_fetch((volatile _c8y_atomic_counter_t *) buffer);
}

static inline _c8y_atomic_counter_t get_count(void *buffer) {
	return *(volatile _c8y_atomic_counter_t *) buffer;
}


static inline size_t c8y_buffer_capacity(const c8y_response *response) {
	ESP_LOGD(__func__, "free: %d", response->free);
	return response->free;
}

static inline void c8y_buffer_consumed(c8y_response *response, size_t size) {
	ESP_LOGD(__func__, "");
	response->used += size;
	response->free -= size;

	ESP_LOGD(__func__, "used: %d, free: %d, off: %d, parsed: %d, initial: %d",
	         response->used, response->free, response->off, response->parsed, response->initial_buffer_size);
}

static inline bool c8y_reserve_buffer(c8y_response *response, size_t size) {
	ESP_LOGD(__func__, "");
	if (response->free >= size) {
		ESP_LOGD(__func__, " NO, enough space");
		return true;
	}
	ESP_LOGD(__func__, " YES, expand");
	return c8y_expand_buffer(response, size);
}

static inline char *c8y_buffer(c8y_response *response) {
	ESP_LOGD(__func__, " addr: %d", (int) (response->buffer + response->used));
	return response->buffer + response->used;
}


c8y_response *c8y_response_new(size_t initial_buffer_size) {
	ESP_LOGD(__func__, "");
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
	ESP_LOGD(__func__, "OK");
	return response;
}

bool c8y_response_init(c8y_response *response, size_t initial_buffer_size) {
	ESP_LOGD(__func__, "");
	if (initial_buffer_size < COUNTER_SIZE) {
		initial_buffer_size = COUNTER_SIZE;
	}

	char *buffer = (char *) malloc(initial_buffer_size);
	if (buffer == NULL) {
		ESP_LOGE(__func__, "buffer == NULL");
		return false;
	}

	response->buffer = buffer;
	response->used = COUNTER_SIZE;
	response->free = initial_buffer_size - response->used;
	response->off = COUNTER_SIZE;
	response->parsed = 0;
	response->initial_buffer_size = initial_buffer_size;

	init_count(response->buffer);

	ESP_LOGD(__func__, "addr: %d, used: %d, free: %d, off: %d, parsed: %d, initial: %d",
	         (int) (response->buffer), response->used, response->free, response->off, response->parsed,
	         response->initial_buffer_size);

	return true;
}


bool c8y_expand_buffer(c8y_response *response, size_t size) {
	ESP_LOGD(__func__, "");
	if (response->used == response->off && get_count(response->buffer) == 1) {
		// rewind buffer
		response->free += response->used - COUNTER_SIZE;
		response->used = COUNTER_SIZE;
		response->off = COUNTER_SIZE;

		if (response->free >= size) {
			return true;
		}
	}

	if (response->off == COUNTER_SIZE) {
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

	} else {
		size_t next_size = response->initial_buffer_size;  // include COUNTER_SIZE
		size_t not_parsed = response->used - response->off;
		while (next_size < size + not_parsed + COUNTER_SIZE) {
			next_size *= 2;
		}

		char *tmp = (char *) malloc(next_size);
		if (tmp == NULL) {
			ESP_LOGE(__func__, "tmp == NULL");
			return false;
		}

		init_count(tmp);

		memcpy(tmp + COUNTER_SIZE, response->buffer + response->off, not_parsed);

		response->buffer = tmp;
		response->used = not_parsed + COUNTER_SIZE;
		response->free = next_size - response->used;
		response->off = COUNTER_SIZE;
	}

	ESP_LOGD(__func__, "addr: %d, used: %d, free: %d, off: %d, parsed: %d, initial: %d",
	         (int) (response->buffer), response->used, response->free, response->off, response->parsed,
	         response->initial_buffer_size);


	return true;
}

void c8y_response_destroy(c8y_response *response) {
	decr_count(response->buffer);
}

void c8y_response_free(c8y_response *response) {
	c8y_response_destroy(response);
	free(response);
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


		// only feed data if the unpacker is available
		if (evt->user_data != NULL) {

			c8y_response *response = evt->user_data;

			if (esp_http_client_is_chunked_response(evt->client)) {
				ESP_LOG_BUFFER_HEXDUMP("CHUNKED", evt->data, (uint16_t) evt->data_len, ESP_LOG_DEBUG);
				if (c8y_buffer_capacity(response) < evt->data_len) {
					c8y_reserve_buffer(response, (uint16_t) evt->data_len);
				}
				memcpy(c8y_buffer(response), evt->data, (uint16_t) evt->data_len);
				c8y_buffer_consumed(response, (uint16_t) evt->data_len);
			} else {
				ESP_LOGW(__func__, "endet up here");
			}
		}
	}
	return ESP_OK;
}


void c8yHttpClientInit(const char *uuid) {

	c8y_status.registered = false;

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

esp_err_t c8y_parse_response(c8y_response *response) {
	c8y_terminate_string(response);
	printf("OK = %s\r\n", response->buffer + response->off);
	cJSON *json_response = cJSON_Parse(response->buffer + response->off);
	//get the password
	cJSON *temp_object = cJSON_GetObjectItemCaseSensitive(json_response, "password");
	if (cJSON_IsString(temp_object) && (temp_object->valuestring != NULL)) {
		char *c8y_password = malloc(strlen(temp_object->valuestring));
		strcpy(c8y_password, temp_object->valuestring);
		ESP_LOGD(__func__, "password: %s", c8y_password);
		//todo store the password
	} else return ESP_FAIL;
	//get the username
	cJSON *temp_object2 = cJSON_GetObjectItemCaseSensitive(json_response, "username");
	if (cJSON_IsString(temp_object2) && (temp_object2->valuestring != NULL)) {
		char *c8y_username = malloc(strlen(temp_object2->valuestring));
		strcpy(c8y_username, temp_object2->valuestring);
		ESP_LOGD(__func__, "username: %s", c8y_username);
		// todo store the tenantId
	} else return ESP_FAIL;

	char *json_string_result = cJSON_Print(json_response);
	printf("JSON: %s", json_string_result);
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

//	c8y_response.complete = false;
//	c8y_response.initial = true;


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
		c8y_status.registered = true;
		c8y_parse_response(response);
		while (1) {
			// todo
			if (rtc_wdt_is_on()) rtc_wdt_feed();
		}
	}

	esp_http_client_cleanup(client);
	return err;
}

void c8y_bootstrap() {
	ESP_LOGD(__func__, "");
	char *uuid = get_hw_ID_string();
	char url[70];
	strcpy(url, "https://");
	strcat(url, host);
	strcat(url, "/devicecontrol/deviceCredentials");


	while (c8y_status.registered == false) {
		c8y_response *response = c8y_response_new(160);
		c8y_bootstrap_post(url, uuid, strlen(uuid), response);
//		printf("RESPONSE = %s\r\n", response->buffer + response->off);
		c8y_response_free(response);
		vTaskDelay(pdMS_TO_TICKS(3000));
	}
}

void c8y_test(void) {
	ESP_LOGD(__func__, "");

	c8yHttpClientInit(get_hw_ID_string());
	c8y_bootstrap();

	while (1) {
		if (rtc_wdt_is_on()) rtc_wdt_feed();
	}

}

void message_chained(ubirch_protocol_c8y *proto) {

}


