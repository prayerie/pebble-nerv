#include <ctype.h>
#include <pebble.h>
//#include "main.h"

#define KEY_TEMPERATURE 0
#define KEY_CONDITIONS 1
#define SETTINGS_KEY 1

static Window *s_main_window;

static TextLayer *s_time_layer_l;
static TextLayer *s_time_layer_r;
static TextLayer *s_dat_layer;
static TextLayer *s_day_text_layer_jp;
static TextLayer *s_day_text_layer;
static TextLayer *s_tem_layer;

static GFont transport_med_small;
static GFont transport_med;
static GFont helv;
static GFont seven_seg, seven_seg_sml;

static GFont noto;

static BitmapLayer *s_background_layer;

static GBitmap *s_background_bitmap, *s_on_bitmap, *s_bt_bitmap, *s_day_bitmap, *s_temp_bitmap;
static GBitmap *s_background_j_bitmap;
static GBitmap *s_bat_bg_bitmap, *s_bat_bar_bitmap;

static BitmapLayer *s_background_layer, *s_bt_layer, *s_on_layer, *s_day_layer, *s_bat_bg_layer, *s_temp_layer;
static BitmapLayer *s_bat_bar1_layer, *s_bat_bar2_layer, *s_bat_bar3_layer, *s_bat_bar4_layer;

static int s_battery_level;
static int s_last_battery_draw_pct = 100;

typedef struct ClaySettings {
    bool fahrenheit;
    bool hideDotw;
    bool japanese;
    bool hideTemp;
    char customKey[32];
    char customLoc[32];
    char colourScheme[32];
    char secret[32];
    char bwBgColor[32];
} __attribute__((__packed__)) ClaySettings;

static ClaySettings settings;

static void default_settings() {
    settings.fahrenheit = false;
    settings.hideDotw = false;
    settings.japanese = false;
    settings.hideTemp = false;
    strcpy(settings.customKey, "");
    strcpy(settings.customLoc, "");
    strcpy(settings.colourScheme, "d");
    strcpy(settings.secret, "");
    strcpy(settings.bwBgColor, "b");
}

static void save_settings() {
    int wrote = persist_write_data(SETTINGS_KEY, &settings, sizeof(settings));
}

static void load_settings() {
    default_settings();
    int read = persist_read_data(SETTINGS_KEY, &settings, sizeof(settings));
}

static void bluetooth_callback(bool connected) {
    layer_set_hidden(bitmap_layer_get_layer(s_bt_layer), connected);

    if (!connected) {
        vibes_double_pulse();
    }
}

static void outbox_iter() {
    DictionaryIterator *iter;
    app_message_outbox_begin(&iter);

    dict_write_uint8(iter, 0, 0);

    app_message_outbox_send();
}

// static void refresh_weather_layout() {
//     layer_set_hidden(text_layer_get_layer(s_wea_layer), !settings.twoLineWeather);
//     layer_set_hidden(text_layer_get_layer(s_tem_layer), !settings.twoLineWeather);
//     layer_set_hidden(text_layer_get_layer(s_wea_tem_layer), settings.twoLineWeather);
// }

// static void update_background() {
//     if (!s_background_layer)
//         return;

//     if (settings.fillCorners)
//         bitmap_layer_set_bitmap(s_background_layer, s_background_filled_bitmap);
//     else
//         bitmap_layer_set_bitmap(s_background_layer, s_background_bitmap);
// }

static GColor clock_colour() {
    // GColor colour_text;
    // if (1 || strcmp(settings.colourScheme, "d") == 0) {
    //     colour_text = GColorChromeYellow;
    // } else if (strcmp(settings.colourScheme, "r") == 0) {
    //     colour_text = GColorSunsetOrange;
    // } else if (strcmp(settings.colourScheme, "a") == 0) {
    //     colour_text = GColorChromeYellow;
    // } else if (strcmp(settings.colourScheme, "m") == 0) {
    //     colour_text = GColorRed;
    // } else {
    //     colour_text = GColorChromeYellow;
    // }
    return PBL_IF_COLOR_ELSE(GColorChromeYellow, GColorWhite);
}

