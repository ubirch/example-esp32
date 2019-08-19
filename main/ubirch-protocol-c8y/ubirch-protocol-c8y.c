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
#include <freertos/event_groups.h>
#include <networking.h>
#include "mbedtls/base64.h"

#include "../../components/cjson/include/cjson/cJSON.h"
#include "../../components/cjson/include/cjson/cJSON_Utils.h"

#include "settings.h"
#include "ubirch-protocol-c8y.h"

#define LOG_LOCAL_LEVEL ESP_LOG_DEBUG

static const char *TAG = "C8Y";

extern unsigned char UUID[16];


/*!
 * Cumulocity status structure
 */
struct c8y_status {
	bool bootstrapped;      //<! bootstrapping done
	bool registered;        //<! device already registered
	bool created;           //<! device created in Cumulocity
	char *username;         //<! username for connection
	char *password;         //<! password for connection
	char *id;               //<! internal Cumulocity ID, to use later
} c8y_status;


/*!
 * Load the cumulocity status/credentials from flash memory into c8y_status.
 *
 * @return ESP_OK, if status is available
 * @return ESP_ERROR..., if the status is not available, or flash is corrupt
 */
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
	// somehow the username is not 0 teminated and breaks, if it won't be terminated
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
	// somehow the password is not 0 teminated and breaks, if it won't be terminated
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
	// todo somehow the id is not 0 teminated and breaks
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


/*!
 * Store the Cumulocity status/credentials from c8y_status in the flash memory.
 *
 * @return ESP_OK, if status is available
 * @return ESP_ERROR..., if storing was not possible
 */
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

/*!
 * ##################################
 * Helper Functions for c8y_response.
 * ##################################
 */

/*!
 * Get the capacity of the response buffer.
 *
 * @param response
 * @return current size
 */
static inline size_t c8y_buffer_capacity(const c8y_response *response) {
	return response->free;
}

/*!
 * Update the capacity of the response buffer.
 *
 * @param response
 * @param size
 */
static inline void c8y_buffer_consumed(c8y_response *response, size_t size) {
	response->used += size;
	response->free -= size;
}

/*!
 * Initialize the response buffer.
 *
 * @param response
 * @param initial_buffer_size
 * @return
 */
static bool c8y_response_init(c8y_response *response, size_t initial_buffer_size) {
	char *buffer = (char *) malloc(initial_buffer_size);
	if (buffer == NULL) {
		ESP_LOGE(__func__, "buffer == NULL");
		return false;
	}

	response->buffer = buffer;
	response->used = 0;
	response->free = initial_buffer_size - response->used;
	return true;
}

/*!
 * Expand the current response buffer.
 *
 * @param response
 * @param size
 * @return
 */
