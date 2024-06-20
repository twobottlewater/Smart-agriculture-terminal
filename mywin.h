#ifndef MYWIN

#define MYWIN

 

#include "lvgl/lvgl.h"

#include "lvgl/demos/lv_demos.h"

#include "lv_drivers/display/fbdev.h"

#include "lv_drivers/indev/evdev.h"

#include <unistd.h>

#include <pthread.h>

#include <time.h>

#include <sys/time.h>

#include <stdio.h>

 

// 界面切换

void show_main_win();

 

#endif