static void update_colours() {

    gbitmap_destroy(s_background_bitmap);
    gbitmap_destroy(s_background_j_bitmap);
    if (strcmp(settings.bwBgColor, "w") == 0 && PBL_IF_BW_ELSE(1, 0))
        s_background_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BACKGROUND1_2);
    else
        s_background_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BACKGROUND1);
    bitmap_layer_set_bitmap(s_background_layer, s_background_bitmap);
    printf("This code ran\n");

    // if (strcmp(settings.colourScheme, "d") == 0) {
    //     s_background_bitmap =
    //         gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BACKGROUND1);
    //     s_background_j_bitmap =
    //         gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BACKGROUND1_2);
    // } else if (strcmp(settings.colourScheme, "r") == 0) {
    //     s_background_bitmap =
    //         gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BACKGROUND2);
    //     s_background_j_bitmap =
    //         gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BACKGROUND2_2);
    // } else if (strcmp(settings.colourScheme, "a") == 0) {
    //     s_background_bitmap =
    //         gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BACKGROUND3);
    //     s_background_j_bitmap =
    //         gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BACKGROUND3_2);
    // } else if (strcmp(settings.colourScheme, "m") == 0) {
    //     s_background_bitmap =
    //         gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BACKGROUND4);
    //     s_background_j_bitmap =
    //         gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BACKGROUND4_2);
    // }
    text_layer_set_text_color(s_time_layer_r, clock_colour());
    text_layer_set_text_color(s_time_layer_l, clock_colour());
    text_layer_set_text_color(s_day_text_layer, clock_colour());
    text_layer_set_text_color(s_dat_layer, clock_colour());
}

static void update_temp_format() {
    if (!s_temp_layer)
        return;
    gbitmap_destroy(s_temp_bitmap);
    if (settings.fahrenheit)
        s_temp_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_TEMPF);
    else
        s_temp_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_TEMPC);
    bitmap_layer_set_bitmap(s_temp_layer, s_temp_bitmap);
}

static void update_date() {
    time_t temp = time(NULL);
    struct tm *tick_time = localtime(&temp);
    static char dat_buffer[4];
    static char date_buffer[16];
#if defined(PBL_RECT)
    strftime(date_buffer, sizeof(date_buffer), "%y    %m   %d", tick_time);
#else
    if (settings.fahrenheit)
        strftime(date_buffer, sizeof(date_buffer), "%m/%d", tick_time);
    else
        strftime(date_buffer, sizeof(date_buffer), "%d/%m", tick_time);
#endif
    strftime(dat_buffer, sizeof(date_buffer), "%a", tick_time);
    for (int i = 0; i < 4; i++) {
        dat_buffer[i] = toupper((unsigned char)dat_buffer[i]);
    }
    text_layer_set_text(s_day_text_layer, dat_buffer);
    text_layer_set_text(s_dat_layer, date_buffer);
}

// 0 = off,
// 1 = CLEAR
// 2 = CLOUD
// 3 = PRECIP
// 4 = ATMO
static void set_indicator(int status) {
    int x = PBL_IF_RECT_ELSE(39, 29), y = PBL_IF_RECT_ELSE(114, 119);
    layer_set_hidden(bitmap_layer_get_layer(s_on_layer), 1);
    switch (status) {
    case 0:
        return;
    case 1:
        break;
    case 2:
        x = x + 30;
        break;
    case 3:
        x = x + 60;
        break;
    case 4:
        x = x + 90;
        break;
    default:
        APP_LOG(APP_LOG_LEVEL_ERROR, "This shouldn't happen (set_indicator param %d is not 0, 1, 2, 3, or 4)\n", status);
    }
    layer_set_hidden(bitmap_layer_get_layer(s_on_layer), 0);
    layer_set_frame(bitmap_layer_get_layer(s_on_layer), GRect(x, y, 21, 7));
}

