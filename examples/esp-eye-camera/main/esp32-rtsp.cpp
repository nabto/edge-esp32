#include "CRtspSession.h"
#include "OV2640Streamer.h"
#include "esp_timer.h"
#include "driver/ledc.h"
#include "esp_log.h"
#include "esp_task_wdt.h"

#ifdef CONFIG_FRAMESIZE_QVGA
#define CAM_FRAMESIZE FRAMESIZE_QVGA
#endif
#ifdef CONFIG_FRAMESIZE_CIF
#define CAM_FRAMESIZE FRAMESIZE_CIF
#endif
#ifdef CONFIG_FRAMESIZE_VGA
#define CAM_FRAMESIZE FRAMESIZE_VGA
#endif
#ifdef CONFIG_FRAMESIZE_SVGA
#define CAM_FRAMESIZE FRAMESIZE_SVGA
#endif
#ifdef CONFIG_FRAMESIZE_XGA
#define CAM_FRAMESIZE FRAMESIZE_XGA
#endif
#ifdef CONFIG_FRAMESIZE_SXGA
#define CAM_FRAMESIZE FRAMESIZE_SXGA
#endif
#ifdef CONFIG_FRAMESIZE_UXGA
#define CAM_FRAMESIZE FRAMESIZE_UXGA
#endif

//Run `idf.py menuconfig` and set various other options in "ESP32CAM Configuration"

//common camera settings, see below for other camera options
#define CAM_QUALITY 12 //0 to 63, with higher being lower-quality (and less bandwidth), 12 seems reasonable

OV2640 cam;


// https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/esp_timer.html#_CPPv418esp_timer_get_timev
unsigned long millis()
{
    return (unsigned long) (esp_timer_get_time() / 1000ULL);
}

void delay(uint32_t ms)
{
    vTaskDelay(ms / portTICK_PERIOD_MS);
}

OV2640Streamer* streamer = NULL;
CRtspSession* session = NULL;

/**
 * Single client only
 */
void client_worker(void * client)
{

    int n=0;
    unsigned long lastMeasurement = millis();
    
    streamer = new OV2640Streamer((SOCKET)client, cam);
    session = new CRtspSession((SOCKET)client, streamer);

    unsigned long lastFrameTime = 0;
    const unsigned long msecPerFrame = (1000 / CONFIG_CAM_FRAMERATE);


    while (session->m_stopped == false)
    {
        session->handleRequests(0);

        unsigned long now = millis();
        if ((now > (lastFrameTime + msecPerFrame)) || (now < lastFrameTime))
        {
            session->broadcastCurrentFrame(now);
            lastFrameTime = now;

            unsigned long newMeasurement = millis();
            n++;
            if(n == 20) {
                printf("FPS: %3.2f \n", 1000.f/((newMeasurement-lastMeasurement)/20));
                n=0;
                lastMeasurement = newMeasurement;
            }

            
        }
        else
        {
            //let the system do something else for a bit
            //vTaskDelay(1);
            delay(5);
        }

    }

/*
#ifdef CONFIG_SINGLE_CLIENT_MODE
    esp_restart();
#else
*/
    //shut ourselves down
    delete streamer;
    delete session;
    session=NULL;
    streamer=NULL;
    printf("Stopping RTSP streamer and session\n");
    vTaskDelete(NULL);
// #endif
}

camera_config_t espeye_config{
    .pin_pwdn = -1, // FILXME: on the TTGO T-Journal I think this is GPIO 0
    .pin_reset = -1, // software reset will be performed
        
    .pin_xclk = 4,

    .pin_sscb_sda = 18,
    .pin_sscb_scl = 23,

    .pin_d7 = 36,
    .pin_d6 = 37,
    .pin_d5 = 38,
    .pin_d4 = 39,
    .pin_d3 = 35,
    .pin_d2 = 14,
    .pin_d1 = 13,
    .pin_d0 = 34,
    .pin_vsync = 5,
    .pin_href = 27,
    .pin_pclk = 25,
    .xclk_freq_hz = 20000000,
    .ledc_timer = LEDC_TIMER_0,
    .ledc_channel = LEDC_CHANNEL_0,
    .pixel_format = PIXFORMAT_JPEG,
    // .frame_size = FRAMESIZE_UXGA, // needs 234K of framebuffer space
    // .frame_size = FRAMESIZE_SXGA, // needs 160K for framebuffer
    // .frame_size = FRAMESIZE_XGA, // needs 96K or even smaller FRAMESIZE_SVGA - can work if using only 1 fb
    .frame_size = CAM_FRAMESIZE,
    .jpeg_quality = 12, //0-63 lower numbers are higher quality
    .fb_count = 2,      // if more than one i2s runs in continous mode.  Use only with jpeg
    .fb_location = CAMERA_FB_IN_PSRAM,
    .grab_mode = CAMERA_GRAB_WHEN_EMPTY,
    .sccb_i2c_port = -1
};