static bool c8y_expand_buffer(c8y_response *response, size_t size) {
	if (response->used == 0) {
		// rewind buffer
		response->free += response->used - 0;
		response->used = 0;

		if (response->free >= size) {
			return true;
		}
	}

	size_t next_size = (response->used + response->free) * 2;  // include 0
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

/*!
 * Reserve a certain space for response buffer.
 *
 * @param response
 * @param size
 * @return
 */
static inline bool c8y_reserve_buffer(c8y_response *response, size_t size) {
	if (response->free >= size) {
		return true;
	}
	return c8y_expand_buffer(response, size);
}

/*!
 * Get the pointer to the current free position in the response buffer.
 *
 * @param response
 * @return
 */
static inline char *c8y_buffer(c8y_response *response) {
	return response->buffer + response->used;
}

/*!
 * Create a new response buffer.
 *
 * @param initial_buffer_size
 * @return
 */
static c8y_response *c8y_response_new(size_t initial_buffer_size) {
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


/*!
 * Delete the response buffer.
 *
 * @param response
 */
static void c8y_response_free(c8y_response *response) {
	free(response);
}


/*!
 * Get the authorization in base64 encoding,
 * consisting of the username and the password
 *
 * @param auth is the pointer to the string pointer for the authorization string.
 *
 * @note this function allocates memory for the authorization token.
 * todo error handling
 */
void c8y_get_authorization(char **auth) {
	char token[strlen(c8y_status.username) + strlen(c8y_status.password) + 1];
	strcpy(token, c8y_status.username);
	strcat(token, ":");
	strcat(token, c8y_status.password);
	ESP_LOGD("TOKEN", "%s", token);
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
	ESP_LOGD("AUTHORIZATION", "%s", *auth);
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

/*!
 * Initialize the http client for the cumulocity connection.
 *
 * @param uuid Unique user identification of the device
 */
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

/*!
 * Terminate the response string with '\0'
 *
 * @param response c8y_response structure to terminate
 * @return ESP_OK, if response is available,
 * @return ESP_FAIL, if response not available
 * todo check the length of response
 */
static esp_err_t c8y_terminate_string(c8y_response *response) {
	if (response == NULL)
		return ESP_FAIL;
	response->buffer[response->used] = '\0';
	response->used += 1;
	response->free -= 1;
	return ESP_OK;
}
//############################################################################################
/*
 * JSON bootstrap response
{
	"password":"1Ex1_StR1w",
	"tenantId":"ubirch",
	"self":"http://management.cumulocity.com/devicecontrol/deviceCredentials/30AEA423-4968-1223-3445-566778899AAB",
	"id":"30AEA423-4968-1223-3445-566778899AAB",
	"username":"device_30AEA423-4968-1223-3445-566778899AAB"
}
 * */

/*!
 * Parse the bootstrap response, by extracting 'username' and 'password'
 *
 * @param response cy8_response struct to store the parsed information
 * @return ESP_OK, if the required information is avaliable
 * @return ESP_FAIL, if the information is not available
 * todo check fot NULL pointer
 */
static esp_err_t c8y_bootstrap_parse_response(c8y_response *response) {
	c8y_terminate_string(response);
	ESP_LOGD(__func__, "OK = %s\r\n", response->buffer);
	cJSON *json_response = cJSON_Parse(response->buffer);
	//get the password
	cJSON *temp_object = cJSON_GetObjectItemCaseSensitive(json_response, "password");
	if (cJSON_IsString(temp_object) && (temp_object->valuestring != NULL)) {
		size_t len = strlen(temp_object->valuestring) + 1; // add one more to end string with '\0'
		char *c8y_password = malloc(len);
		strcpy(c8y_password, temp_object->valuestring);
		c8y_password[len - 1] = '\0';
		ESP_LOGD(__func__, "password: %s, len: %d", c8y_password, len - 1);
		if (c8y_status.password != NULL) free(c8y_status.password);
		c8y_status.password = c8y_password;
	} else {
		ESP_LOGW(__func__, "password not found");
		return ESP_FAIL;
	}
	//get the username
	cJSON *temp_object2 = cJSON_GetObjectItemCaseSensitive(json_response, "username");
	if (cJSON_IsString(temp_object2) && (temp_object2->valuestring != NULL)) {
		size_t len = strlen(temp_object2->valuestring) + 1; // add one more to end string with '\0'
		char *c8y_username = malloc(len);
		strcpy(c8y_username, temp_object2->valuestring);
		c8y_username[len - 1] = '\0';
		ESP_LOGD(__func__, "username: %s, len: %d", c8y_username, len - 1);
		if (c8y_status.username != NULL) free(c8y_status.username);
		c8y_status.username = c8y_username;
	} else {
		ESP_LOGW(__func__, "username not found");
		return ESP_FAIL;
	}

	char *json_string_result = cJSON_Print(json_response);
	ESP_LOGD("JSON", ": %s", json_string_result);
	cJSON_Delete(json_response);
	free(json_string_result);
	return ESP_OK;
}

/*!
 * HTTP POST request for the bootstrapping in cumulocity.
 *
 * @param url to do the bootstrapping
 * @param data for the http post
 * @param response c8y_response structure for parsing the reply
 * @return ESP_OK, post was succesful reply between 200 and 299
 * @return ESP_FAIL, if post was not sucessful
 */
static esp_err_t c8y_bootstrap_post(const char *url, const char *data, c8y_response *response) {
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

/*!
 * Perform the bootstrapping
 */
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
		c8y_bootstrap_post(url, uuid, response);
		c8y_response_free(response);
		free(uuid);
		vTaskDelay(pdMS_TO_TICKS(3000));
	}
}

//############################################################################################
/*
 * JSON registered parse_response
 {
        "managedObject":        {
                "self": "https://ubirch.cumulocity.com/inventory/managedObjects/286171",
                "id":   "286171"
        },
        "externalId":   "30AEA423-4968-1223-3445-566778899AAB",
        "self": "https://ubirch.cumulocity.com/identity/externalIds/c8y_Serial/30AEA423-4968-1223-3445-566778899AAB",
        "type": "c8y_Serial"
}

 */

/*!
 * Parse the response, while checking, if the device is registerd on Cumulocity.
 *
 * @param response structure, with the response to parse.
 * @return ESP_OK, if response can be parsed and if it contains "id"
 * @return ESP_FAIL, otherwise
 */
esp_err_t c8y_registered_parse_response(c8y_response *response) {
	c8y_terminate_string(response);
	ESP_LOGD(__func__, "OK = %s", response->buffer);
	cJSON *json_response = cJSON_Parse(response->buffer);

	//get the id
	cJSON *managed_object = cJSON_GetObjectItemCaseSensitive(json_response, "managedObject");
	cJSON *temp_object = cJSON_GetObjectItemCaseSensitive(managed_object, "id");
	if (cJSON_IsString(temp_object) && (temp_object->valuestring != NULL)) {
		size_t len = strlen(temp_object->valuestring) + 1; // add one more to end string with '\0'
		char *c8y_id = malloc(len);
		strcpy(c8y_id, temp_object->valuestring);
		c8y_id[len - 1] = '\0';
		ESP_LOGD(__func__, "id: %s, len: %d", c8y_id, len - 1);
		// todo compare the ids
		if (c8y_status.id != NULL) free(c8y_status.id);
		c8y_status.id = c8y_id;
	} else {
		ESP_LOGW(__func__, "id not found");
		return ESP_FAIL;
	}

	char *json_string_result = cJSON_Print(json_response);
	ESP_LOGD("JSON", ": %s", json_string_result);
	cJSON_Delete(json_response);
	free(json_string_result);
	return ESP_OK;

}

/*!
 * Get http request to check if the device is registered already
 *
 * @param url to cumulocity
 * @param authorization base64 encoded token from username and password
 * @param response json object with device information
 * @return ESP_OK, if get request was successful,
 * @return ESP_FAIL, if the get request failed
 */
esp_err_t c8y_registered_get(const char *url, const char *authorization, c8y_response *response) {
	ESP_LOGD(__func__, "url: %s)", url);

	esp_http_client_config_t config = {
			.url = url,
			.event_handler = _c8y_http_event_handler,
			.user_data = response
	};
	esp_http_client_handle_t client = esp_http_client_init(&config);

	// GET
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
	}

	esp_http_client_cleanup(client);
	return err;
}

/*!
 * Perform the cumulocity request, to check if the device is already registered.
 *
 * @return ESP_OK, if the device is registered,
 * @return ESP_FAIL, if the device is not registerd yet
 */
esp_err_t c8y_registered() {
	ESP_LOGD(__func__, "");
	char *uuid = get_hw_ID_string();
	char url[108];
	strcpy(url, "https://"); // 8 char
	strcat(url, TENANT); // 6 char
	strcat(url, ".cumulocity.com"); // 15 char
	strcat(url, "/identity/externalIds/c8y_Serial/"); // 33 char
	strcat(url, uuid); // 37 char
	free(uuid);

	// get the authorization token
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
/*
 * JSON response for create
 {
        "additionParents":      {
                "self": "https://ubirch.cumulocity.com/inventory/managedObjects/286171/additionParents",
                "references":   []
        },
        "owner":        "device_30AEA423-4968-1223-3445-566778899AAB",
        "childDevices": {
                "self": "https://ubirch.cumulocity.com/inventory/managedObjects/286171/childDevices",
                "references":   []
        },
        "childAssets":  {
                "self": "https://ubirch.cumulocity.com/inventory/managedObjects/286171/childAssets",
                "references":   []
        },
        "creationTime": "2019-06-03T09:19:51.768Z",
        "lastUpdated":  "2019-06-03T09:19:51.768Z",
        "childAdditions":       {
                "self": "https://ubirch.cumulocity.com/inventory/managedObjects/286171/childAdditions",
                "references":   []
        },
        "name": "30AEA423-4968-1223-3445-566778899AAB",
        "assetParents": {
                "self": "https://ubirch.cumulocity.com/inventory/managedObjects/286171/assetParents",
                "references":   []
        },
        "deviceParents":        {
                "self": "https://ubirch.cumulocity.com/inventory/managedObjects/286171/deviceParents",
                "references":   []
        },
        "self": "https://ubirch.cumulocity.com/inventory/managedObjects/286171",
        "id":   "286171",
        "c8y_IsDevice": {
        },
        "c8y_Hardware": {
                "serialNumber": "30AEA423-4968-1223-3445-566778899AAB",
                "model":        "ESP32D0WDQ6",
                "revision":     "1.0"
        }
}
 */
/*!
 * Parse the response of creating a device on cumulocity.
 *
 * @param response buffer of the response, to process
 * @return ESP_OK, if response is valid and inhibits the parameter "id"
 * @return ESP_FAIL, if the response is faulty, or does not inhibit the parameter "id"
 */
esp_err_t c8y_create_parse_response(c8y_response *response) {
	c8y_terminate_string(response);
	ESP_LOGD(__func__, "OK = %s", response->buffer);
	cJSON *json_response = cJSON_Parse(response->buffer);
	//get the id
	cJSON *temp_object = cJSON_GetObjectItemCaseSensitive(json_response, "id");
	if (cJSON_IsString(temp_object) && (temp_object->valuestring != NULL)) {
		size_t len = strlen(temp_object->valuestring) + 1; // add one more to end string with '\0'
		char *c8y_id = malloc(len);
		strcpy(c8y_id, temp_object->valuestring);
		c8y_id[len - 1] = '\0';
		ESP_LOGD(__func__, "id: %s, len: %d", c8y_id, len - 1);
		// todo compare the ids
		if (c8y_status.id != NULL) free(c8y_status.id);
		c8y_status.id = c8y_id;
	} else {
		ESP_LOGW(__func__, "id not found");
		return ESP_FAIL;
	}

	char *json_string_result = cJSON_Print(json_response);
	ESP_LOGD("JSON", ": %s", json_string_result);
	cJSON_Delete(json_response);
	free(json_string_result);
	return ESP_OK;
}

/*!
 * http post request to create a device in cumulocity.
 *
 * @param url to cumulocity create
 * @param uuid of the device
 * @param authorization base64 token
 * @param response
 * @return
 */
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


/*!
 * Create a device on cumulocity
 */
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
	free(uuid);
	vTaskDelay(pdMS_TO_TICKS(3000));
}

//################################################
/*
 * JSON response for register
{
        "managedObject":        {
                "self": "https://ubirch.cumulocity.com/inventory/managedObjects/286171",
                "id":   "286171"
        },
        "externalId":   "30AEA423-4968-1223-3445-566778899AAB",
        "self": "https://ubirch.cumulocity.com/identity/externalIds/c8y_Serial/30AEA423-4968-1223-3445-566778899AAB",
        "type": "c8y_Serial"
}
 */

/*!
 * Parse the response, during registering a device on Cumulocity
 * @param response structure for the response
 * @return ESP_OK, if response was parsed sucessfully and "id" was was found in the response
 * @return ESP_FAIL, otherwise
 */
esp_err_t c8y_register_parse_response(c8y_response *response) {
	c8y_terminate_string(response);
	ESP_LOGD(__func__, "OK = %s", response->buffer);
	cJSON *json_response = cJSON_Parse(response->buffer);
	//get the id
	cJSON *managed_object = cJSON_GetObjectItemCaseSensitive(json_response, "managedObject");
	cJSON *temp_object = cJSON_GetObjectItemCaseSensitive(managed_object, "id");
	if (cJSON_IsString(temp_object) && (temp_object->valuestring != NULL)) {
		size_t len = strlen(temp_object->valuestring) + 1; // add one more to end string with '\0'
		char *c8y_id = malloc(len);
		strcpy(c8y_id, temp_object->valuestring);
		c8y_id[len - 1] = '\0';
		ESP_LOGD(__func__, "id: %s, len: %d", c8y_id, len - 1);
		// todo compare the ids
		if (c8y_status.id != NULL) free(c8y_status.id);
		c8y_status.id = c8y_id;
	} else {
		ESP_LOGW(__func__, "id not found");
		return ESP_FAIL;
	}

	char *json_string_result = cJSON_Print(json_response);
	ESP_LOGD("JSON", ": %s", json_string_result);
	cJSON_Delete(json_response);
	free(json_string_result);
	return ESP_OK;
}

/*!
 * HTTP post to register a device on Cumulocity.
 *
 * @param url to the site for registering
 * @param uuid of the device
 * @param authorization token
 * @param response structure for the response
 * @return ESP_OK, if the post was sucessful
 * @return ESP_FAIL, if post was not sucessful
 */
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
	}

	esp_http_client_cleanup(client);
	return err;
}