static void update_ht() {
    Layer *l, *r;
    l = text_layer_get_layer(s_time_layer_l);
    r = text_layer_get_layer(s_time_layer_r);
    if (!l || !r)
        return;
    if (strcmp(settings.secret, "ht") == 0) {
        layer_set_hidden(l, 1);
        layer_set_hidden(r, 1);
    } else {
        layer_set_hidden(l, 0);
        layer_set_hidden(r, 0);
    }
}

static void inbox_received_callback(DictionaryIterator *iterator,
                                    void *context) {
    static char temperature_buffer[8];
    static char conditions_buffer[32];
    static char weather_layer_buffer[32];

    Tuple *temp_tuple = dict_find(iterator, MESSAGE_KEY_TEMPERATURE);
    Tuple *conditions_tuple = dict_find(iterator, MESSAGE_KEY_CONDITIONS);

    Tuple *fahrenheit_t = dict_find(iterator, MESSAGE_KEY_fahrenheit);
    if (fahrenheit_t) {
        settings.fahrenheit = fahrenheit_t->value->int32 == 1;
        persist_write_data(SETTINGS_KEY, &settings, sizeof(settings));
        update_temp_format();
        update_date();
        outbox_iter();
    }

    Tuple *hideDotw_t = dict_find(iterator, MESSAGE_KEY_hideDotw);
    if (hideDotw_t) {
        settings.hideDotw = hideDotw_t->value->int32 == 1;
        save_settings();
        layer_set_hidden(bitmap_layer_get_layer(s_day_layer), settings.hideDotw);
        layer_set_hidden(text_layer_get_layer(s_day_text_layer), settings.hideDotw);
    }

    Tuple *hideTemp_t = dict_find(iterator, MESSAGE_KEY_hideTemp);
    if (hideTemp_t) {
        settings.hideTemp = hideTemp_t->value->int32 == 1;
        save_settings();
        layer_set_hidden(text_layer_get_layer(s_tem_layer), settings.hideTemp);
        layer_set_hidden(bitmap_layer_get_layer(s_temp_layer), settings.hideTemp);
    }

    Tuple *customKey_t = dict_find(iterator, MESSAGE_KEY_customKey);
    if (customKey_t) {
        strcpy(settings.customKey, customKey_t->value->cstring);
        outbox_iter();
        save_settings();
    }

    Tuple *customLoc_t = dict_find(iterator, MESSAGE_KEY_customLoc);
    if (customLoc_t) {
        strcpy(settings.customLoc, customLoc_t->value->cstring);
        outbox_iter();
        save_settings();
    }

    Tuple *japanese_t = dict_find(iterator, MESSAGE_KEY_japanese);
    if (japanese_t) {
        settings.japanese = japanese_t->value->int32 == 1;
        // update_jp();
        save_settings();
    }

    Tuple *colourScheme_t = dict_find(iterator, MESSAGE_KEY_colourScheme);
    if (colourScheme_t) {
        strcpy(settings.colourScheme, colourScheme_t->value->cstring);
        // update_colours();
        // update_jp();
        save_settings();
    }

    Tuple *secret_t = dict_find(iterator, MESSAGE_KEY_secret);
    if (secret_t) {
        strcpy(settings.secret, secret_t->value->cstring);
        // update_colours();
        // update_jp();
        save_settings();
        update_ht();
    }

    Tuple *bwBgColor_t = dict_find(iterator, MESSAGE_KEY_bwBgColor);
    if (bwBgColor_t) {
        strcpy(settings.bwBgColor, bwBgColor_t->value->cstring);
        //
        // update_jp();
        save_settings();
        update_colours();
    }
    if (temp_tuple && conditions_tuple) {
        double temp = (int)temp_tuple->value->int32 / 10;
        int weather_category = 0;
        if (settings.fahrenheit)
            temp = ((9 * temp) / 5) + 32;
        if (settings.fahrenheit)
            snprintf(temperature_buffer, sizeof(temperature_buffer), "%d", (int)temp);
        else
            snprintf(temperature_buffer, sizeof(temperature_buffer), "%d", (int)temp);
        snprintf(conditions_buffer, sizeof(conditions_buffer), "%.3s", conditions_tuple->value->cstring);
        text_layer_set_text(s_tem_layer, temperature_buffer);
        if (strcmp(conditions_buffer, "Cle") == 0) {
            weather_category = 1;
        } else if (strcmp(conditions_buffer, "Clo") == 0) {
            weather_category = 2;
        } else if (strcmp(conditions_buffer, "Thu") == 0) {
            weather_category = 4;
        } else if (strcmp(conditions_buffer, "Dri") == 0) {
            weather_category = 3;
        } else if (strcmp(conditions_buffer, "Rai") == 0) {
            weather_category = 3;
        } else if (strcmp(conditions_buffer, "Fog") == 0) {
            weather_category = 4;
        } else if (strcmp(conditions_buffer, "Sno") == 0) {
            weather_category = 3;
        } else if (strcmp(conditions_buffer, "Mis") == 0) {
            weather_category = 4;
        } else if (strcmp(conditions_buffer, "Smo") == 0) {
            weather_category = 4;
        } else if (strcmp(conditions_buffer, "Haz") == 0) {
            weather_category = 4;
        } else if (strcmp(conditions_buffer, "Dus") == 0) {
            weather_category = 4;
        } else if (strcmp(conditions_buffer, "San") == 0) {
            weather_category = 4;
        } else if (strcmp(conditions_buffer, "Ash") == 0) {
            weather_category = 4;
        } else if (strcmp(conditions_buffer, "Squ") == 0) {
            weather_category = 4;
        } else if (strcmp(conditions_buffer, "Tor") == 0) {
            weather_category = 4;
        }

        set_indicator(weather_category);
    }
}

