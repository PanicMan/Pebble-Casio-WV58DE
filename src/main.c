#include <pebble.h>

enum ConfigKeys {
	CONFIG_KEY_INV=1,
	CONFIG_KEY_VIBR=2,
	CONFIG_KEY_DATEFMT=3
};

typedef struct {
	bool inv;
	bool vibr;
	uint16_t datefmt;
} CfgDta_t;

static const uint32_t const segments[] = {100, 100, 100};
static const VibePattern vibe_pat = {
	.durations = segments,
	.num_segments = ARRAY_LENGTH(segments),
};

Window *window;
TextLayer *ddmm_layer, *yyyy_layer, *hhmm_layer, *ss_layer, *wd_layer;
BitmapLayer *background_layer, *radio_layer, *battery_layer, *dst_layer;
InverterLayer *inv_layer;

static GBitmap *background, *radio, *batteryAll;
static GFont digitS, digitM, digitL;
static CfgDta_t CfgData;

char ddmmBuffer[] = "00-00";
char yyyyBuffer[] = "0000";
char hhmmBuffer[] = "00:00";
char ssBuffer[] = "00";
char wdBuffer[] = "XXX";

//-----------------------------------------------------------------------------------------------------------------------
char *upcase(char *str) {
    for (int i = 0; str[i] != 0; i++) {
        if (str[i] >= 'a' && str[i] <= 'z') {
            str[i] -= 'a' - 'A';
        }
    }
    return str;
}
//-----------------------------------------------------------------------------------------------------------------------
void battery_state_service_handler(BatteryChargeState charge_state) 
{
	int nImage = 0;
	if (charge_state.is_charging)
		nImage = 10;
	else 
		nImage = 10 - (charge_state.charge_percent / 10);
	
	GRect sub_rect = GRect(0, 10*nImage, 20, 10);
	bitmap_layer_set_bitmap(battery_layer, gbitmap_create_as_sub_bitmap(batteryAll, sub_rect));
}
//-----------------------------------------------------------------------------------------------------------------------
void bluetooth_connection_handler(bool connected)
{
	layer_set_hidden(bitmap_layer_get_layer(radio_layer), connected != true);
}
//-----------------------------------------------------------------------------------------------------------------------
void tick_handler(struct tm *tick_time, TimeUnits units_changed)
{
	int seconds = tick_time->tm_sec;

	strftime(ssBuffer, sizeof(ssBuffer), "%S", tick_time);
	text_layer_set_text(ss_layer, ssBuffer);

	if(seconds == 0 || units_changed == MINUTE_UNIT)
	{
		if(clock_is_24h_style())
			strftime(hhmmBuffer, sizeof(hhmmBuffer), "%H:%M", tick_time);
		else
			strftime(hhmmBuffer, sizeof(hhmmBuffer), "%I:%M", tick_time);
		
		//strcpy(hhmmBuffer, "22:22");
		text_layer_set_text(hhmm_layer, hhmmBuffer);
		
		strftime(ddmmBuffer, sizeof(ddmmBuffer), 
			CfgData.datefmt == 1 ? "%d-%m" : 
			CfgData.datefmt == 2 ? "%d/%m" : 
			CfgData.datefmt == 3 ? "%m/%d" : "%d.%m", tick_time);
		//snprintf(ddmmBuffer, sizeof(ddmmBuffer), "%d", rc.origin.x);
		text_layer_set_text(ddmm_layer, ddmmBuffer);
		
		strftime(wdBuffer, sizeof(wdBuffer), "%a", tick_time);
		upcase(wdBuffer);
		text_layer_set_text(wd_layer, wdBuffer);

		strftime(yyyyBuffer, sizeof(yyyyBuffer), "%Y", tick_time);
		text_layer_set_text(yyyy_layer, yyyyBuffer);

		//Check DST at 4h at morning
		if ((tick_time->tm_hour == 4 && tick_time->tm_min == 0) || units_changed == MINUTE_UNIT)
			layer_set_hidden(bitmap_layer_get_layer(dst_layer), tick_time->tm_isdst != 1);
		
		//Hourly vibrate
		if (CfgData.vibr && tick_time->tm_min == 0)
			vibes_enqueue_custom_pattern(vibe_pat); 	
	}
}
//-----------------------------------------------------------------------------------------------------------------------
static void update_configuration(void)
{
    if (persist_exists(CONFIG_KEY_INV))
		CfgData.inv = persist_read_bool(CONFIG_KEY_INV);
	else	
		CfgData.inv = false;
	
    if (persist_exists(CONFIG_KEY_VIBR))
		CfgData.vibr = persist_read_bool(CONFIG_KEY_VIBR);
	else	
		CfgData.vibr = false;
	
    if (persist_exists(CONFIG_KEY_DATEFMT))
		CfgData.datefmt = (int16_t)persist_read_int(CONFIG_KEY_DATEFMT);
	else	
		CfgData.datefmt = 1;

	app_log(APP_LOG_LEVEL_DEBUG, __FILE__, __LINE__, "Curr Conf: inv:%d, vibr:%d, datefmt:%d", CfgData.inv, CfgData.vibr, CfgData.datefmt);
	
	Layer *window_layer = window_get_root_layer(window);

	//Inverter Layer
	layer_remove_from_parent(inverter_layer_get_layer(inv_layer));
	if (CfgData.inv)
		layer_add_child(window_layer, inverter_layer_get_layer(inv_layer));
	
	//Get a time structure so that it doesn't start blank
	time_t temp = time(NULL);
	struct tm *t = localtime(&temp);

	//Manually call the tick handler when the window is loading
	tick_handler(t, MINUTE_UNIT);

	//Set Battery state
	BatteryChargeState btchg = battery_state_service_peek();
	battery_state_service_handler(btchg);
	
	//Set Bluetooth state
	bool connected = bluetooth_connection_service_peek();
	bluetooth_connection_handler(connected);
}
//-----------------------------------------------------------------------------------------------------------------------
void in_received_handler(DictionaryIterator *received, void *ctx)
{
	app_log(APP_LOG_LEVEL_DEBUG, __FILE__, __LINE__, "enter in_received_handler");
    
	Tuple *akt_tuple = dict_read_first(received);
    while (akt_tuple)
    {
        app_log(APP_LOG_LEVEL_DEBUG,
                __FILE__,
                __LINE__,
                "KEY %d=%s", (int16_t)akt_tuple->key,
                akt_tuple->value->cstring);

		if (akt_tuple->key == CONFIG_KEY_INV)
			persist_write_bool(CONFIG_KEY_INV, strcmp(akt_tuple->value->cstring, "yes") == 0);

		if (akt_tuple->key == CONFIG_KEY_VIBR)
			persist_write_bool(CONFIG_KEY_VIBR, strcmp(akt_tuple->value->cstring, "yes") == 0);
		
		if (akt_tuple->key == CONFIG_KEY_DATEFMT)
			persist_write_int(CONFIG_KEY_DATEFMT, 
				strcmp(akt_tuple->value->cstring, "fra") == 0 ? 1 : 
				strcmp(akt_tuple->value->cstring, "eng") == 0 ? 2 : 
				strcmp(akt_tuple->value->cstring, "usa") == 0 ? 3 : 0);
		
		akt_tuple = dict_read_next(received);
	}
	
    update_configuration();
}
//-----------------------------------------------------------------------------------------------------------------------
void in_dropped_handler(AppMessageResult reason, void *ctx)
{
    app_log(APP_LOG_LEVEL_WARNING,
            __FILE__,
            __LINE__,
            "Message dropped, reason code %d",
            reason);
}
//-----------------------------------------------------------------------------------------------------------------------
void window_load(Window *window)
{
	Layer *window_layer = window_get_root_layer(window);

	//Init Background
	background = gbitmap_create_with_resource(RESOURCE_ID_RESOURCE_ID_IMAGE_BACKGROUND);
	background_layer = bitmap_layer_create(GRect(0, 0, 144, 168));
	bitmap_layer_set_background_color(background_layer, GColorClear);
	bitmap_layer_set_bitmap(background_layer, background);
	layer_add_child(window_layer, bitmap_layer_get_layer(background_layer));
	
	digitS = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_RESOURCE_ID_FONT_DIGITAL_25));
	digitM = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_RESOURCE_ID_FONT_DIGITAL_35));
	digitL = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_RESOURCE_ID_FONT_DIGITAL_55));
  
	//DAY+MONTH layer
	ddmm_layer = text_layer_create(GRect(2, 5, 70, 32));
	text_layer_set_background_color(ddmm_layer, GColorClear);
	text_layer_set_text_color(ddmm_layer, GColorBlack);
	text_layer_set_text_alignment(ddmm_layer, GTextAlignmentCenter);
	text_layer_set_font(ddmm_layer, digitS);
	layer_add_child(window_layer, text_layer_get_layer(ddmm_layer));
        
	//YEAR layer
	yyyy_layer = text_layer_create(GRect(82, 5, 60, 32));
	text_layer_set_background_color(yyyy_layer, GColorClear);
	text_layer_set_text_color(yyyy_layer, GColorBlack);
	text_layer_set_text_alignment(yyyy_layer, GTextAlignmentCenter);
	text_layer_set_font(yyyy_layer, digitS);
	layer_add_child(window_layer, text_layer_get_layer(yyyy_layer));
        
	//HOUR+MINUTE layer
	hhmm_layer = text_layer_create(GRect(2, 50, 110, 75));
	text_layer_set_background_color(hhmm_layer, GColorClear);
	text_layer_set_text_color(hhmm_layer, GColorBlack);
	text_layer_set_text_alignment(hhmm_layer, GTextAlignmentCenter);
	text_layer_set_font(hhmm_layer, digitL);
	layer_add_child(window_layer, text_layer_get_layer(hhmm_layer));
        
	//SECOND layer
	ss_layer = text_layer_create(GRect(111, 55, 30, 30));
	text_layer_set_background_color(ss_layer, GColorClear);
	text_layer_set_text_color(ss_layer, GColorBlack);
	text_layer_set_text_alignment(ss_layer, GTextAlignmentCenter);
	text_layer_set_font(ss_layer, digitS);
	layer_add_child(window_layer, text_layer_get_layer(ss_layer));
        
	//Init battery
	batteryAll = gbitmap_create_with_resource(RESOURCE_ID_RESOURCE_ID_IMAGE_BATTERIES);
	battery_layer = bitmap_layer_create(GRect(116, 90, 20, 10)); 
	bitmap_layer_set_background_color(battery_layer, GColorClear);
	layer_add_child(window_layer, bitmap_layer_get_layer(battery_layer));

	//DST layer, have to be after battery, uses its image
	dst_layer = bitmap_layer_create(GRect(123, 52, 12, 5));
	bitmap_layer_set_background_color(dst_layer, GColorClear);
	bitmap_layer_set_bitmap(dst_layer, gbitmap_create_as_sub_bitmap(batteryAll, GRect(0, 110, 12, 5)));
	layer_add_child(window_layer, bitmap_layer_get_layer(dst_layer));
        
	//WEEKDAY layer
	wd_layer = text_layer_create(GRect(3, 125, 84, 40));
	text_layer_set_background_color(wd_layer, GColorClear);
	text_layer_set_text_color(wd_layer, GColorBlack);
	text_layer_set_text_alignment(wd_layer, GTextAlignmentCenter);
	text_layer_set_font(wd_layer, digitM);
	layer_add_child(window_layer, text_layer_get_layer(wd_layer));
        
	//Init bluetooth radio
	radio = gbitmap_create_with_resource(RESOURCE_ID_RESOURCE_ID_IMAGE_RADIO);
	radio_layer = bitmap_layer_create(GRect(106, 130, 31, 33));
	bitmap_layer_set_background_color(radio_layer, GColorClear);
	bitmap_layer_set_bitmap(radio_layer, radio);
	bitmap_layer_set_compositing_mode(radio_layer, GCompOpAnd);
	layer_add_child(window_layer, bitmap_layer_get_layer(radio_layer));
	
	//Init inverter_layer
	inv_layer = inverter_layer_create(GRect(0, 0, 144, 168));

	//Update Configuration
	update_configuration();
}
//-----------------------------------------------------------------------------------------------------------------------
void window_unload(Window *window)
{
	//Destroy text layers
	text_layer_destroy(ddmm_layer);
	text_layer_destroy(yyyy_layer);
	text_layer_destroy(hhmm_layer);
	text_layer_destroy(ss_layer);
	text_layer_destroy(wd_layer);
	
	//Unload Fonts
	fonts_unload_custom_font(digitS);
	fonts_unload_custom_font(digitM);
	fonts_unload_custom_font(digitL);
	
	//Destroy GBitmaps
	gbitmap_destroy(batteryAll);
	gbitmap_destroy(radio);
	gbitmap_destroy(background);

	//Destroy BitmapLayers
	bitmap_layer_destroy(dst_layer);
	bitmap_layer_destroy(battery_layer);
	bitmap_layer_destroy(radio_layer);
	bitmap_layer_destroy(background_layer);
	
	//Destroy Inverter Layer
	inverter_layer_destroy(inv_layer);
}
//-----------------------------------------------------------------------------------------------------------------------
void handle_init(void) 
{
	window = window_create();
	window_set_window_handlers(window, (WindowHandlers) {
		.load = window_load,
		.unload = window_unload,
	});
    window_stack_push(window, true);
	
	//Subscribe services
	tick_timer_service_subscribe(SECOND_UNIT, (TickHandler)tick_handler);
	battery_state_service_subscribe(&battery_state_service_handler);
	bluetooth_connection_service_subscribe(&bluetooth_connection_handler);

	//Subscribe messages
	app_message_register_inbox_received(in_received_handler);
    app_message_register_inbox_dropped(in_dropped_handler);
    app_message_open(128, 128);
	
	APP_LOG(APP_LOG_LEVEL_DEBUG, "Done initializing, pushed window: %p", window);
}
//-----------------------------------------------------------------------------------------------------------------------
void handle_deinit(void) 
{
	app_message_deregister_callbacks();
	tick_timer_service_unsubscribe();
	battery_state_service_unsubscribe();
	bluetooth_connection_service_unsubscribe();
	window_destroy(window);
}
//-----------------------------------------------------------------------------------------------------------------------
int main(void) 
{
	  handle_init();
	  app_event_loop();
	  handle_deinit();
}
//-----------------------------------------------------------------------------------------------------------------------