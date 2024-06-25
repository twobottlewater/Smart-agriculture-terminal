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
#include <sys/socket.h>
#include <sys/types.h>          /* 网络头文件 */
#include <netinet/in.h>
#include <arpa/inet.h>
#include "cJSON.h" // cJSON库头文件，需提前安装和包含
#include <netdb.h>



#define DISP_BUF_SIZE (128 * 1024)
#define TEMP_UPDATE_INTERVAL 5000  // 以毫秒为单位的温度更新间隔
#define CHART_UPDATE_INTERVAL 10000  // 图表更新间隔以毫秒为单位
#define MAX_TEMP_VALUES 5 //每组最大数据


#define SERVER_IP "192.168.1.160"  //Ubuntu主机服务器端的ip
#define SERVER_PORT 12000          //Ubuntu主机服务器的端口号

#define CLIENT_IP "192.168.1.161"  //我自己这边端的ip
#define CLIENT_PORT 20001          //客户端的端口号这个主要用来开发板发
#define LED_DEVICE "/dev/led_drv"



    /*************** 
    我创建的内容开始
    ***************/
    int led_fd;//操作灯的文件描述符
    int flush_button_state = 0; // 0: off, 1: on
    int light_button_state = 0; // 0: off, 1: on

    // 定义灯控制函数
    void control_led(int led, int state) {
        char buf[2];
        buf[0] = state; // 亮灭状态
        buf[1] = led;   // 第几盏灯
        write(led_fd, buf, sizeof(buf));
    }

    //登录界面的
    static lv_obj_t * kb;
    lv_obj_t * user_ta;
    lv_obj_t * pass_ta;
    static lv_obj_t * pass_vis_btn;

    //日常运行界面的
    static lv_obj_t *chart1;
    static lv_obj_t *chart2;
    static lv_obj_t *time_label;
    static lv_obj_t *temp_label;
    static lv_obj_t *humid_label;
    static lv_obj_t *lux_label;
    static lv_obj_t *flush_button;
    static lv_obj_t *light_button;
    static lv_chart_series_t *ser1;
    static lv_chart_series_t *ser2;
    static lv_obj_t *chart;
    static lv_chart_series_t *ser;
    lv_obj_t *city_label;
    lv_obj_t *wind_power_label;
    lv_obj_t *rain_label;
    static int temp_index = 0;
    static int temp_values[MAX_TEMP_VALUES];
    static int humid_values[MAX_TEMP_VALUES];
    static int lux_values[MAX_TEMP_VALUES];

    //来一个结构体
    // 天气数据结构
    typedef struct 
    {
        char city[32];
        char wind_power[32];
        char rain[8];
    } WeatherData;
    //把两个按钮的文本设置独立一下
    lv_obj_t *flush_btn_label;
    lv_obj_t *light_btn_label;

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

    //------------展示hello world--------
    void show_hello_world_screen() 
    {
    lv_obj_t * scr = lv_scr_act();
        lv_obj_clean(scr);  // 清除当前屏幕上的所有对象

        // 创建一个容器，作为文本的背景
        lv_obj_t * cont = lv_obj_create(scr);
        lv_obj_set_size(cont, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
        lv_obj_set_style_pad_all(cont, 20, 0); // 内部填充
        lv_obj_set_style_bg_color(cont, lv_color_hex(0xFFFFFF), 0); // 背景颜色
        lv_obj_set_style_border_width(cont, 2, 0); // 边框宽度
        lv_obj_set_style_border_color(cont, lv_color_hex(0xFFFFFF), 0); // 边框颜色
        lv_obj_set_style_radius(cont, 10, 0); // 圆角半径
        lv_obj_center(cont);

        // 添加阴影效果
        lv_obj_set_style_shadow_width(cont, 8, 0); // 阴影宽度
        lv_obj_set_style_shadow_color(cont, lv_color_hex(0x808080), 0); // 阴影颜色

        // 创建标签，并将其作为容器的子对象
        lv_obj_t * label = lv_label_create(cont);
        lv_label_set_text(label, "Smart Agriculture \nMonitoring Center");
        lv_obj_set_style_text_font(label, &lv_font_montserrat_24, 0); // 设置字体大小
        lv_obj_set_style_text_color(label, lv_color_hex(0xD32F2F), 0); // 设置字体颜色为红色
        lv_obj_align(label, LV_ALIGN_CENTER, 0, 0); // 居中对齐
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
            sleep(1);
            create_chart_screen();
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




    //--键盘事件函数---
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

    //机器人的按钮事件 按钮点击事件回调函数
    static void flush_btn_event_cb(lv_event_t * e) {
        lv_obj_t * btn = lv_event_get_target(e);
        flush_button_state = !flush_button_state; // 切换按钮状态
        lv_label_set_text(lv_obj_get_child(btn, 0), flush_button_state ? "OFF" : "ON");

        if(flush_button_state) {
            control_led(7, 0); // 关闭D7灯
            lv_obj_set_style_bg_color(flush_button, lv_color_hex(0xFFA000), 0); // 设置按钮背景颜色
        } else {
            control_led(7, 1); // 打开D7灯
            lv_obj_set_style_bg_color(flush_button, lv_color_hex(0x007AFF), 0); // 设置按钮背景颜色
        }
    }

    //灯的按钮事件 按钮点击事件回调函数
    static void light_btn_event_cb(lv_event_t * e) 
    {
        lv_obj_t * btn = lv_event_get_target(e);
        //切换按钮状态
        light_button_state=!light_button_state;
        lv_label_set_text(lv_obj_get_child(btn, 0), light_button_state ? "OFF" : "ON");

    
        if(light_button_state) 
        {
            control_led(8, 0); // 关闭D8灯
            lv_obj_set_style_bg_color(light_button, lv_color_hex(0xFFA000), 0); // 设置按钮背景颜色
        } else {
            control_led(8, 1); // 打开D8灯
            lv_obj_set_style_bg_color(light_button, lv_color_hex(0x007AFF), 0); // 设置按钮背景颜色
        }
    }



    //创建登录界面
    void create_login_screen()
    {  
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

//===========================温度的折线图==============================
static void update_chart(lv_timer_t * timer) {
    for (int i = 0; i < MAX_TEMP_VALUES; i++) {
        lv_chart_set_next_value(chart1, ser1, temp_values[i]);
        lv_chart_set_next_value(chart2, ser2, humid_values[i]);
    }
}


//----------------时间获取函数-------------------

static void update_time(lv_timer_t * timer) {
    time_t raw_time;
    struct tm *time_info;
    char time_buffer[64];

    time(&raw_time);
    time_info = localtime(&raw_time);
    strftime(time_buffer, sizeof(time_buffer), "%Y-%m-%d %H:%M:%S", time_info);

    lv_label_set_text(time_label, time_buffer);
}

void update_system_time(const char *datetime) {
    char command[64];

    // Construct the command string
    snprintf(command, sizeof(command), "date -s \"%s\"", datetime);

    // Execute the command to update system time
    int result = system(command);

    if (result == -1) {
        perror("system");
    } else {
        printf("System time updated to: %s\n", datetime);
    }
}


//-----------随机生成温度值的函数----------------------
static void generate_random_temp(lv_timer_t *timer) {
   for (int i = 0; i < MAX_TEMP_VALUES; i++) {
        temp_values[i] = rand() % 51; // Random temperature between 0 and 50
        humid_values[i] = rand() % 101; // Random humidity between 0 and 100
        lux_values[i] = rand() % 2001; // Random Lux between 0 and 2000
    }
    lv_label_set_text_fmt(temp_label, "temperature : %d°C", temp_values[MAX_TEMP_VALUES - 1]);
    lv_label_set_text_fmt(humid_label, "Humidity data: %d%%", humid_values[MAX_TEMP_VALUES - 1]);
    lv_label_set_text_fmt(lux_label, "Lux: %d lx", lux_values[MAX_TEMP_VALUES - 1]);
}



void create_chart_screen() {
    lv_obj_t *scr = lv_scr_act();  // 获取屏幕父对象

    // 创建时间标签
    time_label = lv_label_create(scr);
    lv_label_set_text(time_label, "Loading...");
    lv_obj_set_style_text_font(time_label, &lv_font_montserrat_24, 0); // 设置字体大小
    lv_obj_align(time_label, LV_ALIGN_TOP_MID, 0, 10); // 顶部居中，稍微偏移

    // 创建垂直容器，用于排列标签
    lv_obj_t * label_cont = lv_obj_create(scr);
    lv_obj_set_width(label_cont, LV_SIZE_CONTENT);
    lv_obj_set_height(label_cont, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(label_cont, LV_FLEX_FLOW_COLUMN); // 使用垂直排列
    lv_obj_set_style_pad_all(label_cont, 10, 0); // 设置容器内边距
    lv_obj_align(label_cont, LV_ALIGN_TOP_LEFT, 10, 40); // 顶部左侧对齐，稍微偏移



    // 创建温度数据标签
    temp_label = lv_label_create(label_cont);
    lv_label_set_text(temp_label, "Temperature: xxx°C");
    lv_obj_set_style_text_font(temp_label, &lv_font_montserrat_24, 0); // 设置字体大小
    lv_obj_set_style_pad_bottom(temp_label, 10, 0); // 设置标签之间的间距

    // 创建湿度数据标签
    humid_label = lv_label_create(label_cont);
    lv_label_set_text(humid_label, "Humidity: xxx%");
    lv_obj_set_style_text_font(humid_label, &lv_font_montserrat_24, 0); // 设置字体大小
    lv_obj_set_style_pad_bottom(humid_label, 10, 0); // 设置标签之间的间距

    // 创建光照强度数据标签
    lux_label = lv_label_create(label_cont);
    lv_label_set_text(lux_label, "Lux: xxx lx");
    lv_obj_set_style_text_font(lux_label, &lv_font_montserrat_24, 0); // 设置字体大小

//---------------------------------------------------------------
    // 创建一个水平容器用于放置标签和按钮 -----机器人按钮
    lv_obj_t *flush_cont = lv_obj_create(scr);
    lv_obj_set_width(flush_cont, LV_SIZE_CONTENT);
    lv_obj_set_height(flush_cont, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(flush_cont, LV_FLEX_FLOW_ROW); // 使用水平排列
    lv_obj_set_style_pad_all(flush_cont, 10, 0); // 设置容器内边距
    lv_obj_align(flush_cont, LV_ALIGN_TOP_RIGHT, -20, 100);

    // 设置容器样式
    lv_obj_set_style_bg_color(flush_cont, lv_color_hex(0xF0F0F0), 0); // 设置背景颜色
    lv_obj_set_style_border_width(flush_cont, 2, 0); // 设置边框宽度
    lv_obj_set_style_border_color(flush_cont, lv_color_hex(0xCCCCCC), 0); // 设置边框颜色
    lv_obj_set_style_radius(flush_cont, 10, 0); // 设置圆角半径

    lv_obj_t *flush_label = lv_label_create(flush_cont);
    lv_label_set_text(flush_label, "Watering\nrobot: ");
    lv_obj_set_style_text_font(flush_label, &lv_font_montserrat_24, 0); // 设置字体大小
    lv_obj_set_style_text_align(flush_label, LV_TEXT_ALIGN_CENTER, 0); // 设置文本居中对齐

    flush_button = lv_btn_create(flush_cont);
    lv_obj_set_size(flush_button, 100, 50); // 设置按钮大小
    lv_obj_set_style_bg_color(flush_button, lv_color_hex(0x007AFF), 0); // 设置按钮背景颜色
    lv_obj_set_style_border_width(flush_button, 2, 0); // 设置按钮边框宽度
    lv_obj_set_style_border_color(flush_button, lv_color_hex(0x005BBB), 0); // 设置按钮边框颜色
    lv_obj_set_style_radius(flush_button, 10, 0); // 设置按钮圆角半径

    flush_btn_label = lv_label_create(flush_button);
    lv_label_set_text(flush_btn_label, "ON");
    lv_obj_set_style_text_font(flush_btn_label, &lv_font_montserrat_24, 0); // 设置按钮字体大小
    lv_obj_set_style_text_align(flush_btn_label, LV_TEXT_ALIGN_CENTER, 0); // 设置文本居中对齐
    lv_obj_set_style_text_color(flush_btn_label, lv_color_hex(0xFFFFFF), 0); // 设置按钮文字颜色
    lv_obj_add_event_cb(flush_button, flush_btn_event_cb, LV_EVENT_CLICKED, NULL);

    // 创建一个水平容器用于放置标签和按钮-------灯光按钮
    lv_obj_t *light_cont = lv_obj_create(scr);
    lv_obj_set_width(light_cont, LV_SIZE_CONTENT);
    lv_obj_set_height(light_cont, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(light_cont, LV_FLEX_FLOW_ROW); // 使用水平排列
    lv_obj_set_style_pad_all(light_cont, 10, 0); // 设置容器内边距
    lv_obj_align(light_cont, LV_ALIGN_TOP_RIGHT, -20, 200);

    // 设置容器样式
    lv_obj_set_style_bg_color(light_cont, lv_color_hex(0xF0F0F0), 0); // 设置背景颜色
    lv_obj_set_style_border_width(light_cont, 2, 0); // 设置边框宽度
    lv_obj_set_style_border_color(light_cont, lv_color_hex(0xCCCCCC), 0); // 设置边框颜色
    lv_obj_set_style_radius(light_cont, 10, 0); // 设置圆角半径

    lv_obj_t *light_label = lv_label_create(light_cont);
    lv_label_set_text(light_label, "Light: ");
    lv_obj_set_style_text_font(light_label, &lv_font_montserrat_24, 0); // 设置字体大小
    lv_obj_set_style_text_align(light_label, LV_TEXT_ALIGN_CENTER, 0); // 设置文本居中对齐

    light_button = lv_btn_create(light_cont);
    lv_obj_set_size(light_button, 100, 50); // 设置按钮大小
    lv_obj_set_style_bg_color(light_button, lv_color_hex(0x007AFF), 0); // 设置按钮背景颜色
    lv_obj_set_style_border_width(light_button, 2, 0); // 设置按钮边框宽度
    lv_obj_set_style_border_color(light_button, lv_color_hex(0x005BBB), 0); // 设置按钮边框颜色
    lv_obj_set_style_radius(light_button, 10, 0); // 设置按钮圆角半径

    light_btn_label = lv_label_create(light_button);
    lv_label_set_text(light_btn_label, "ON");
    lv_obj_set_style_text_font(light_btn_label, &lv_font_montserrat_24, 0); // 设置按钮字体大小
    lv_obj_set_style_text_align(light_btn_label, LV_TEXT_ALIGN_CENTER, 0); // 设置文本居中对齐
    lv_obj_set_style_text_color(light_btn_label, lv_color_hex(0xFFFFFF), 0); // 设置按钮文字颜色
    lv_obj_add_event_cb(light_button, light_btn_event_cb, LV_EVENT_CLICKED, NULL);


    // 创建天气数据容器
    lv_obj_t *weather_cont = lv_obj_create(scr);
    lv_obj_set_width(weather_cont, LV_SIZE_CONTENT);
    lv_obj_set_height(weather_cont, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(weather_cont, LV_FLEX_FLOW_COLUMN); // 使用垂直排列
    lv_obj_set_style_pad_all(weather_cont, 10, 0); // 设置容器内边距
    lv_obj_align(weather_cont, LV_ALIGN_TOP_LEFT, 50, 200); // 根据需要调整位置

    // 创建城市标签
    city_label = lv_label_create(weather_cont);
    lv_label_set_text(city_label, "City: Unknown");
    lv_obj_set_style_text_font(city_label, &lv_font_montserrat_14, 0); // 设置字体大小
    lv_obj_set_style_pad_bottom(city_label, 10, 0); // 设置标签之间的间距

    // 创建风力标签
    wind_power_label = lv_label_create(weather_cont);
    lv_label_set_text(wind_power_label, "Wind Power: Unknown");
    lv_obj_set_style_text_font(wind_power_label, &lv_font_montserrat_14, 0); // 设置字体大小
    lv_obj_set_style_pad_bottom(wind_power_label, 10, 0); // 设置标签之间的间距

    // 创建降水概率标签
    rain_label = lv_label_create(weather_cont);
    lv_label_set_text(rain_label, "Rain: Unknown");
    lv_obj_set_style_text_font(rain_label, &lv_font_montserrat_14, 0); // 设置字体大小
    WeatherData data;
    get_weather_data(&data);
    strcpy( data.city, "guangzhou");
    strcpy( data.wind_power, "3-5 l");
    strcpy( data.rain, "80%");
    lv_label_set_text_fmt(city_label, "City: %s", data.city);
    lv_label_set_text_fmt(wind_power_label, "Wind Power: %s", data.wind_power);
    lv_label_set_text_fmt(rain_label, "Rain: %s", data.rain);
//---------------------------------------------------------------
    // 创建第一个折线图
    chart1 = lv_chart_create(scr);
    lv_obj_set_size(chart1, 300, 150);  // Adjust size as needed
    lv_obj_align(chart1, LV_ALIGN_BOTTOM_LEFT, 38, -10); // Bottom left with some offset
    lv_chart_set_type(chart1, LV_CHART_TYPE_LINE); //设置图表类型为折线图
    lv_chart_set_point_count(chart1, MAX_TEMP_VALUES); // Set the number of points
    lv_chart_set_update_mode(chart1, LV_CHART_UPDATE_MODE_SHIFT); // Shift when adding new points

    // 设置y轴刻度和标签
    lv_chart_set_range(chart1, LV_CHART_AXIS_PRIMARY_Y, 0, 50);
    lv_chart_set_axis_tick(chart1, LV_CHART_AXIS_PRIMARY_Y, 10, 5, 6, 5, true, 50); // 0-50 with major ticks and labels

    // 设置x轴刻度和标签
    lv_chart_set_axis_tick(chart1, LV_CHART_AXIS_PRIMARY_X, 10, 5, MAX_TEMP_VALUES / 2, MAX_TEMP_VALUES / 2, true, 50);

    // 设置背景颜色
    lv_obj_set_style_bg_color(chart1, lv_palette_main(LV_PALETTE_LIGHT_BLUE), LV_PART_MAIN);

    // 去掉背景网格线
    lv_obj_set_style_bg_opa(chart1, LV_OPA_100, LV_PART_MAIN);
    lv_obj_set_style_line_color(chart1, lv_color_hex(0xFFFFFF), LV_PART_TICKS); // 设置刻度线颜色为背景色，从而隐藏它们
    lv_obj_set_style_size(chart1, 0, LV_PART_TICKS); // 设置刻度线的大小为0，从而隐藏它们

    // 创建第一个系列
    ser1 = lv_chart_add_series(chart1, lv_palette_main(LV_PALETTE_RED), LV_CHART_AXIS_PRIMARY_Y);

    // ---------------------在折线图上方创建温度标签-----------------------
    lv_obj_t *label1 = lv_label_create(chart1); // 将 label1 放置在 chart1 内部
    lv_label_set_text(label1, "Temperature");
    lv_obj_align(label1, LV_ALIGN_OUT_TOP_LEFT, 0, -10); // 相对于 chart1 的顶部左侧，调整偏移量
    lv_obj_set_style_text_font(label1, &lv_font_montserrat_14, 0); // 设置字体大小

    // 创建第二个折线图
    chart2 = lv_chart_create(scr);
    lv_obj_set_size(chart2, 300, 150);  // Adjust size as needed
    lv_obj_align(chart2, LV_ALIGN_BOTTOM_RIGHT, -10, -10); // Bottom right with some offset
    lv_chart_set_type(chart2, LV_CHART_TYPE_LINE); // Set chart type to line
    lv_chart_set_point_count(chart2, MAX_TEMP_VALUES); // Set the number of points
    lv_chart_set_update_mode(chart2, LV_CHART_UPDATE_MODE_SHIFT); // Shift when adding new points

    // 设置y轴刻度和标签
    lv_chart_set_range(chart2, LV_CHART_AXIS_PRIMARY_Y, 0, 100);
    lv_chart_set_axis_tick(chart2, LV_CHART_AXIS_PRIMARY_Y, 10, 5, 6, 5, true, 50); // 0-100 with major ticks and labels

    // 设置x轴刻度和标签
    lv_chart_set_axis_tick(chart2, LV_CHART_AXIS_PRIMARY_X, 10, 5, MAX_TEMP_VALUES / 2, MAX_TEMP_VALUES / 2, true, 50);

    // 设置背景颜色
    lv_obj_set_style_bg_color(chart2, lv_palette_main(LV_PALETTE_LIGHT_GREEN), LV_PART_MAIN);

    // 去掉背景网格线
    lv_obj_set_style_bg_opa(chart2, LV_OPA_100, LV_PART_MAIN);
    lv_obj_set_style_line_color(chart2, lv_color_hex(0xFFFFFF), LV_PART_TICKS); // 设置刻度线颜色为背景色，从而隐藏它们
    lv_obj_set_style_size(chart2, 0, LV_PART_TICKS); // 设置刻度线的大小为0，从而隐藏它们

    // 创建第二个系列
    ser2 = lv_chart_add_series(chart2, lv_palette_main(LV_PALETTE_BLUE), LV_CHART_AXIS_PRIMARY_Y);

    //在图表内部定义一个标题
    lv_obj_t *label2 = lv_label_create(chart2); // 将 label2 放置在 chart2 内部
    lv_label_set_text(label2, "Humidity"); // 设置标题文本
    lv_obj_align(label2, LV_ALIGN_OUT_TOP_LEFT, 0, -10); // 相对于 chart1 的顶部左侧，调整偏移量
    lv_obj_set_style_text_font(label2, &lv_font_montserrat_14, 0); // 设置字体大小



    // 初始化温湿度值
    for (int i = 0; i < MAX_TEMP_VALUES; i++) {
        temp_values[i] = 0;
        humid_values[i] = 0;
        lv_chart_set_next_value(chart1, ser1, temp_values[i]);
        lv_chart_set_next_value(chart2, ser2, humid_values[i]);
    }

    // 创建定时器来生成随机温湿度值
    lv_timer_create(generate_random_temp, TEMP_UPDATE_INTERVAL, NULL);

    // 创建定时器来更新折线图
    lv_timer_create(update_chart, CHART_UPDATE_INTERVAL, NULL);

    // 创建定时器来更新时间标签
    lv_timer_create(update_time, 1000, NULL); // 每秒更新一次时间
}


//----------------创建的一个线程来接受服务器发来的数据--------------------
void* thread_Recv(void* arg)
{
    //创建套接字
    int udp_recv_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (udp_recv_socket == -1)
    {
        perror("socket");
        return NULL;
    }

    // 设置端口号复用--重复利用
    int opt = 1;
    setsockopt(udp_recv_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in ownAddr;
    ownAddr.sin_family = AF_INET;
    ownAddr.sin_port = htons(CLIENT_PORT);
    ownAddr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(udp_recv_socket, (struct sockaddr *)&ownAddr, sizeof(ownAddr)) == -1)
    {
        perror("bind");
        close(udp_recv_socket);
        return NULL;
    }
    int led_n,temp_n,bot_n;
 
    while (1)
    {
        char buf[1024] = {0};
        struct sockaddr_in addr;
        int addrlen = sizeof(addr);
        ssize_t ret = recvfrom(udp_recv_socket, buf, sizeof(buf), 0, (struct sockaddr *)&addr, &addrlen);
        if (ret == -1)
        {
            perror("recvfrom");
            continue;
        }
         printf("Received: %s\n", buf);
        // 得判断一下等下来的数据是不是时间数据
        if (strncmp(buf, "TIME:", 5) == 0)
        {
            printf("b捕捉到时间数据\n");
            const char *time_part = buf + 5;
            //  // 提取时间数据并传递给 update_time_once 函数
            // char *received_time_str = buf + 5;
            // update_time_once(received_time_str);
            struct tm received_time_info;
            if (strptime(time_part, "%Y-%m-%d %H:%M:%S", &received_time_info) == NULL)
            {
            perror("strptime");
            return;
            }
           update_system_time(time_part);
        }
        //没问题就解析字符串
        // 解析接收到的 JSON 数据
        cJSON *root = cJSON_Parse(buf);
        if (root == NULL)
        {
            printf("Error parsing JSON: %s\n", cJSON_GetErrorPtr());
            continue;
        }

        // 解析并处理具体的指令
        cJSON *paras = cJSON_GetObjectItem(root, "paras");
        if (paras != NULL)
        {


            cJSON *temp = cJSON_GetObjectItem(paras, "temp");
            cJSON *led = cJSON_GetObjectItem(paras, "led");
            cJSON *bot = cJSON_GetObjectItem(paras, "bot");
            if(temp != NULL)
            {
                temp_n=temp->valueint;
                temp_values[MAX_TEMP_VALUES - 1] = temp_n;
                printf("temp里面是:%d\n",temp->valueint);
                lv_label_set_text_fmt(temp_label, "temperature : %d°C", temp_values[MAX_TEMP_VALUES - 1]);
            }

            if (led != NULL)
            {
                led_n=led->type;
                printf("led里面是:%d\n",led->type);
                if (led_n!=0)
                {
                    light_button_state=0;
                    control_led(8, 1); // 打开D8灯
                    lv_obj_set_style_bg_color(light_button, lv_color_hex(0x007AFF), 0); // 设置按钮背景颜色
                    lv_label_set_text(light_btn_label, "ON");
                    /* code */
                }
                else
                {
                    light_button_state=1;
                    control_led(8, 0); // 关闭D8灯
                    lv_obj_set_style_bg_color(light_button, lv_color_hex(0xFFA000), 0); // 设置按钮背景颜色
                    lv_label_set_text(light_btn_label, "OFF");
                }
                
            }
            if (bot != NULL)
            {
                bot_n=bot->type;
                printf("bot里面是:%d\n",bot->type);
                if (bot_n!=0)
                {
                    flush_button_state = 0; // ON
                    control_led(7, 1); // 打开D7灯
                    lv_obj_set_style_bg_color(flush_button, lv_color_hex(0x007AFF), 0); // 设置按钮背景颜色
                    lv_label_set_text(flush_btn_label, "ON");
                }
                else
                {
                    flush_button_state = 1; // OFF
                    control_led(7, 0); // 关闭D7灯
                    lv_obj_set_style_bg_color(flush_button, lv_color_hex(0xFFA000), 0); // 设置按钮背景颜色
                   lv_label_set_text(flush_btn_label, "OFF");
                }            
            }


            
            
             cJSON_Delete(root); // 删除 cJSON 对象，释放内存
        }



    }

    close(udp_recv_socket);
    return NULL;

}

//--------------发送线程的函数-----------------------
void* thread_SenTo(void* arg)
{
    //1.创建套接字
    int socketfd = socket(AF_INET,SOCK_DGRAM,0);
    if(socketfd == -1)
    {
        perror("socket");
        return -1;
    }

    //绑定发送端的对方的ip和端口
    struct sockaddr_in dest_addr;
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(SERVER_PORT);
    dest_addr.sin_addr.s_addr = inet_addr(SERVER_IP);
    while (1)
    {
          // 创建CJSON对象
        cJSON *root = cJSON_CreateObject();
        if (root == NULL) {
            perror("cJSON_CreateObject");
            close(socketfd);
            return NULL;
        }

        // 添加数据到CJSON对象
        cJSON_AddNumberToObject(root, "temperature", temp_values[MAX_TEMP_VALUES - 1]);
        cJSON_AddNumberToObject(root, "humidity", humid_values[MAX_TEMP_VALUES - 1]);
        cJSON_AddNumberToObject(root, "lux", lux_values[MAX_TEMP_VALUES - 1]);
        cJSON_AddNumberToObject(root, "robot", flush_button_state);
        cJSON_AddNumberToObject(root, "light", light_button_state);

        // 将CJSON对象转化为字符串
        char *cjson_str = cJSON_Print(root);
        if (cjson_str == NULL) {
            perror("cJSON_Print");
            cJSON_Delete(root);
            close(socketfd);
            return NULL;
        }
        // 发送数据
        int ret = sendto(socketfd, cjson_str, strlen(cjson_str), 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
        if (ret == -1) {
            perror("sendto error");
            free(cjson_str);
            cJSON_Delete(root);
            close(socketfd);
            return NULL;
        }

        printf("sendto ret: %d\n", ret);

        // 释放内存
        free(cjson_str);
        cJSON_Delete(root);

        // 每隔5秒发送一次
        sleep(5);

        //if
        if(temp_values[MAX_TEMP_VALUES - 1]>80)
        {
        control_led(9, 1); //开灯
        }
        else
        {
        control_led(9, 0); //关灯
        }

        if( humid_values[MAX_TEMP_VALUES - 1]>80)
        {
         control_led(10, 1); //开灯
        }
        else{
              control_led(10, 0); //开灯
        }
        
    }

    close(socketfd);
}

// 提取 JSON 数据
const char* extract_json(const char* response) {
    const char *json_start = strstr(response, "\r\n\r\n");
    if (json_start) {
        json_start += 4; // 跳过 "\r\n\r\n"
    } else {
        json_start = response; // 如果没有找到 HTTP 头部结束标记，则认为整个响应是 JSON
    }
    return json_start;
}

void get_weather_data(WeatherData *data)
{

    int tcpsock;
	int ret;
	char ip[20]={0};
	char rbuf[20000]={0};
	
	//定义ipv4地址结构体变量
	struct sockaddr_in bindaddr;
	bzero(&bindaddr,sizeof(bindaddr));
	bindaddr.sin_family=AF_INET; //地址协议
	bindaddr.sin_addr.s_addr=htonl(INADDR_ANY);
	bindaddr.sin_port=htons(5548);  //客户端端口号
	
	//获取你要访问的http服务器ip地址
	//http没有加密的   https在http的基础上添加了加密层
	struct hostent *p=gethostbyname("ali-weather.showapi.com"); //天气预报的网址
	struct in_addr *q=(struct in_addr *)(*(p->h_addr_list));
	strcpy(ip,inet_ntoa(*q));
	
	//定义ipv4地址结构体变量存放服务器的ip和端口号
	struct sockaddr_in serveraddr;
	bzero(&serveraddr,sizeof(serveraddr));
	serveraddr.sin_family=AF_INET;
	serveraddr.sin_addr.s_addr=inet_addr(ip); //http服务器的ip
	serveraddr.sin_port=htons(80);  //http协议,端口默认就是80
	
	//创建tcp套接字 --》买手机
	tcpsock=socket(AF_INET,SOCK_STREAM,0);
	if(tcpsock==-1)
	{
		perror("创建tcp套接字失败了!\n");
		return -1;
	}
	
	//取消端口绑定限制
	int on=1; //非零
	setsockopt(tcpsock,SOL_SOCKET,SO_REUSEADDR,&on,sizeof(on));
	
	//绑定ip和端口号 --》绑定手机号
	ret=bind(tcpsock,(struct sockaddr *)&bindaddr,sizeof(bindaddr));
	if(ret==-1)
	{
		perror("绑定ip和端口号失败了!\n");
		return -1;
	}
	
	//连接服务器 --》拨号
	ret=connect(tcpsock,(struct sockaddr *)&serveraddr,sizeof(serveraddr));
	if(ret==-1)
	{
		perror("连接服务器失败了!\n");
		return -1;
	}
	
	//拼接字符串得到完整的http请求
	char *httprequest="GET /area-to-weather-date?area=%E5%B9%BF%E5%B7%9E HTTP/1.1\r\n"
					  "Host: ali-weather.showapi.com\r\n"
					  "Authorization: APPCODE 5d9ebeff853145a7b80cbe3dbfd9580e\r\n\r\n";
	//发送刚才的请求
	send(tcpsock,httprequest,strlen(httprequest),0);
	
	//接收http服务器回复的应答信息
	recv(tcpsock,rbuf,20000,0);
    printf("天气预报网址给我回复: %s\n",rbuf);
   
    cJSON *root = cJSON_Parse(rbuf);
    if (root == NULL) {
        // printf("Error parsing JSON data %s.\n", cJSON_GetErrorPtr());
        return;
    }

    cJSON *body = cJSON_GetObjectItem(root, "showapi_res_body");
    if (body == NULL) {
        printf("Error finding 'showapi_res_body' in JSON data.\n");
        cJSON_Delete(root);
        return;
    }

    cJSON *city_info = cJSON_GetObjectItem(body, "cityInfo");
    if (city_info) {
        cJSON *city = cJSON_GetObjectItem(city_info, "c4");
        if (city) {
            strncpy(data->city, city->valuestring, sizeof(data->city) - 1);
        }
    }

    cJSON *f1 = cJSON_GetObjectItem(body, "f1");
    if (f1) {
        cJSON *wind_power = cJSON_GetObjectItem(f1, "day_wind_power");
        if (wind_power) {
            strncpy(data->wind_power, wind_power->valuestring, sizeof(data->wind_power) - 1);
        }

        cJSON *rain = cJSON_GetObjectItem(f1, "jiangshui");
        if (rain) {
            strncpy(data->rain, rain->valuestring, sizeof(data->rain) - 1);
        }
    }

    cJSON_Delete(root);
	//关闭套接字
	close(tcpsock);



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
    //update_system_time("2024-06-25 15:30:00");
    //创建接收服务器数据的线程
    pthread_t rec_id; // 创建线程标识符
	pthread_create(&rec_id, NULL, thread_Recv, NULL);

    //创建发送数据到服务器的线程
    pthread_t senFuntion_id; // 创建线程标识符
	pthread_create(&senFuntion_id, NULL, thread_SenTo, NULL);
   
    //打开设备文件
    led_fd = open(LED_DEVICE, O_RDWR);
    if(led_fd == -1) {
        perror("open");
        return -1;
    }
    /*Create a Demo*/
    //lv_demo_widgets();//屏蔽掉demo
    control_led(9, 0); // 关闭D7灯
    control_led(10, 0); // 关闭D7灯
    /*Handle LitlevGL tasks (tickless mode)*/
    while(1) 
    {
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