static void inbox_dropped_callback(AppMessageResult reason, void *context) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Message dropped!");
}

static void outbox_failed_callback(DictionaryIterator *iterator,
                                   AppMessageResult reason, void *context) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Outbox send failed!");
}

static void outbox_sent_callback(DictionaryIterator *iterator, void *context) {
    APP_LOG(APP_LOG_LEVEL_INFO, "Outbox send success!");
}

static void update_battery() {
    int bar1, bar2, bar3, bar4;
    bar1 = s_battery_level >= 20;
    bar2 = s_battery_level >= 40;
    bar3 = s_battery_level >= 60;
    bar4 = s_battery_level >= 90;
    Layer *b1l = bitmap_layer_get_layer(s_bat_bar1_layer);
    Layer *b2l = bitmap_layer_get_layer(s_bat_bar2_layer);
    Layer *b3l = bitmap_layer_get_layer(s_bat_bar3_layer);
    Layer *b4l = bitmap_layer_get_layer(s_bat_bar4_layer);
    if (b1l) {
        layer_set_hidden(b1l, !bar1);
        layer_set_hidden(b2l, !bar2);
        layer_set_hidden(b3l, !bar3);
        layer_set_hidden(b4l, !bar4);
        s_last_battery_draw_pct = s_battery_level;
    }
}

