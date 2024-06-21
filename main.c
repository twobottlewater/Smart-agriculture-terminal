#include "lvgl/lvgl.h"
#include "lvgl/demos/lv_demos.h"
#include "lv_drivers/display/fbdev.h"
#include "lv_drivers/indev/evdev.h"
#include <pthread.h>
#include <time.h>
#include <sys/time.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>  // 添加这个头文件以使用 usleep

#define DISP_BUF_SIZE (128 * 1024)

/*************** 
我创建的内容开始
***************/


static lv_obj_t * kb;
 lv_obj_t * user_ta;
 lv_obj_t * pass_ta;
static lv_obj_t * pass_vis_btn;



bool check_user_credentials(const char* username, const char* password) {
    int fd = open("./mima.txt", O_RDONLY);
    if (fd < 0) {
        perror("Failed to open file for reading");
        return false;
    }

    char buffer[256];
    ssize_t bytes_read;
    char file_username[128];
    char file_password[128];
    bool found = false;

    while ((bytes_read = read(fd, buffer, sizeof(buffer) - 1)) > 0) {
        buffer[bytes_read] = '\0';
        char *line = strtok(buffer, "\n");
        while (line != NULL) {
            if (sscanf(line, "%127[^@]@%127[^\n]", file_username, file_password) == 2) {
                if (strcmp(username, file_username) == 0 && strcmp(password, file_password) == 0) {
                    found = true;
                    break;
                }
            }
            line = strtok(NULL, "\n");
        }
        if (found) break;
    }

    close(fd);
    return found;
}


void show_hello_world_screen() {
    lv_obj_t * scr = lv_scr_act();
    lv_obj_clean(scr);  // 清除当前屏幕上的所有对象

    lv_obj_t * label = lv_label_create(scr);
    lv_label_set_text(label, "Hello, World!");
    lv_obj_center(label);
}


//登录事件回调函数
void login_event_handler(lv_event_t * e)
{
    const char * username = lv_textarea_get_text(user_ta);
    const char *password = lv_textarea_get_text(pass_ta);
    printf("字符串1:%s\n",username);
    printf("字符串2:%s\n",password);
    if (check_user_credentials(username, password)) {
        show_hello_world_screen();
    } else {
        printf("Invalid username or password\n");
    }
    // Implement login logic here
   // printf(" login-key has been passed\n");
}

// 注册事件
void register_event_handler(lv_event_t * e) 
{
    const char * username = lv_textarea_get_text(user_ta);
    const char *password = lv_textarea_get_text(pass_ta);
    int fd = open("mima.txt", O_WRONLY | O_CREAT | O_APPEND, 0777);
    if (fd < 0) {
        perror("Failed to open file for writing");
        return;
    }

    if (lseek(fd, 0, SEEK_END) > 0) {  // Check if file is not empty
        if (write(fd, "\n", 1) != 1) {  // Write a newline before appending new credentials
            perror("Failed to write newline");
            close(fd);
            return;
        }
    }

    char buffer[256];
    snprintf(buffer, sizeof(buffer), "%s@%s", username, password);
    if (write(fd, buffer, strlen(buffer)) != strlen(buffer)) {
        perror("Failed to write credentials");
    }

    close(fd);
    printf("User credentials saved\n");

}





static void ta_event_cb(lv_event_t * e) {
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * ta = lv_event_get_target(e);
    if(code == LV_EVENT_FOCUSED) {
        lv_keyboard_set_textarea(kb, ta);
        lv_obj_clear_flag(kb, LV_OBJ_FLAG_HIDDEN);
    }

    if(code == LV_EVENT_DEFOCUSED) {
        lv_keyboard_set_textarea(kb, NULL);
        lv_obj_add_flag(kb, LV_OBJ_FLAG_HIDDEN);
    }
}

//密码显示和隐藏的按钮的事件函数
void pass_vis_btn_event_cb(lv_event_t * e) {
    static bool pass_visible = false;
    pass_visible = !pass_visible;
    if (pass_visible) {
        lv_textarea_set_password_mode(pass_ta, false);
        lv_label_set_text(lv_obj_get_child(pass_vis_btn, 0), LV_SYMBOL_EYE_OPEN);
    } else {
        lv_textarea_set_password_mode(pass_ta, true);
        lv_label_set_text(lv_obj_get_child(pass_vis_btn, 0), LV_SYMBOL_EYE_CLOSE);
    }
}