/*!
 * Perform a device registration on Cumulocity.
 */
void c8y_register(void) {
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
	free(uuid);
	vTaskDelay(pdMS_TO_TICKS(3000));
}

//################################################
/*! @code example of json structure for the measurement
{
	"source": {
        "id": "303958"
	},
	"type": "EXAMPLE-MOCK",
	"time": "1970-01-01T00:00:19.000Z",
	"c8y_Measurement": {
        "Temperature": {
            "value": 22.511999130249023,
            "unit": "°C"
        },
        "Humidity": {
            "value": 44.931999206542969,
            "unit": "%RH"
        }
	}
}
@endcode
*/

/*!
 * Create a json object for the measurement data to post to Cumulocity.
 *
 * @param timestamp of the current measurement
 * @param f_temperature floating point value of the temperature
 * @param f_humidity floating point value of the humidity
 * @return serialized json in char string format
 */
char *c8y_measurement_create_json(time_t timestamp, float f_temperature, float f_humidity) {
	bool finished = false;
	char time_string[30];
	// 2013-06-22T17:03:14.000+02:00id

	struct tm t = {0};
	localtime_r(&timestamp, &t);
	sprintf(time_string, "%04d-%02d-%02dT%02d:%02d:%02d.%03dZ",
	        (1900 + t.tm_year), (1 + t.tm_mon), t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec, 0);
	cJSON *data_json = cJSON_CreateObject();
	if (data_json == NULL) return ESP_FAIL;
	cJSON *source = NULL;
	source = cJSON_AddObjectToObject(data_json, "source");
	cJSON_AddStringToObject(source, "id", c8y_status.id);
	cJSON_AddStringToObject(data_json, "type", "EXAMPLE-MOCK");
	cJSON_AddStringToObject(data_json, "time", time_string);
	cJSON *measurement = NULL;
	measurement = cJSON_AddObjectToObject(data_json, "c8y_Measurement");
	if (measurement == NULL) return ESP_FAIL;
	cJSON *temperature = NULL;
	temperature = cJSON_AddObjectToObject(measurement, "Temperature");
	cJSON_AddNumberToObject(temperature, "value", f_temperature);
	cJSON_AddStringToObject(temperature, "unit", "°C");
	cJSON *humidity = NULL;
	humidity = cJSON_AddObjectToObject(measurement, "Humidity");
	cJSON_AddNumberToObject(humidity, "value", f_humidity);
	cJSON_AddStringToObject(humidity, "unit", "%RH");
	char *data_string = cJSON_PrintUnformatted(data_json);
	ESP_LOGD(__func__, "MEASUREMENT-DATA %s", data_string);
	cJSON_Delete(data_json);
	ESP_LOGD(__func__, "Done");
	return data_string;
}

