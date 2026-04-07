#pragma once
#include "config.h"

// LVGL screens
extern lv_obj_t *scr_idle;
extern lv_obj_t *scr_hike;
extern lv_obj_t *scr_saving;
extern lv_obj_t *scr_sync;

// idle screen
extern lv_obj_t *lbl_idle_bigtime; 
extern lv_obj_t *lbl_idle_bt_icon; 
extern lv_obj_t *lbl_idle_gps_icon; 
extern lv_obj_t *lbl_idle_batt_icon;
extern lv_obj_t *lbl_idle_batt_pct; 
extern lv_obj_t *lbl_idle_btstate; 
extern lv_obj_t *lbl_idle_charge_icon;
extern lv_obj_t *lbl_idle_date;

// hike screen
extern lv_obj_t *lbl_hike_toptime;
extern lv_obj_t *lbl_hike_bt_icon; 
extern lv_obj_t *lbl_hike_batt_icon; 
extern lv_obj_t *lbl_hike_batt_pct;
extern lv_obj_t *lbl_hike_charge_icon; 
extern lv_obj_t *lbl_hike_low_batt; 
extern lv_obj_t *lbl_dur_val;
extern lv_obj_t *lbl_steps_val; 
extern lv_obj_t *lbl_dist_val; 
extern lv_obj_t *lbl_kcal_val;
extern lv_obj_t *lbl_hike_gps;

// functions
extern const char* battIcon();
extern const char* chargeIcon();
extern String battPctStr();
extern String twoDigits(int n);
extern void setBackground(lv_obj_t *scr);
extern lv_obj_t *makeLabel(lv_obj_t *parent, const char *text, int x, int y, lv_color_t color = LV_COLOR_BLACK, const lv_font_t *font = FONT_SMALL);
extern lv_obj_t *makeCard(lv_obj_t *parent, int x, int y, int w, int h, int radius = 14);
extern void addHuippuLogo(lv_obj_t *scr);
extern void setBtIconColor(lv_obj_t *icon);
extern void setGpsIconColor(lv_obj_t *icon);
extern void buildIdleScreen();
extern void buildHikeScreen();
extern void buildSavingScreen();
extern void buildSyncScreen();

// event handlers
extern void startHikeBtnEvent(lv_obj_t *obj, lv_event_t event);
extern void endHikeBtnEvent(lv_obj_t *obj, lv_event_t event);
extern void wakeTouchEvent(lv_obj_t *obj, lv_event_t event);