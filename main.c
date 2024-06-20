#include "lvgl/lvgl.h"
#include "lvgl/demos/lv_demos.h"
#include "lv_drivers/display/fbdev.h"
#include "lv_drivers/indev/evdev.h"
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <sys/time.h>

#define DISP_BUF_SIZE (128 * 1024)

int main(void)
{
    /*LittlevGL init*/
    lv_init();//LVGL程序的初始化


    //对液晶屏进行初始化
    /*Linux frame buffer device init*/
    fbdev_init();//液晶屏的初始化，就是用open打开液晶屏的驱动，然后ioctl获取了液晶屏的参数信息，mmap映射得到了首地址

    /*A small buffer for LittlevGL to draw the screen's content*/
    static lv_color_t buf[DISP_BUF_SIZE];

    /*Initialize a descriptor for the buffer*/
    static lv_disp_draw_buf_t disp_buf;
    lv_disp_draw_buf_init(&disp_buf, buf, NULL, DISP_BUF_SIZE);//把你刚才定义的那个buf注册到disp_buf里面

    /*Initialize and register a display driver*/
    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.draw_buf   = &disp_buf;
    disp_drv.flush_cb   = fbdev_flush;  //函数指针，fbdev_flush函数是LVGL画点函数
    disp_drv.hor_res    = 800;//分辨率
    disp_drv.ver_res    = 480;
    lv_disp_drv_register(&disp_drv);//把液晶屏注册到LVGL中



    //对液晶屏进行初始化 //第二个部分：对触摸屏进行初始化和注册
    evdev_init();//open打开触摸屏
    static lv_indev_drv_t indev_drv_1;//结构体变量
    lv_indev_drv_init(&indev_drv_1);    /*Basic initialization*/ //初始化刚才的结构体变量
    indev_drv_1.type = LV_INDEV_TYPE_POINTER;    //触摸类型

    /*This function will be called periodically (by the library) to get the mouse position and state*/
    indev_drv_1.read_cb = evdev_read;//函数指针，读取保存触摸屏坐标
    lv_indev_t *mouse_indev = lv_indev_drv_register(&indev_drv_1);//把触摸屏注册到LVGL




    /*Set a cursor for the mouse*/
    //创建鼠标光标
    LV_IMG_DECLARE(mouse_cursor_icon)
    lv_obj_t * cursor_obj = lv_img_create(lv_scr_act()); /*Create an image object for the cursor */
    lv_img_set_src(cursor_obj, &mouse_cursor_icon);           /*Set the image source*/
    lv_indev_set_cursor(mouse_indev, cursor_obj);             /*Connect the image  object to the driver*/


    /*Create a Demo*/
    //lv_demo_widgets();//屏蔽掉demo

    /*Handle LitlevGL tasks (tickless mode)*/
    while(1) {
        lv_timer_handler();//轮询方式处理事件
        usleep(5000);
    }

    return 0;
}

/*Set in lv_conf.h as `LV_TICK_CUSTOM_SYS_TIME_EXPR`*/
uint32_t custom_tick_get(void)
{
    static uint64_t start_ms = 0;
    if(start_ms == 0) {
        struct timeval tv_start;
        gettimeofday(&tv_start, NULL);
        start_ms = (tv_start.tv_sec * 1000000 + tv_start.tv_usec) / 1000;
    }

    struct timeval tv_now;
    gettimeofday(&tv_now, NULL);
    uint64_t now_ms;
    now_ms = (tv_now.tv_sec * 1000000 + tv_now.tv_usec) / 1000;

    uint32_t time_ms = now_ms - start_ms;
    return time_ms;
}
