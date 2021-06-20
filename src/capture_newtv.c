#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <getopt.h>
#include <time.h>
#include <signal.h>
#include <unistd.h>
#include <pthread.h>

#include <vtcapture/vtCaptureApi_c.h>

#include "hyperion_client.h"

#define MAX(a, b) (((a) > (b)) ? (a) : (b))

static struct option long_options[] = {
    {"width", optional_argument, 0, 'x'},
    {"height", optional_argument, 0, 'y'},
    {"address", required_argument, 0, 'a'},
    {"port", optional_argument, 0, 'p'},
    {"fps", optional_argument, 0, 'f'},
    {0, 0, 0, 0},
};


pthread_mutex_t frame_mutex = PTHREAD_MUTEX_INITIALIZER;

const VT_CALLER_T caller[24] = "com.webos.tbtest.cap";

VT_DRIVER *driver;
VT_CLIENTID_T client[128] = "00";

bool app_quit = false;
bool capture_initialized = false;
int isrunning = 0;
int done = 0;

_LibVtCaptureProperties props;

_LibVtCapturePlaneInfo plane;
int stride, x, y, w, h, xa, ya, wa, ha;
VT_REGION_T region;
VT_REGION_T activeregion;

_LibVtCaptureBufferInfo buff;
char *addr0, *addr1;
int size0, size1;
int comsize;  
char *rgbout;

VT_RESOLUTION_T resolution = {192, 108};
static const char *_address = NULL;
static int _port = 19400, _fps = 15, _framedelay_us = 0;

int capture_initialize();
void capture_terminate();
void capture_stop();
void capture_frame();
void send_picture();
void NV21_TO_RGB24(unsigned char *yuyv, unsigned char *rgb, int width, int height);

static void handle_signal(int signal)
{
    switch (signal)
    {
    case SIGINT:
        app_quit = true;
        hyperion_destroy();
        break;
    default:
        break;
    }
}

static void print_usage()
{
    printf("Usage: hyperion-webos -a ADDRESS [OPTION]...\n");
    printf("\n");
    printf("Grab screen content continously and send to Hyperion via flatbuffers server.\n");
    printf("\n");
    printf("  -x, --width           Width of video frame (default 192)\n");
    printf("  -y, --height          Height of video frame (default 108)\n");
    printf("  -a, --address         IP address of Hyperion server\n");
    printf("  -p, --port            Port of Hyperion flatbuffers server (default 19400)\n");
    printf("  -f, --fps             Framerate for sending video frames (default 15)\n");
}

static int parse_options(int argc, char *argv[])
{
    int opt, longindex;
    while ((opt = getopt_long(argc, argv, "x:y:a:p:f:", long_options, &longindex)) != -1)
    {
        switch (opt)
        {
        case 'x':
            resolution.w = atoi(optarg);
            break;
        case 'y':
            resolution.h = atoi(optarg);
            break;
        case 'a':
            _address = strdup(optarg);
            break;
        case 'p':
            _port = atol(optarg);
            break;
        case 'f':
            _fps = atoi(optarg);
            break;
        }
    }
    if (!_address)
    {
        fprintf(stderr, "Error! Address not specified.\n");
        print_usage();
        return 1;
    }
    if (_fps < 0 || _fps > 60)
    {
        fprintf(stderr, "Error! FPS should between 0 (unlimited) and 60.\n");
        print_usage();
        return 1;
    }
    if (_fps == 0)
        _framedelay_us = 0;
    else
        _framedelay_us = 1000000 / _fps;
    return 0;
}

