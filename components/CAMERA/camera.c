#include "camera.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_camera.h"
#include "driver/gpio.h"
#include "lcd.h"
#include <stdio.h>

/* 摄像头配置 */
static camera_config_t camera_config = {
    /* 引脚配置 */
    .pin_pwdn = CAM_PIN_PWDN,
    .pin_reset = CAM_PIN_RESET,
    .pin_xclk = CAM_PIN_XCLK,
    .pin_sccb_sda = CAM_PIN_SIOD,
    .pin_sccb_scl = CAM_PIN_SIOC,
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
    /* 图像配置 */
    .xclk_freq_hz = 24000000,   // 设置XCLK频率24M
    .ledc_timer = LEDC_TIMER_0,   // 选择定时器
    .ledc_channel = LEDC_CHANNEL_0,   // 选择pwm通道    
    .pixel_format = PIXFORMAT_RGB565,       /* 图像输出模式 和开发板的LCD显示图像格式一致*/
    .frame_size = FRAMESIZE_QVGA,        /* 图像输出大小 FRAMESIZE_QVGA*/
    .jpeg_quality = 5,                      /* 0-63，对于OV系列相机传感器，数量越少意味着质量越高 */
    .fb_count = 2,                          /* 当使用jpeg模式时，如果fb_count超过一个，则驱动程序将在连续模式下工作 缓冲区数量*/
    .grab_mode = CAMERA_GRAB_WHEN_EMPTY,    /*使用缓冲区的模式，此模式为当缓冲区空时写入数据 */
    .fb_location = CAMERA_FB_IN_PSRAM,      /*设置缓冲区存放位置 ，此时存放在PSRAM*/
};

/**
 * @brief       摄像头初始化
 * @param       cmd 传输的8位命令数据
 * @retval      无
 */
void camera_init(void)
{
    CAM_PWDN(0);
    CAM_RST(0);
    vTaskDelay(20);
    CAM_RST(1);
    vTaskDelay(20);

    esp_camera_init(&camera_config);

    // sensor_t * s = esp_camera_sensor_get();
    // 垂直翻转（上下颠倒）
    // s->set_vflip(s, 1);  // 1 = 开启翻转，0 = 关闭
    // 水平镜像（左右颠倒）
    // s->set_hmirror(s, 1);  // 1 = 开启镜像，0 = 关闭
}

unsigned long i = 0;
unsigned long j = 0;
camera_fb_t *fb = NULL;

/**
 * @brief       显示摄像头数据（RGB565）
 * @param       x：x轴坐标
 * @param       y：y轴坐标
 * @retval      无
 */

void camera_show(uint16_t x, uint16_t y)
{
    fb = esp_camera_fb_get();   

    if (fb == NULL) {
        printf("ERROR: fb is NULL!\n");
        return;
    }
    
    // printf("=== Camera Frame Info ===\n");
    // printf("fb->buf   = 0x%p\n", fb->buf);
    // printf("fb->len   = %u bytes\n", fb->len);
    // printf("fb->width = %u\n", fb->width);
    // printf("fb->height= %u\n", fb->height);
    // printf("fb->format= %u", fb->format);
    
    // switch(fb->format) {
    //     case PIXFORMAT_RGB565: printf(" (RGB565)\n"); break;
    //     case PIXFORMAT_YUV422: printf(" (YUV422)\n"); break;
    //     case PIXFORMAT_GRAYSCALE: printf(" (GRAYSCALE)\n"); break;
    //     case PIXFORMAT_JPEG: printf(" (JPEG)\n"); break;
    //     default: printf(" (Unknown)\n"); break;
    // }
    
    // 打印前32字节数据
    // printf("First 32 bytes: ");
    // for(int i = 0; i < 32 && i < fb->len; i++) {
    //     printf("%02X ", fb->buf[i]);
    //     if((i+1) % 16 == 0) printf("\n");
    // }
    // printf("\n");
    /*
    === Camera Frame Info ===
    fb->buf   = 0x0x3c050ab0
    fb->len   = 153600 bytes
    fb->width = 320
    fb->height= 240
    fb->format= 0 (RGB565)
    First 32 bytes: 53 AF 53 AF 5B AE 5B 8D 5B 8D 53 6C 53 8B 53 AC 
    53 8B 53 AB 4B AC 4B 8B 43 AC 43 CD 4B CE 43 AD     
    */

    lcd_show_picture(fb->buf);

    esp_camera_fb_return(fb);
    
    fb = NULL;
}