static void main_window_load(Window *window) {
    Layer *window_layer = window_get_root_layer(window);
    GRect bounds = layer_get_bounds(window_layer);
    s_background_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BACKGROUND1);

    s_background_layer = bitmap_layer_create(bounds);
    bitmap_layer_set_bitmap(s_background_layer, s_background_bitmap);
    layer_add_child(window_layer, bitmap_layer_get_layer(s_background_layer));
    // s_bt_icon_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BT_ICON);
    seven_seg = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_DSEG_33));
    seven_seg_sml = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_DSEG_16));
    noto = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_NOTO_14));
    helv = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_HELV_15));
    // s_bt_icon_layer = bitmap_layer_create(GRect(15, 63, 30, 30));

    s_on_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_ON);
    s_on_layer = bitmap_layer_create(GRect(39, 114, 21, 7));
    bitmap_layer_set_bitmap(s_on_layer, s_on_bitmap);
    layer_add_child(window_layer, bitmap_layer_get_layer(s_on_layer));

    set_indicator(2);

    if (settings.fahrenheit)
        s_temp_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_TEMPF);
    else
        s_temp_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_TEMPC);
    // s_temp_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_TEMPF);
    s_temp_layer = bitmap_layer_create(GRect(PBL_IF_RECT_ELSE(97, 105), PBL_IF_RECT_ELSE(125, 130), 48, 21));
    bitmap_layer_set_bitmap(s_temp_layer, s_temp_bitmap);
    layer_add_child(window_layer, bitmap_layer_get_layer(s_temp_layer));

    s_bat_bg_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BATBG);
    s_bat_bg_layer = bitmap_layer_create(GRect(PBL_IF_RECT_ELSE(0, 82), 0, 17, 8));
    bitmap_layer_set_bitmap(s_bat_bg_layer, s_bat_bg_bitmap);
    layer_add_child(window_layer, bitmap_layer_get_layer(s_bat_bg_layer));

    s_bat_bar_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BATBAR);

    s_bat_bar1_layer = bitmap_layer_create(GRect(PBL_IF_RECT_ELSE(4, 86), 1, 2, 5));
    bitmap_layer_set_bitmap(s_bat_bar1_layer, s_bat_bar_bitmap);
    layer_add_child(window_layer, bitmap_layer_get_layer(s_bat_bar1_layer));

    s_bat_bar2_layer = bitmap_layer_create(GRect(PBL_IF_RECT_ELSE(7, 89), 1, 2, 5));
    bitmap_layer_set_bitmap(s_bat_bar2_layer, s_bat_bar_bitmap);
    layer_add_child(window_layer, bitmap_layer_get_layer(s_bat_bar2_layer));

    s_bat_bar3_layer = bitmap_layer_create(GRect(PBL_IF_RECT_ELSE(10, 92), 1, 2, 5));
    bitmap_layer_set_bitmap(s_bat_bar3_layer, s_bat_bar_bitmap);
    layer_add_child(window_layer, bitmap_layer_get_layer(s_bat_bar3_layer));

    s_bat_bar4_layer = bitmap_layer_create(GRect(PBL_IF_RECT_ELSE(13, 95), 1, 2, 5));
    bitmap_layer_set_bitmap(s_bat_bar4_layer, s_bat_bar_bitmap);
    layer_add_child(window_layer, bitmap_layer_get_layer(s_bat_bar4_layer));

    s_day_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_DAY);
    s_day_layer = bitmap_layer_create(GRect(PBL_IF_RECT_ELSE(5, 35), 13, 57, 27));
    bitmap_layer_set_bitmap(s_day_layer, s_day_bitmap);
    layer_add_child(window_layer, bitmap_layer_get_layer(s_day_layer));

    s_dat_layer = text_layer_create(GRect(PBL_IF_RECT_ELSE(48, 88), PBL_IF_RECT_ELSE(148, 153), bounds.size.w, 50));
    text_layer_set_background_color(s_dat_layer, GColorClear);
    text_layer_set_text_color(s_dat_layer, clock_colour());
    text_layer_set_font(s_dat_layer, noto);
    text_layer_set_text_alignment(s_dat_layer, GTextAlignmentLeft);
    text_layer_set_text(s_dat_layer, "00     00    00");
    layer_add_child(window_layer, text_layer_get_layer(s_dat_layer));

    s_day_text_layer = text_layer_create(GRect(PBL_IF_RECT_ELSE(-42, -30), 13, bounds.size.w, 50));
    text_layer_set_background_color(s_day_text_layer, GColorClear);
    text_layer_set_text_color(s_day_text_layer, clock_colour());
    text_layer_set_font(s_day_text_layer, helv);
    text_layer_set_text_alignment(s_day_text_layer, GTextAlignmentCenter);
    layer_add_child(window_layer, text_layer_get_layer(s_day_text_layer));
    text_layer_set_text(s_day_text_layer, "DAY");

    s_bt_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_NO_BT);
    s_bt_layer = bitmap_layer_create(GRect(PBL_IF_RECT_ELSE(13, 37), 10, 57, 27));
    bitmap_layer_set_bitmap(s_bt_layer, s_bt_bitmap);
    layer_add_child(window_layer, bitmap_layer_get_layer(s_bt_layer));

    // int time_x = 16;
    // if (strcmp(settings.colourScheme, "brown") == 0)
    //     time_x = 20;
    s_time_layer_l = text_layer_create(GRect(PBL_IF_RECT_ELSE(35, 55), PBL_IF_RECT_ELSE(67, 72), bounds.size.w, 50));
    text_layer_set_background_color(s_time_layer_l, GColorClear);
    text_layer_set_text_color(s_time_layer_l, clock_colour());
    text_layer_set_font(s_time_layer_l, seven_seg);
    text_layer_set_text_alignment(s_time_layer_l, GTextAlignmentLeft);
    layer_add_child(window_layer, text_layer_get_layer(s_time_layer_l));

    s_time_layer_r = text_layer_create(GRect(PBL_IF_RECT_ELSE(90, 110), PBL_IF_RECT_ELSE(67, 72), bounds.size.w, 50));
    text_layer_set_background_color(s_time_layer_r, GColorClear);
    text_layer_set_text_color(s_time_layer_r, clock_colour());

    text_layer_set_font(s_time_layer_r, seven_seg);
    text_layer_set_text_alignment(s_time_layer_r, GTextAlignmentLeft);

    layer_add_child(window_layer, text_layer_get_layer(s_time_layer_r));

    // s_bat_layer = text_layer_create(GRect(16, 113, bounds.size.w, 50));
    // text_layer_set_background_color(s_bat_layer, GColorClear);
    // text_layer_set_text_color(s_bat_layer, GColorWhite);
    // text_layer_set_font(s_bat_layer, motorway);
    // text_layer_set_text_alignment(s_bat_layer, GTextAlignmentCenter);

    // s_dat_layer = text_layer_create(GRect(-32, 117, bounds.size.w, 50));
    // text_layer_set_background_color(s_dat_layer, GColorClear);
    // text_layer_set_text_color(s_dat_layer, text_colour());
    // text_layer_set_font(s_dat_layer, transport_he);
    // text_layer_set_text_alignment(s_dat_layer, GTextAlignmentCenter);

    // s_wea_tem_layer = text_layer_create(GRect(47, 67, bounds.size.w, 50));
    // text_layer_set_background_color(s_wea_tem_layer, GColorClear);
    // text_layer_set_text_color(s_wea_tem_layer, text_colour());
    // text_layer_set_font(s_wea_tem_layer, transport_med_small);
    // text_layer_set_text_alignment(s_wea_tem_layer, GTextAlignmentLeft);
    // layer_add_child(window_layer, text_layer_get_layer(s_wea_tem_layer));

    // s_wea_layer = text_layer_create(GRect(7, 75, bounds.size.w, 50));
    // text_layer_set_background_color(s_wea_layer, GColorClear);
    // text_layer_set_text_color(s_wea_layer, text_colour());
    // text_layer_set_font(s_wea_layer, transport_med_small);
    // text_layer_set_text_alignment(s_wea_layer, GTextAlignmentCenter);

    s_tem_layer = text_layer_create(GRect(PBL_IF_RECT_ELSE(-7, -35), PBL_IF_RECT_ELSE(128, 133), bounds.size.w, 50));
    text_layer_set_background_color(s_tem_layer, GColorClear);
    text_layer_set_text_color(s_tem_layer, PBL_IF_COLOR_ELSE(GColorFolly, GColorWhite));
    text_layer_set_font(s_tem_layer, seven_seg_sml);
    text_layer_set_text_alignment(s_tem_layer, GTextAlignmentRight);
    layer_add_child(window_layer, text_layer_get_layer(s_tem_layer));

    // layer_add_child(window_layer, text_layer_get_layer(s_bat_layer));
    // layer_add_child(window_layer, text_layer_get_layer(s_dat_layer));

    // layer_set_hidden(text_layer_get_layer(s_wea_layer), !settings.twoLineWeather);
    // layer_set_hidden(text_layer_get_layer(s_tem_layer), !settings.twoLineWeather);
    // layer_set_hidden(text_layer_get_layer(s_wea_tem_layer), settings.twoLineWeather);
    bluetooth_callback(connection_service_peek_pebble_app_connection());
    update_colours();
    update_battery();
    // update_jp();
    update_temp_format();
    update_ht();
    update_date();
}

