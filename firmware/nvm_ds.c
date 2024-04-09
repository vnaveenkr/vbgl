#include <stdio.h>
#include <inttypes.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "driver/gpio.h"

// 1 - Success, 0 - Fail
bool nvm_read(char *id, uint8_t *data, int size)
{
	nvs_handle_t handle;
	esp_err_t err;
	size_t reqsize = size;

	err = nvs_open("storage", NVS_READONLY, &handle);
	if (err != ESP_OK)
		goto errexit1;
	err = nvs_get_blob(handle, id, data, &reqsize);
	if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND)
		goto errexit1;
	if (reqsize == 0)
		goto errexit2;
	nvs_close(handle);
	return 1;

errexit1:
	printf("NVM read failed\n");
errexit2:
	printf("Nothing in NVM\n");
	nvs_close(handle);
	return 0;
}

void nvm_write(char *id, uint8_t *data, int size)
{
	nvs_handle_t handle;
	esp_err_t err;
	size_t reqsize = size;

	err = nvs_open("storage", NVS_READWRITE, &handle);
	if (err != ESP_OK)
		goto errexit1;
	err = nvs_set_blob(handle, id, data, reqsize);
	if (err != ESP_OK)
		goto errexit1;
	err = nvs_commit(handle);
	if (err != ESP_OK)
		goto errexit1;
	nvs_close(handle);
	return;

errexit1:
	printf("NVM write failed\n");
	nvs_close(handle);
}
