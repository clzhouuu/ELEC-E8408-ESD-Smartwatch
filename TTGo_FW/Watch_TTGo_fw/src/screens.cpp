#include "config.h"
#include "globals.h"
#include "screens.h"
#include "image.c"


// LVGL screens
lv_obj_t *scr_idle = nullptr;
lv_obj_t *scr_hike = nullptr;
lv_obj_t *scr_saving = nullptr;
lv_obj_t *scr_sync = nullptr;

// idle screen
lv_obj_t *lbl_idle_bigtime = nullptr; 
lv_obj_t *lbl_idle_bt_icon = nullptr; 
lv_obj_t *lbl_idle_gps_icon = nullptr; 
lv_obj_t *lbl_idle_batt_icon = nullptr;
lv_obj_t *lbl_idle_batt_pct = nullptr; 
lv_obj_t *lbl_idle_btstate = nullptr; 
lv_obj_t *lbl_idle_charge_icon = nullptr;

// hike screen
lv_obj_t *lbl_hike_toptime = nullptr;
lv_obj_t *lbl_hike_bt_icon = nullptr; 
lv_obj_t *lbl_hike_batt_icon = nullptr; 
lv_obj_t *lbl_hike_batt_pct = nullptr;
lv_obj_t *lbl_hike_charge_icon = nullptr; 
lv_obj_t *lbl_hike_low_batt = nullptr; 
lv_obj_t *lbl_dur_val = nullptr;
lv_obj_t *lbl_steps_val = nullptr; 
lv_obj_t *lbl_dist_val = nullptr; 
lv_obj_t *lbl_kcal_val = nullptr;
lv_obj_t *lbl_hike_gps = nullptr;

// battery icon based on percentage
const char* battIcon() {
    if (batteryPercent > 75) {
        return LV_SYMBOL_BATTERY_FULL;
    }
    if (batteryPercent > 50) {
        return LV_SYMBOL_BATTERY_3;
    }
    if (batteryPercent > 25) {
        return LV_SYMBOL_BATTERY_2;
    }
    if (batteryPercent > 10) {
        return LV_SYMBOL_BATTERY_1;
    }
    return LV_SYMBOL_BATTERY_EMPTY;
}

// battery charge icon based on percentage
const char* chargeIcon() {
    if (s_isCharging) {
        return LV_SYMBOL_CHARGE;
    }
    return "";
} 

// battery percent
String battPctStr() {
    char buf[8];
    snprintf(buf, sizeof(buf), "%d%%", batteryPercent);
    return String(buf);
}

// helper for HHMMSS
String twoDigits(int n) {
    String time;
    if (n < 10) {
        time = "0" + String(n);
    } else {
        time = String(n);
    }
    return time;
}

// background image
void setBackground(lv_obj_t *scr) {
    lv_obj_t *img = lv_img_create(scr, NULL);
    lv_img_set_src(img, &bkgrnd);
    lv_obj_align(img, NULL, LV_ALIGN_CENTER, 0, 0);
}

// label template
lv_obj_t *makeLabel(lv_obj_t *parent, const char *text, int x, int y, lv_color_t color, const lv_font_t *font)
{
    lv_obj_t *lbl = lv_label_create(parent, NULL);
    lv_label_set_text(lbl, text);
    lv_obj_set_pos(lbl, x, y);
 
    lv_style_t *st = new lv_style_t;
    lv_style_init(st);
    lv_style_set_text_color(st, LV_STATE_DEFAULT, color);
    lv_style_set_text_font(st, LV_STATE_DEFAULT, font);
    lv_obj_add_style(lbl, LV_OBJ_PART_MAIN, st);
 
    return lbl;
}

// box template
lv_obj_t *makeCard(lv_obj_t *parent, int x, int y, int w, int h, int radius) {
    lv_style_t *st = new lv_style_t;
    lv_style_init(st);
    lv_style_set_bg_color(st, LV_STATE_DEFAULT, LV_COLOR_WHITE);
    lv_style_set_bg_opa(st, LV_STATE_DEFAULT, LV_OPA_50);
    lv_style_set_border_width(st, LV_STATE_DEFAULT, 0);
    lv_style_set_radius(st, LV_STATE_DEFAULT, radius);
    lv_style_set_pad_all(st, LV_STATE_DEFAULT, 0);
 
    lv_obj_t *card = lv_obj_create(parent, NULL);
    lv_obj_set_size(card, w, h);
    lv_obj_set_pos(card, x, y);
    lv_obj_add_style(card, LV_OBJ_PART_MAIN, st);
    return card;
}

// add the logo on top
void addHuippuLogo(lv_obj_t *scr) {
    makeLabel(scr, "HUIPPU", 8, 8, LV_COLOR_BLACK, FONT_SMALL);
}

// set bt color
void setBtIconColor(lv_obj_t *icon) {
    bool btConn = SerialBT.hasClient();
    lv_color_t btColor;
    if (btConn) {
        btColor = LV_COLOR_MAKE(0x00, 0x99, 0xFF);
    } else {
        btColor = LV_COLOR_MAKE(0xAA, 0xAA, 0xAA);
    }
    lv_obj_set_style_local_text_color(icon, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, btColor);
}

// set gps color
void setGpsIconColor(lv_obj_t *icon) {
    bool gpsFix = gps && gps->location.isValid() && gps->location.age() < 5000;
    lv_color_t gpsColor;
    if (gpsFix) {
        gpsColor = LV_COLOR_MAKE(0x00, 0xAA, 0x00);
    } else {  
        gpsColor = LV_COLOR_MAKE(0xAA, 0xAA, 0xAA);
    }    
    lv_obj_set_style_local_text_color(icon, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, gpsColor);
}