/*!
 * Parse the response of the measurement post.
 *
 * @param response structure for the response
 * @return ESP_OK.
 * @todo no verification yet
 */
esp_err_t c8y_measurement_parse_response(c8y_response *response) {
	c8y_terminate_string(response);
	ESP_LOGD(__func__, "OK = %s", response->buffer);
	cJSON *json_response = cJSON_Parse(response->buffer);
	if (json_response != NULL) {
		char *json_string_result = cJSON_Print(json_response);
		ESP_LOGD("JSON", ": %s", json_string_result);
		cJSON_Delete(json_response);
		free(json_string_result);
	}
	return ESP_OK;
}

/*!
 * HTTP post of the measurement data to Cumulocity.
 *
 * @param url to Cumulocity measurements
 * @param data to post
 * @param authorization token
 * @param response structure for the response
 * @return ESP_OK, if post was successful
 * @return ESP_FAIL, if http post status != 201
 */
esp_err_t c8y_measurement_post(const char *url, const char *data, const char *authorization, c8y_response *response) {
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
	esp_http_client_set_header(client, "Content-Type", "application/vnd.com.nsn.cumulocity.measurement+json");
	esp_http_client_set_header(client, "Accept", "application/vnd.com.nsn.cumulocity.measurement+json");


	esp_http_client_set_post_field(client, data, (int) (strlen(data)));
	esp_err_t err = esp_http_client_perform(client);
	if (err == ESP_OK) {
		const int http_status = esp_http_client_get_status_code(client);
		const int content_length = esp_http_client_get_content_length(client);
		ESP_LOGI(__func__, "HTTP POST status = %d, content_length = %d", http_status, content_length);
		err = (http_status == 201) ? ESP_OK : ESP_FAIL;
	} else {
		ESP_LOGE(__func__, "HTTP POST request failed: %s", esp_err_to_name(err));
	}

	c8y_measurement_parse_response(response);

	esp_http_client_cleanup(client);
	return err;
}


