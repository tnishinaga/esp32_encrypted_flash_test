#include <stdio.h>
#include <string.h>
#include <stdint.h>

// freertos
#include "freertos/FreeRTOS.h"

// esp32 libs
#include "esp_system.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"

// esp32 driver
#include "driver/uart.h"

static const char *TAG = "test";

void app_main(void)
{
    esp_err_t ret;

    // nvs_keyパーティションを検索
    // https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/storage/spi_flash.html#_CPPv418esp_partition_find20esp_partition_type_t23esp_partition_subtype_tPKc
    esp_partition_t *nvs_key_part = 
        esp_partition_find_first(
            ESP_PARTITION_TYPE_DATA, 
            ESP_PARTITION_SUBTYPE_DATA_NVS_KEYS,
            "nvs_key"
            );
    if (nvs_key_part == NULL) {
        // パーティションがない
        ESP_LOGE(TAG, "nvs_key partition not found");
        return;
    }

    nvs_sec_cfg_t nvs_cfg;
    ret = nvs_flash_read_security_cfg(nvs_key_part, &nvs_cfg);
    switch (ret) {
        case ESP_OK:
            break;
        case ESP_ERR_NVS_KEYS_NOT_INITIALIZED:
            // 鍵生成
            ret = nvs_flash_generate_keys(nvs_key_part, &nvs_cfg);
            break;
        case ESP_ERR_NVS_CORRUPT_KEY_PART:
            ESP_LOGE(TAG, "nvs_key is found to be corrupt");
            break;
        default:
            // error
            ESP_LOGE(TAG, "unknown error");
            break;
    }
    ESP_ERROR_CHECK(ret);

    // NVSライブラリを初期化
    ret = nvs_flash_secure_init(&nvs_cfg);
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        // 空きがなかったりパーティションが作られてなかったらイレースからやり直す
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_secure_init(&nvs_cfg);
    }
    ESP_ERROR_CHECK(ret);

    nvs_handle_t handle;
    // testネームスペースのNVSをRWで開く
    ESP_ERROR_CHECK(nvs_open("test", NVS_READWRITE, &handle));

    char *data = NULL;
    size_t required_size;
    ret = nvs_get_str(handle, "hello", NULL, &required_size);
    if (ret == ESP_ERR_NVS_NOT_FOUND) {
        // 登録がない場合は登録する
        ESP_LOGI(TAG, "key not found");
        nvs_set_str(handle, "hello", "world");
    } else {
        ESP_ERROR_CHECK(ret);
        ESP_LOGI(TAG, "key found");
        data = malloc(sizeof(char) * required_size);
        ret = nvs_get_str(handle, "hello", data, &required_size);
        ESP_LOGI(TAG, "data: %s", data);
    }

    nvs_close(handle);
    nvs_flash_deinit();
    if (data != NULL) {
        free(data);
    }

    while (true) {
        vTaskDelay(300 / portTICK_PERIOD_MS);
    }
}

