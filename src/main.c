#include <pebble.h>

Window *my_window;
TextLayer *text_layer;

void handle_init(void) {
	  my_window = window_create();

	  text_layer = text_layer_create(GRect(0, 0, 144, 20));
}

void handle_deinit(void) {
	  text_layer_destroy(text_layer);
	  window_destroy(my_window);
}

int main(void) {
	  handle_init();
	  app_event_loop();
	  handle_deinit();
}