int main(int argc, char *argv[])
{
    int ret;
    if ((ret = parse_options(argc, argv)) != 0)
    {
        return ret;
    }
    if (getenv("XDG_RUNTIME_DIR") == NULL)
    {
        setenv("XDG_RUNTIME_DIR", "/tmp/xdg", 1);
    }

    int dumping = 2;
    int capturex = 0;
    int capturey = 0;
    int buffer_count = 3;

    VT_DUMP_T dump = dumping;
    VT_LOC_T loc = {capturex, capturey};
    VT_BUF_T buf_cnt = buffer_count;

    props.dump = dump;
    props.loc = loc;
    props.reg = resolution;
    props.buf_cnt = buf_cnt;
    props.frm = _fps;

    if ((ret = capture_initialize()) != 0)
    {
        goto cleanup;
    }
    
    hyperion_client("webos", _address, _port, 150);
    signal(SIGINT, handle_signal);
    printf("Start connection loop\n");
    while (!app_quit)
    {
        capture_frame();
        if (hyperion_read() < 0)
        {
            fprintf(stderr, "Connection terminated.\n");
            app_quit = true;
        }
    }
    ret = 0;
cleanup:
    hyperion_destroy();
    if (isrunning == 0){
        capture_terminate();
    }else{
        capture_stop();
    }
    return ret;
}

int capture_initialize()
{
    driver = vtCapture_create();
    fprintf(stderr, "Driver created!\n");

    done = vtCapture_init(driver, caller, client);
    if (done != 0) {
        fprintf(stderr, "vtCapture_init failed: %x\nQuitting...\n", done);
        return -1;
    }
    fprintf(stderr, "vtCapture_init done!\nCaller_ID: %s Client ID: %s \n", caller, client);

    done = vtCapture_preprocess(driver, client, &props);
    if (done != 0) {
        fprintf(stderr, "vtCapture_preprocess failed: %x\nQuitting...\n", done);
        return -1;
    }
    fprintf(stderr, "vtCapture_preprocess done!\n");

    done = vtCapture_planeInfo(driver, client, &plane);
    if (done == 0 ) {
        stride = plane.stride;

        region = plane.planeregion;
        x = region.a, y = region.b, w = region.c, h = region.d;

        activeregion = plane.activeregion;
        xa = activeregion.a, ya = activeregion.b, wa = activeregion.c, ha = activeregion.d;
    }else{
        fprintf(stderr, "vtCapture_planeInfo failed: %x\nQuitting...\n", done);
        return -1;
    }
    fprintf(stderr, "vtCapture_planeInfo done!\nstride: %d\nRegion: x: %d, y: %d, w: %d, h: %d\nActive Region: x: %d, y: %d w: %d h: %d\n", stride, x, y, w, h, xa, ya, wa, ha);

    done = vtCapture_process(driver, client);
    if (done == 0){
        isrunning = 1;
        capture_initialized = true;
    }else{
        isrunning = 0;
        fprintf(stderr, "vtCapture_process failed: %x\nQuitting...\n", done);
        return -1;
    }
    fprintf(stderr, "vtCapture_process done!\n");

    sleep(2);

    done = vtCapture_currentCaptureBuffInfo(driver, &buff);
    if (done == 0 ) {
        addr0 = buff.start_addr0;
        addr1 = buff.start_addr1;
        size0 = buff.size0;
        size1 = buff.size1;
    }else{
        fprintf(stderr, "vtCapture_currentCaptureBuffInfo failed: %x\nQuitting...\n", done);
        capture_stop();
        return -1;
    }
    fprintf(stderr, "vtCapture_currentCaptureBuffInfo done!\naddr0: %p addr1: %p size0: %d size1: %d\n", addr0, addr1, size0, size1);

    comsize = size0+size1; 
    return 0;
}

void capture_stop()
{
    int done;

    isrunning = 0;
    done = vtCapture_stop(driver, client);
    if (done != 0)
    {
        fprintf(stderr, "vtCapture_stop failed: %x\nQuitting...\n", done);
        capture_terminate();
        return;
    }
    fprintf(stderr, "vtCapture_stop done!\n");
    capture_terminate();
    return;
}

