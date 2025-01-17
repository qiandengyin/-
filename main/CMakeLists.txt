idf_component_register(
    SRC_DIRS
        "."
        "ble_hid"
        "tusb_hid"
        "app_wifi"
        "app_espnow"
        "keyboard_bsp"
        "keyboard"
        "app_audio"
        "app_led"
        "app_uart"
        "app_udp_client"
        "baidu_api"
        "chatgpt_api"

    INCLUDE_DIRS
        "."
        "ble_hid"
        "tusb_hid"
        "app_wifi"
        "app_espnow"
        "keyboard_bsp"
        "keyboard"
        "app_audio"
        "app_led"
        "app_uart"
        "app_udp_client"
        "baidu_api"
        "chatgpt_api")

spiffs_create_partition_image(storage ../spiffs FLASH_IN_PROJECT)