#include "pebble.h"

#define DELAY_INTERVAL_MS 500

// This is a custom defined key for saving our count field
#define NUM_SUSHI_PKEY 0
#define NUM_SASHIMI_PKEY 1
#define NUM_MISCELLANEOUS_PKEY 2

#define NUM_PER_KEY 1
#define NUM_PER_HOLD 8

// You can define defaults for values in persistent storage
#define NUM_DEFAULT 0

#define NAME_SIZE 20

#define PAGE_COUNT 3
#define PAGE_DEFAULT 0

typedef struct{
  uint16_t s_num_count;
  char name[NAME_SIZE];
} s_item_params;

static s_item_params sushi = {NUM_DEFAULT, "sushi"};
static s_item_params sashimi = {NUM_DEFAULT, "sashimi"};
static s_item_params miscellaneous = {NUM_DEFAULT, "miscellaneous"};
static s_item_params *ref;

static Window *s_main_window;

static ActionBarLayer *s_action_bar;
static TextLayer *s_header_layer, *s_body_layer, *s_label_layer;
static GBitmap *s_icon_plus, *s_icon_minus, *s_icon_select;

static uint8_t page = PAGE_DEFAULT;

static void deinit();

static void update_text() {
  static char s_body_text[18];
  static char s_sub_text[NAME_SIZE + 14];

  switch(ref->s_num_count){
    case 1:
      snprintf(s_body_text, sizeof(s_body_text), "%u Piece", ref->s_num_count);
      break;
    default:
      snprintf(s_body_text, sizeof(s_body_text), "%u Pieces", ref->s_num_count);
  }
  text_layer_set_text(s_body_layer, s_body_text);
  snprintf(s_sub_text, sizeof(s_sub_text), "of %s in my gut", ref->name);
  text_layer_set_text(s_label_layer, s_sub_text);
}

static void increment_click_handler(ClickRecognizerRef recognizer, void *context) {
  ref->s_num_count++;
  update_text();
}

static void decrement_click_handler(ClickRecognizerRef recognizer, void *context) {
  if (ref->s_num_count <= 0) {
    // Keep the counter at zero
    return;
  }

  ref->s_num_count--;
  update_text();
}

static void update_ref(uint8_t ref_num){
  switch(ref_num){
    case NUM_SUSHI_PKEY:
      ref = &sushi;
      break;
    case NUM_SASHIMI_PKEY:
      ref = &sashimi;
      break;
    case NUM_MISCELLANEOUS_PKEY:
      ref = &miscellaneous;
      break;
    default:
      APP_LOG(APP_LOG_LEVEL_WARNING, "No data for page " +  page);
  }
}

static void switch_handler(ClickRecognizerRef recognizer, void *context) { 
  page = (page + 1) % PAGE_COUNT;
  update_ref(page);
  update_text();
}

static void reset_handler(ClickRecognizerRef recognizer, void *context){
  for(uint8_t c = 0; c < PAGE_COUNT; ++c){
    update_ref(c);
    ref->s_num_count = 0;
  }
  update_ref(page);
  update_text();
}

static void increment_multi_handler(ClickRecognizerRef recognizer, void *context){
  ref->s_num_count += NUM_PER_HOLD;
  update_text();
}

static void decrement_multi_handler(ClickRecognizerRef recognizer, void *context){
  if(ref->s_num_count <= NUM_PER_HOLD){
    ref->s_num_count = 0;
  } else {
    ref->s_num_count -= NUM_PER_HOLD;
  }
  update_text();
}

static void click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_UP, increment_click_handler);
  window_single_click_subscribe(BUTTON_ID_DOWN, decrement_click_handler);
  window_single_click_subscribe(BUTTON_ID_SELECT, switch_handler);
  window_long_click_subscribe(BUTTON_ID_UP, DELAY_INTERVAL_MS, increment_multi_handler, NULL);
  window_long_click_subscribe(BUTTON_ID_DOWN, DELAY_INTERVAL_MS, decrement_multi_handler, NULL);
  window_long_click_subscribe(BUTTON_ID_SELECT, DELAY_INTERVAL_MS, reset_handler, NULL);
}

static void main_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);

  s_action_bar = action_bar_layer_create();
  action_bar_layer_add_to_window(s_action_bar, window);
  action_bar_layer_set_click_config_provider(s_action_bar, click_config_provider);

  action_bar_layer_set_icon(s_action_bar, BUTTON_ID_UP, s_icon_plus);
  action_bar_layer_set_icon(s_action_bar, BUTTON_ID_DOWN, s_icon_minus);
  action_bar_layer_set_icon(s_action_bar, BUTTON_ID_SELECT, s_icon_select);

  int width = layer_get_frame(window_layer).size.w - ACTION_BAR_WIDTH - 3;

  s_header_layer = text_layer_create(GRect(4, 0, width, 60));
  text_layer_set_font(s_header_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24));
  text_layer_set_background_color(s_header_layer, GColorClear);
  text_layer_set_text(s_header_layer, "Sushi Counter");
  layer_add_child(window_layer, text_layer_get_layer(s_header_layer));

  s_body_layer = text_layer_create(GRect(4, 44, width, 60));
  text_layer_set_font(s_body_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
  text_layer_set_background_color(s_body_layer, GColorClear);
  layer_add_child(window_layer, text_layer_get_layer(s_body_layer));

  s_label_layer = text_layer_create(GRect(4, 44 + 28, width, 60));  
  text_layer_set_font(s_label_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
  text_layer_set_background_color(s_label_layer, GColorClear);
  layer_add_child(window_layer, text_layer_get_layer(s_label_layer));

  update_text();
}

static void main_window_unload(Window *window) {
  text_layer_destroy(s_header_layer);
  text_layer_destroy(s_body_layer);
  text_layer_destroy(s_label_layer);

  action_bar_layer_destroy(s_action_bar);
}

static void init() {
  s_icon_plus = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_ACTION_ICON_PLUS);
  s_icon_minus = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_ACTION_ICON_MINUS);
  s_icon_select = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_ACTION_ICON_SELECT);

  // Get the count from persistent storage for use if it exists, otherwise use the default
  for(int c = 0; c < PAGE_COUNT; ++c){
    update_ref(c);
    ref->s_num_count = persist_exists(c) ? persist_read_int(c) : NUM_DEFAULT;
  }
  update_ref(PAGE_DEFAULT);
  s_main_window = window_create();
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload,
  });
  window_stack_push(s_main_window, true);
}

static void deinit() {
  // Save the count into persistent storage on app exit
  for(int c = 0; c < PAGE_COUNT; ++c){
    update_ref(c);
    persist_write_int(c, ref->s_num_count);
  }

  window_destroy(s_main_window);

  gbitmap_destroy(s_icon_plus);
  gbitmap_destroy(s_icon_minus);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