extern "C" void rtsp_server(void*);
void rtsp_server(void*)
{
    SOCKET server_socket;
    SOCKET client_socket;
    sockaddr_in server_addr;
    sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    TaskHandle_t xHandle = NULL;

    // Camera board configuration
    camera_config_t config = espeye_config;
    //camera_config_t config = esp32cam_config; // NOT TESTED
    //camera_config_t config = esp32cam_aithinker_config;

    config.frame_size = CAM_FRAMESIZE;
    config.jpeg_quality = CAM_QUALITY;
    cam.init(config);

    sensor_t * s = esp_camera_sensor_get();
    #ifdef CONFIG_CAM_HORIZONTAL_MIRROR
        s->set_hmirror(s, 1);
    #endif
    #ifdef CONFIG_CAM_VERTICAL_FLIP
        s->set_vflip(s, 1);
    #endif

    //setup other camera options
    //s->set_brightness(s, 0);     // -2 to 2
    s->set_contrast(s, 1);       // -2 to 2
    //s->set_saturation(s, 0);     // -2 to 2
    //s->set_special_effect(s, 2); // 0 to 6 (0 - No Effect, 1 - Negative, 2 - Grayscale, 3 - Red Tint, 4 - Green Tint, 5 - Blue Tint, 6 - Sepia)
    //s->set_whitebal(s, 0);       // 0 = disable , 1 = enable
    //s->set_awb_gain(s, 1);       // 0 = disable , 1 = enable
    s->set_wb_mode(s, 2);        // 0 to 4 - if awb_gain enabled (0 - Auto, 1 - Sunny, 2 - Cloudy, 3 - Office, 4 - Home)
    //s->set_exposure_ctrl(s, 1);  // 0 = disable , 1 = enable
    //s->set_aec2(s, 0);           // 0 = disable , 1 = enable
    //s->set_ae_level(s, 0);       // -2 to 2
    //s->set_aec_value(s, 300);    // 0 to 1200
    //s->set_gain_ctrl(s, 1);      // 0 = disable , 1 = enable
    //s->set_agc_gain(s, 0);       // 0 to 30
    //s->set_gainceiling(s, (gainceiling_t)0);  // 0 to 6
    //s->set_bpc(s, 0);            // 0 = disable , 1 = enable
    //s->set_wpc(s, 1);            // 0 = disable , 1 = enable
    //s->set_raw_gma(s, 1);        // 0 = disable , 1 = enable
    //s->set_lenc(s, 1);           // 0 = disable , 1 = enable
    //s->set_dcw(s, 1);            // 0 = disable , 1 = enable
    //s->set_colorbar(s, 0);       // 0 = disable , 1 = enable

    server_addr.sin_family      = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port        = htons(8554); // listen on RTSP port 8554
    server_socket               = socket(AF_INET, SOCK_STREAM, 0);

    int enable = 1;
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
    {
        printf("setsockopt(SO_REUSEADDR) failed! errno=%d\n", errno);
    }

    // bind our server socket to the RTSP port and listen for a client connection
    if (bind(server_socket, (sockaddr*)&server_addr, sizeof(server_addr)) != 0)
    {
        printf("bind failed! errno=%d\n", errno);
    }

    if (listen(server_socket, 5) != 0)
    {
        printf("listen failed! errno=%d\n", errno);
    }

    printf("\n\nrtsp://<IP_Address>:8554/mjpeg/1\n\n");

    // loop forever to accept client connections
    while (true)
    {
        client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &client_addr_len);
        if(session) {
            session->m_stopped=true;
        }
        // wait for stopped (and self cleanup)
        while(session) {
            printf("waiting for free session\n");
            vTaskDelay(500 / portTICK_PERIOD_MS);
        }

        printf("Client connected from: %s, creating new worker\n", inet_ntoa(client_addr.sin_addr));
        xTaskCreate(client_worker, "client-worker", 4096, (void*)client_socket, tskIDLE_PRIORITY, &xHandle);
        //xTaskCreatePinnedToCore(client_worker, "client-worker", 3584, (void*)client_socket, 35, &xHandle, 1);
    }

    close(server_socket);
}


