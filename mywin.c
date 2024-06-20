#include "mywin.h"

 

#define HIDDEN_WIN (0)

// 主界面

static lv_obj_t * main_win = NULL;

// 视频界面

static lv_obj_t * video_win = NULL;

// 音乐界面

static lv_obj_t * music_win = NULL;

 

// 视频按钮的回调函数  (事件函数)

static void video_btn_event_callback(lv_event_t * event);

 

// 音乐按钮的回调函数  (事件函数)

static void music_btn_event_callback(lv_event_t * event);

 

// 图标按钮

lv_obj_t * lv_imgbtn(lv_obj_t * parent, int x, int y, int w, int h, char * pic_path)

{

    /*Darken the button when pressed and make it wider*/

    static lv_style_t style_pr;

    lv_style_init(&style_pr);

    lv_style_set_img_recolor_opa(&style_pr, LV_OPA_30);

    lv_style_set_img_recolor(&style_pr, lv_color_black());

    lv_style_set_transform_height(&style_pr, 20);

 

    /*Create an image button*/

    lv_obj_t * imgbtn = lv_imgbtn_create(parent);

    if(imgbtn == NULL) {

        LV_LOG_ERROR("lv_imgbtn_create fail:"); // 输出错误提示

        return NULL;

    }

 

    lv_obj_set_size(imgbtn, w, h); //"S:/1.png"

    lv_imgbtn_set_src(imgbtn, LV_IMGBTN_STATE_RELEASED, NULL, pic_path, NULL);

    // lv_obj_add_style(imgbtn1, &style_def, 0);

    lv_obj_add_style(imgbtn, &style_pr, LV_STATE_PRESSED);

 

    lv_obj_align(imgbtn, LV_ALIGN_TOP_LEFT, x, h);

 

    return imgbtn;

}

 

// 显示主界面

void show_main_win()

{

    // 1.创建主界面

    main_win = lv_obj_create(lv_scr_act()); // 创建一个独立的对象，大小与屏幕一致

 

    // 2.设置主界面的大小与屏幕一致

    lv_obj_set_size(main_win, 800, 480);

 

    // 3.在主界面中创建音乐与视频图标

    lv_obj_t * video = lv_imgbtn(main_win, 100, 100, 200, 200, "S:/video.png");

 

    lv_obj_add_event_cb(video, video_btn_event_callback, LV_EVENT_CLICKED, NULL);

 

    lv_obj_t * music = lv_imgbtn(main_win, 400, 100, 200, 200, "S:/music.png");

 

    lv_obj_add_event_cb(music, music_btn_event_callback, LV_EVENT_CLICKED, NULL);

}

 

// 显示播放器界面

static void show_video_win()

{

 

    // 创建一个对象

    video_win = lv_obj_create(lv_scr_act()); // 创建一个独立的对象，大小与屏幕一致

 

    lv_obj_set_size(video_win, 800, 480);

 

    // 在视频界面中创建一个人按钮

    lv_obj_t * label = lv_label_create(video_win);

 

    // 设置标签的内容

    lv_label_set_text(label, "VIDEO");

 

    // 放入中间

    lv_obj_center(label);

}

 

// 音乐界面返回事件函数

static void music_return(lv_event_t * e)

{

    LV_LOG_USER("music_return");

 

    // 删除音乐界面

    lv_obj_del(music_win);

 

    // 显示主界面

    show_main_win();

}

 

// 显示音乐界面

static void show_music_win()

{

 

    // 创建一个对象

    music_win = lv_obj_create(lv_scr_act()); // 创建一个独立的对象，大小与屏幕一致

 

    lv_obj_set_size(music_win, 800, 480);

 

    // 在视频界面中创建一个人按钮

    lv_obj_t * label = lv_label_create(music_win);

 

    // 设置标签的内容

    lv_label_set_text(label, "MUSIC");

 

    // 放入中间

    lv_obj_center(label);

 

    // 创建一个返回按钮

 

    // 创建一个按钮对象

    lv_obj_t * btn = lv_btn_create(music_win);

    // 设置按钮的大小

    lv_obj_set_size(btn, 100, 50);

    // 把按钮放入右下角

    lv_obj_align(btn, LV_ALIGN_BOTTOM_RIGHT, 0, 0);

 

    // 添加按钮的事件 ，当按钮 被点击后就会触发  event_cb 函数

    lv_obj_add_event_cb(btn, music_return, LV_EVENT_CLICKED, NULL);

 

    // 创建一个标签对象  ，在按钮上

    lv_obj_t * label1 = lv_label_create(btn);

    // 设置标签的文字

    lv_label_set_text(label1, "return");

    // 把标签放入按钮的中央

    lv_obj_center(label1);

}

 

// 点击函数

static void video_btn_event_callback(lv_event_t * event)

{

    // 获取事件号

    lv_event_code_t code = lv_event_get_code(event);

 

    // 单击事件

    if(code == LV_EVENT_CLICKED) {

#if HIDDEN_WIN

        // 给窗口1的容器添加隐藏属性，清除窗口2的隐藏属性

        lv_obj_add_flag(win1_contanier, LV_OBJ_FLAG_HIDDEN);

        lv_obj_clear_flag(win2_contanier, LV_OBJ_FLAG_HIDDEN);

#else

        // 删除主界面

        lv_obj_del(main_win);

 

        // 显示视频界面

        show_video_win();

#endif

    }

}

 

static void music_btn_event_callback(lv_event_t * event)

{

    lv_event_code_t code = lv_event_get_code(event);

    if(code == LV_EVENT_CLICKED) {

#if HIDDEN_WIN

        // 给窗口2的容器添加隐藏属性，清除窗口1的隐藏属性

        lv_obj_add_flag(win2_contanier, LV_OBJ_FLAG_HIDDEN);

        lv_obj_clear_flag(win1_contanier, LV_OBJ_FLAG_HIDDEN);

#else

        // 删除主界面

        lv_obj_del(main_win);

 

        // 显示视频界面

        show_music_win();

#endif

    }

}