// idle screen
void buildIdleScreen() {
    if (scr_idle) lv_obj_del(scr_idle);
    scr_idle = lv_obj_create(NULL, NULL);
    setBackground(scr_idle);

    makeLabel(scr_idle, "HUIPPU", 8, 8, LV_COLOR_BLACK, FONT_SMALL);

    lbl_idle_bigtime = makeLabel(scr_idle, "00:00", 82, 60, LV_COLOR_BLACK, FONT_HUGE);

    lbl_idle_bt_icon = makeLabel(scr_idle, LV_SYMBOL_BLUETOOTH, 68, 112, LV_COLOR_MAKE(0xAA,0xAA,0xAA), FONT_SMALL);
    lbl_idle_gps_icon = makeLabel(scr_idle, LV_SYMBOL_GPS, 88, 112, LV_COLOR_MAKE(0xAA,0xAA,0xAA), FONT_SMALL);
    lbl_idle_batt_icon = makeLabel(scr_idle, LV_SYMBOL_BATTERY_FULL, 108, 112, LV_COLOR_BLACK, FONT_SMALL);
    lbl_idle_batt_pct = makeLabel(scr_idle, "100%", 128, 112, LV_COLOR_BLACK, FONT_SMALL);
    lbl_idle_charge_icon = makeLabel(scr_idle, chargeIcon(), 166, 112, LV_COLOR_BLACK, FONT_SMALL);

    makeLabel(scr_idle, "Press the button", 60, 160, LV_COLOR_BLACK, FONT_SMALL);
    makeLabel(scr_idle, "to start a hike", 70, 178, LV_COLOR_BLACK, FONT_SMALL);

    lbl_idle_btstate = makeLabel(scr_idle, "", 120, 8, LV_COLOR_BLACK, FONT_SMALL);
}

// hike screen
void buildHikeScreen() {
    if (scr_hike) lv_obj_del(scr_hike);
    scr_hike = lv_obj_create(NULL, NULL);
    setBackground(scr_hike);

    makeLabel(scr_hike, "HUIPPU", 8, 8, LV_COLOR_BLACK, FONT_SMALL);
    lbl_hike_bt_icon = makeLabel(scr_hike, LV_SYMBOL_BLUETOOTH, 70, 8, LV_COLOR_MAKE(0xAA,0xAA,0xAA), FONT_SMALL); 
    lbl_hike_gps = makeLabel(scr_hike, LV_SYMBOL_GPS, 90, 8, LV_COLOR_MAKE(0xAA,0xAA,0xAA), FONT_SMALL);
    lbl_hike_batt_icon = makeLabel(scr_hike, LV_SYMBOL_BATTERY_FULL, 120, 8, LV_COLOR_BLACK, FONT_SMALL);
    lbl_hike_charge_icon = makeLabel(scr_hike, chargeIcon(), 144, 8, LV_COLOR_BLACK, FONT_SMALL);
    lbl_hike_batt_pct = makeLabel(scr_hike, "100%", 158, 8, LV_COLOR_BLACK, FONT_SMALL);

    makeLabel(scr_hike, "\xB7", 197, 8, LV_COLOR_BLACK, FONT_SMALL);
    lbl_hike_toptime = makeLabel(scr_hike, "00:00", 195, 8, LV_COLOR_BLACK, FONT_SMALL);

    lbl_hike_low_batt = makeLabel(scr_hike, "", 68, 36, LV_COLOR_BLACK, FONT_SMALL);

    lbl_dur_val = makeLabel(scr_hike, "00:00:00", 54, 90, LV_COLOR_BLACK, FONT_HUGE);

    lv_obj_t *card = makeCard(scr_hike, 10, 148, 220, 78, 16);

    makeLabel(card, MY_SYMBOL_STEPS, 37, 12, LV_COLOR_BLACK, &steps);
    makeLabel(card, MY_SYMBOL_PERSON, 106, 12, LV_COLOR_BLACK, &person);
    makeLabel(card, MY_SYMBOL_FIRE, 167, 12, LV_COLOR_BLACK, &fire);

    lbl_steps_val = makeLabel(card, "0", 35, 44, LV_COLOR_BLACK, FONT_SMALL);
    lbl_dist_val = makeLabel(card, "0.0km", 85, 44, LV_COLOR_BLACK, FONT_SMALL);
    lbl_kcal_val = makeLabel(card, "0kcal", 157, 44, LV_COLOR_BLACK, FONT_SMALL);
}

// saving screen
void buildSavingScreen() {
    if (scr_saving) lv_obj_del(scr_saving);
    scr_saving = lv_obj_create(NULL, NULL);
    setBackground(scr_saving);

    makeLabel(scr_saving, "HUIPPU", 8, 8, LV_COLOR_BLACK, FONT_SMALL);
    makeLabel(scr_saving, "SAVING...", 60, 105, LV_COLOR_BLACK, FONT_HUGE);
}

// sync screen
void buildSyncScreen() {
    if (scr_sync) lv_obj_del(scr_sync);
    scr_sync = lv_obj_create(NULL, NULL);
    setBackground(scr_sync);

    makeLabel(scr_sync, "HUIPPU", 8, 8, LV_COLOR_BLACK, FONT_SMALL);
    // "Sending..." centered
    makeLabel(scr_sync, "Sending...", 80, 108, LV_COLOR_BLACK, FONT_MEDIUM);
}