void capture_terminate()
{
    fprintf(stderr, "-- Quit called! --\n");
    int done;
    done = vtCapture_postprocess(driver, client);
        if (done == 0){
            fprintf(stderr, "Quitting: vtCapture_postprocess done!\n");
            done = vtCapture_finalize(driver, client);
            if (done == 0) {
                fprintf(stderr, "Quitting: vtCapture_finalize done!\n");
                vtCapture_release(driver);
                fprintf(stderr, "Quitting: Driver released!\n");
                memset(&client,0,127);
                fprintf(stderr, "Quitting!\n");
                return;
            }
            fprintf(stderr, "Quitting: vtCapture_finalize failed: %x\n", done);
        }
    vtCapture_finalize(driver, client);
    vtCapture_release(driver);
    fprintf(stderr, "Quitting with errors: %x!\n", done);
    return;
}

uint64_t getticks_us()
{
    struct timespec tp;
    clock_gettime(CLOCK_MONOTONIC, &tp);
    return tp.tv_sec * 1000000 + tp.tv_nsec / 1000;
}

void capture_frame()
{
    do{
        if (!capture_initialized){
            return;
        }
        pthread_mutex_lock(&frame_mutex);
        static uint32_t framecount = 0;
        static uint64_t last_ticks = 0, fps_ticks = 0, dur_gentexture = 0, dur_readframe = 0, dur_sendframe = 0;
        uint64_t ticks = getticks_us(), trace_start, trace_end;
        if (ticks - last_ticks < _framedelay_us)
        {
            pthread_mutex_unlock(&frame_mutex);
            return;
        }
        last_ticks = ticks;
        char *combined = (char *) malloc(comsize);
        int rgbsize = sizeof(combined)*w*h*3;
        rgbout = (char *) malloc(rgbsize);
        memcpy(combined, addr0, size0);
        memcpy(combined+size0, addr1, size1);

        NV21_TO_RGB24(combined, rgbout, w, h);

        trace_end = getticks_us();
        dur_gentexture += trace_end - trace_start;
        trace_start = trace_end;

        trace_end = getticks_us();
        dur_readframe += trace_end - trace_start;
        trace_start = trace_end;
        send_picture();
        trace_end = getticks_us();
        dur_sendframe += trace_end - trace_start;
        trace_start = trace_end;
        framecount++;
        if (fps_ticks == 0)
        {
            fps_ticks = ticks;
        }
        else if (ticks - fps_ticks >= 1000000)
        {
            printf("[Stat] Send framerate: %d FPS. gen %d us, read %d us, send %d us\n",
                framecount, dur_gentexture, dur_readframe, dur_sendframe);
            framecount = 0;
            dur_gentexture = 0;
            dur_readframe = 0;
            dur_sendframe = 0;
            fps_ticks = ticks;
        }
        pthread_mutex_unlock(&frame_mutex);
        free(rgbout);
        free(combined);
    }while(app_quit);
}

void send_picture()
{
    int width = resolution.w, height = resolution.h;

    if (hyperion_set_image(rgbout, width, height) != 0)
    {
        fprintf(stderr, "Write timeout\n");
        hyperion_destroy();
        app_quit = true;
    }
}

//Credits: https://www.programmersought.com/article/18954751423/
void NV21_TO_RGB24(unsigned char *yuyv, unsigned char *rgb, int width, int height)
{
        const int nv_start = width * height ;
        int  index = 0, rgb_index = 0;
        uint8_t y, u, v;
        int r, g, b, nv_index = 0,i, j;
 
        for(i = 0; i < height; i++){
            for(j = 0; j < width; j ++){

                nv_index = i / 2  * width + j - j % 2;
 
                y = yuyv[rgb_index];
                u = yuyv[nv_start + nv_index ];
                v = yuyv[nv_start + nv_index + 1];
 
                r = y + (140 * (v-128))/100;  //r
                g = y - (34 * (u-128))/100 - (71 * (v-128))/100; //g
                b = y + (177 * (u-128))/100; //b
 
                if(r > 255)   r = 255;
                if(g > 255)   g = 255;
                if(b > 255)   b = 255;
                if(r < 0)     r = 0;
                if(g < 0)     g = 0;
                if(b < 0)     b = 0;
 
                index = rgb_index % width + (height - i - 1) * width;

                rgb[index * 3+0] = r;
                rgb[index * 3+1] = g;
                rgb[index * 3+2] = b;
                 
                rgb_index++;

            }
        }
}
