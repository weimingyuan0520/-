# ============================================================
#  main 组件 — 主应用程序
# ============================================================
set(src_dirs
    .                   # main.c, filter.c 等
    ../components/wifi-sta  # WiFi STA 驱动 (未注册到 components)
)

set(include_dirs
    .                   # 本目录头文件
    ../components/wifi-sta
)

set(requires
    components
    driver
    freertos
    esp_adc
    spi_flash
    nvs_flash
    esp_wifi
    esp_netif
    lwip
)

idf_component_register(
    SRC_DIRS    ${src_dirs}
    INCLUDE_DIRS ${include_dirs}
    REQUIRES    ${requires}
)

component_compile_options(-Wno-format)