//创建登录界面
void create_login_screen() {
    lv_obj_t * scr = lv_scr_act();  // 获取屏幕父对象

    // Create a background
    lv_obj_t * bg = lv_obj_create(scr);
    lv_obj_set_size(bg, 800, 480);
    lv_obj_center(bg);
    lv_obj_set_style_bg_color(bg, lv_color_hex(0xFFFFFF), 0);  //白色背景

    // Create a label for the title
    lv_obj_t * title = lv_label_create(scr);
    lv_label_set_text(title, "Login");
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 20);

    // Create a label for the username
    lv_obj_t * user_label = lv_label_create(scr);
    lv_label_set_text(user_label, "Username:");
    lv_obj_align(user_label, LV_ALIGN_CENTER, -150, -50);

    // Create a text area for the username
    user_ta = lv_textarea_create(scr);
    lv_obj_set_size(user_ta, 200, 40);
    lv_obj_align(user_ta, LV_ALIGN_CENTER, 50, -50);
    lv_textarea_set_placeholder_text(user_ta, "Enter your username");
    lv_obj_add_event_cb(user_ta, ta_event_cb, LV_EVENT_ALL, NULL);

    // Create a label for the password
    lv_obj_t * pass_label = lv_label_create(scr);
    lv_label_set_text(pass_label, "Password:");
    lv_obj_align(pass_label, LV_ALIGN_CENTER, -150, 10);

    // Create a text area for the password
    pass_ta = lv_textarea_create(scr);
    lv_obj_set_size(pass_ta, 200, 40);
    lv_obj_align(pass_ta, LV_ALIGN_CENTER, 50, 10);
    lv_textarea_set_password_mode(pass_ta, true);
    lv_textarea_set_placeholder_text(pass_ta, "Enter your password");
    lv_obj_add_event_cb(pass_ta, ta_event_cb, LV_EVENT_ALL, NULL);

    // C创建一个按钮来显示/隐藏密码
    pass_vis_btn = lv_btn_create(scr);
    lv_obj_set_size(pass_vis_btn, 60, 40);
    lv_obj_align_to(pass_vis_btn, pass_ta, LV_ALIGN_OUT_RIGHT_MID, 10, 0);
    lv_obj_add_event_cb(pass_vis_btn, pass_vis_btn_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t * pass_vis_label = lv_label_create(pass_vis_btn);
    lv_label_set_text(pass_vis_label,LV_SYMBOL_EYE_OPEN );
    lv_obj_center(pass_vis_label);



    // Create a button for login
    lv_obj_t * login_btn = lv_btn_create(scr);
    lv_obj_set_size(login_btn, 100, 50);
    lv_obj_align(login_btn, LV_ALIGN_CENTER, -60, 100);
    lv_obj_add_event_cb(login_btn,login_event_handler, LV_EVENT_CLICKED, NULL); 

    // Create a label on the login button
    lv_obj_t * btn_label = lv_label_create(login_btn);
    lv_label_set_text(btn_label, "Login");
    lv_obj_center(btn_label);

    // Create a button for register
    lv_obj_t * register_btn = lv_btn_create(scr);
    lv_obj_set_size(register_btn, 100, 50);
    lv_obj_align(register_btn, LV_ALIGN_CENTER, 60, 100);
    lv_obj_add_event_cb(register_btn, register_event_handler, LV_EVENT_CLICKED, NULL);

    // Create a label on the register button
    lv_obj_t * reg_btn_label = lv_label_create(register_btn);
    lv_label_set_text(reg_btn_label, "Register");
    lv_obj_center(reg_btn_label);

    // Create a keyboard
    kb = lv_keyboard_create(scr);
    lv_obj_add_flag(kb, LV_OBJ_FLAG_HIDDEN);  // Initially hidden
    lv_keyboard_set_textarea(kb, NULL);
    lv_obj_add_event_cb(kb, NULL, LV_EVENT_ALL, NULL);


    // 设置单行模式
    lv_textarea_set_one_line(user_ta,true);
    lv_textarea_set_one_line(pass_ta,true);
}


/*************** 
我创建的内容结束
***************/



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

    //我的 创建按钮的使用
    // 登录界面
     create_login_screen();

    
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