/*!
 * Perform a measurement post to Cumulocity.
 *
 * @param timestamp of the current measurement
 * @param temperature floating point value of the temperature
 * @param humidity floating point value of the humidity
 * @return ESP_OK, if measurement was posted successfully
 * @return ESP_FAIL, if error
 */
esp_err_t c8y_measurement(time_t timestamp, float temperature, float humidity) {
	ESP_LOGD(__func__, "");

	// generate the URL
	char url[58];
	strcpy(url, "https://"); // 8 char
	strcat(url, TENANT); // 6 char
	strcat(url, ".cumulocity.com"); // 15 char
	strcat(url, "/measurement/measurements"); // 25 char

	char *measurement = c8y_measurement_create_json(timestamp, temperature, humidity);
	ESP_LOGI(__func__, "measurement: %s", measurement);

	char *authorization = NULL;
	c8y_get_authorization(&authorization);

	c8y_response *response = c8y_response_new(160);
	esp_err_t err = c8y_measurement_post(url, measurement, authorization, response);
	c8y_response_free(response);
	free(authorization);
	vTaskDelay(pdMS_TO_TICKS(10000));
	return err;
}


// todo change the name
void c8y_start(void) {
	ESP_LOGD(__func__, "");

	c8y_status.bootstrapped = false;
	c8y_status.registered = false;

	if (c8y_load_status() == ESP_OK) {
		ESP_LOGI("C8Y-status", "loaded");
	} else {
		ESP_LOGW("C8Y-status", "not available");
		c8y_status.bootstrapped = false;
	}
// todo make the bootstrap automatic

	char *uuid = get_hw_ID_string();
	c8y_http_client_init(uuid);
	free(uuid);
	c8y_bootstrap();
	if (c8y_registered() != ESP_OK) {
		c8y_create();
		c8y_register();
	}
	c8y_store_status();

	xEventGroupSetBits(network_event_group, C8Y_READY);

	vTaskDelay(portMAX_DELAY);
}