static void main_window_unload(Window *window) {
    // fonts_unload_custom_font(transport_med_small);
    // fonts_unload_custom_font(transport_med);
    // fonts_unload_custom_font(transport_he);
    // gbitmap_destroy(s_bt_icon_bitmap);
    // bitmap_layer_destroy(s_bt_icon_layer);
    // gbitmap_destroy(s_background_filled_bitmap);
    fonts_unload_custom_font(seven_seg);
    gbitmap_destroy(s_background_bitmap);
    gbitmap_destroy(s_background_j_bitmap);
    gbitmap_destroy(s_bt_bitmap);
    gbitmap_destroy(s_temp_bitmap);
    gbitmap_destroy(s_on_bitmap);
    gbitmap_destroy(s_day_bitmap);
    gbitmap_destroy(s_bat_bg_bitmap);
    gbitmap_destroy(s_bat_bar_bitmap);
    bitmap_layer_destroy(s_background_layer);
    bitmap_layer_destroy(s_bt_layer);
    bitmap_layer_destroy(s_bat_bar1_layer);
    bitmap_layer_destroy(s_bat_bar2_layer);
    bitmap_layer_destroy(s_bat_bar3_layer);
    bitmap_layer_destroy(s_bat_bar4_layer);
    bitmap_layer_destroy(s_temp_layer);
    bitmap_layer_destroy(s_day_layer);
    bitmap_layer_destroy(s_on_layer);
    bitmap_layer_destroy(s_bat_bg_layer);
    fonts_unload_custom_font(helv);
    fonts_unload_custom_font(noto);
    fonts_unload_custom_font(seven_seg_sml);
}

static void update_time() {
    time_t temp = time(NULL);
    struct tm *tick_time = localtime(&temp);

    static char s_buffer_l[4];
    static char s_buffer_r[4];

    strftime(s_buffer_l, sizeof(s_buffer_l), clock_is_24h_style() ? "%H" : "%I", tick_time);
    strftime(s_buffer_r, sizeof(s_buffer_r), "%M", tick_time);
    text_layer_set_text(s_time_layer_r, s_buffer_r);
    text_layer_set_text(s_time_layer_l, s_buffer_l);
}

static void battery_callback(BatteryChargeState state) {
    s_battery_level = state.charge_percent;
    if ((s_battery_level < 20 && s_last_battery_draw_pct >= 20) ||
        (s_battery_level < 40 && s_last_battery_draw_pct >= 40) ||
        (s_battery_level < 60 && s_last_battery_draw_pct >= 60) ||
        (s_battery_level < 90 && s_last_battery_draw_pct >= 90))
        update_battery();
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
    if (tick_time->tm_min == 0) {
        update_date();
        outbox_iter();
    }
    update_time();
}

static void init() {
    load_settings();
    const uint32_t inbox_size = 128;
    const uint32_t outbox_size = 128;
    s_main_window = window_create();
    tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
    battery_state_service_subscribe(battery_callback);
    battery_callback(battery_state_service_peek());

    app_message_register_inbox_received(inbox_received_callback);
    app_message_open(inbox_size, outbox_size);
    app_message_register_inbox_dropped(inbox_dropped_callback);
    app_message_register_outbox_failed(outbox_failed_callback);
    app_message_register_outbox_sent(outbox_sent_callback);

    window_set_window_handlers(
        s_main_window,
        (WindowHandlers){.load = main_window_load, .unload = main_window_unload});

    window_stack_push(s_main_window, true);

    connection_service_subscribe((ConnectionHandlers){
        .pebble_app_connection_handler = bluetooth_callback});
}

static void deinit() { window_destroy(s_main_window); }

int main(void) {
    init();
    app_event_loop();
    deinit();
}