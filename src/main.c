//56b37c6f-792a-480f-b962-9a0db8c32aa4
#include "pebble.h"
#define UPDATE_MS 50 // Refresh rate in milliseconds

static bool up_button_depressed = false;
static bool dn_button_depressed = false;
static bool sl_button_depressed = false;

static Window *main_window;
static Layer *graphics_layer;

#define MAX_STARS 200
struct starstruct {
	uint8_t x;
  uint8_t y;
  uint8_t z;
} *star[MAX_STARS];

struct playerstruct {
  uint8_t x;
  uint8_t y;
  uint8_t speed;
} player;

int16_t sign16(int16_t x) {return ((x > 0) - (x < 0));}

// ------------------------------------------------------------------------ //
//  Timer Functions
// ------------------------------------------------------------------------ //
static void timer_callback(void *data) {
  AccelData accel = (AccelData) { .x = 0, .y = 0, .z = 0 };
  accel_service_peek(&accel);        // Read accelerometer

  int16_t targetspeed;
  if(up_button_depressed) {        //     up = megaboost
    targetspeed = 32;
  } else if(sl_button_depressed) { // select = boost
    targetspeed = 16;
  } else if(dn_button_depressed) { //   down = brakes
    targetspeed = 0;
  } else {
    targetspeed = 1;
  }
  player.speed += sign16(targetspeed - player.speed);
  
  for(uint16_t i = 0; i < MAX_STARS; i++) {
    if(star[i]->z <= player.speed) {
      star[i]->x = rand() & 255;
		  star[i]->y = rand() & 255;
      star[i]->z = (rand() % 255)+1;
    } else {
     star[i]->z -= player.speed;
    }
  }

  player.x += accel.x>>6;
  player.y -= accel.y>>6;

  layer_mark_dirty(graphics_layer);  // Schedule redraw of screen
}

// ------------------------------------------------------------------------ //
//  Button Functions
// ------------------------------------------------------------------------ //
static void up_push_in_handler(ClickRecognizerRef recognizer, void *context) {up_button_depressed = true;}
static void up_release_handler(ClickRecognizerRef recognizer, void *context) {up_button_depressed = false;}
static void dn_push_in_handler(ClickRecognizerRef recognizer, void *context) {dn_button_depressed = true;}
static void dn_release_handler(ClickRecognizerRef recognizer, void *context) {dn_button_depressed = false;}
static void sl_push_in_handler(ClickRecognizerRef recognizer, void *context) {sl_button_depressed = true;}
static void sl_release_handler(ClickRecognizerRef recognizer, void *context) {sl_button_depressed = false;}
  
static void click_config_provider(void *context) {
  window_raw_click_subscribe(BUTTON_ID_UP, up_push_in_handler, up_release_handler, context);
  window_raw_click_subscribe(BUTTON_ID_DOWN, dn_push_in_handler, dn_release_handler, context);
  window_raw_click_subscribe(BUTTON_ID_SELECT, sl_push_in_handler, sl_release_handler, context);
}
  
// ------------------------------------------------------------------------ //
//  Drawing Functions
// ------------------------------------------------------------------------ //
static void draw_textbox(GContext *ctx, GRect textframe, char *text) {
  graphics_context_set_fill_color(ctx, 0);   graphics_fill_rect(ctx, textframe, 0, GCornerNone);  //Black Solid Rectangle
  graphics_context_set_stroke_color(ctx, 1); graphics_draw_rect(ctx, textframe);                  //White Rectangle Border
  graphics_context_set_text_color(ctx, 1);  // White Text
  graphics_draw_text(ctx, text, fonts_get_system_font(FONT_KEY_GOTHIC_14), textframe, GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);  //Write Text
}  

static void draw_stars(GContext *ctx, GRect frame) {
  graphics_context_set_fill_color(ctx, 0);   graphics_fill_rect(ctx, frame, 0, GCornerNone);  //Black Solid Rectangle
  graphics_context_set_stroke_color(ctx, 1); graphics_draw_rect(ctx, frame);                  //White Rectangle Border

  graphics_context_set_stroke_color(ctx, 1);
  uint8_t tx, ty;
  int16_t px, py;
  
	for(uint16_t i = 0; i < MAX_STARS; i++) {
    // take difference, loop over 65536
    tx = star[i]->x - player.x;
    ty = star[i]->y - player.y;
    
    // convert [0-65536] to [-32768 - 32767]
    px = tx; py = ty;
    px -= 128; py -= 128;
    
    // calculate pixel x and y
    px = (px << 8) / star[i]->z;
    py = (py << 8) / star[i]->z;
    
    //graphics_draw_pixel(ctx, GPoint(px+72,py+84));
    graphics_draw_pixel(ctx, GPoint(px + frame.origin.x + (frame.size.w/2), py + frame.origin.y + (frame.size.h / 2)));
  }  

}

static void graphics_layer_update(Layer *me, GContext *ctx) {

  draw_stars(ctx, GRect(1, 22, 142, 143));
  
  static char text[40];  //Buffer to hold text
  snprintf(text, sizeof(text), "(%d,%d) %d", player.x, player.y, player.speed);  // What text to draw
  draw_textbox(ctx, GRect(1, 1, 142, 20), text);

  app_timer_register(UPDATE_MS, timer_callback, NULL); // Schedule a callback
}


// ------------------------------------------------------------------------ //
//  Star Functions
// ------------------------------------------------------------------------ //
static void init_stars() {
	for(uint16_t i = 0; i < MAX_STARS; i++) {
    struct starstruct *tempstar = malloc(sizeof(struct starstruct));
    star[i] = tempstar;
    
		star[i]->x = rand() & 255;
		star[i]->y = rand() & 255;
    star[i]->z = rand() & 255;
  }  
}

// ------------------------------------------------------------------------ //
//  Main Functions
// ------------------------------------------------------------------------ //
static void main_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  graphics_layer = layer_create(layer_get_frame(window_layer));
  layer_set_update_proc(graphics_layer, graphics_layer_update);
  layer_add_child(window_layer, graphics_layer);
}

static void main_window_unload(Window *window) {
  layer_destroy(graphics_layer);
}

static void main_window_appear(Window *window) {}
static void main_window_disappear(Window *window) {}

static void init(void) {
  // Set up and push main window
  main_window = window_create();
  window_set_window_handlers(main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload,
    .appear = main_window_appear,
    .disappear = main_window_disappear
  });
  window_set_click_config_provider(main_window, click_config_provider);
  window_set_background_color(main_window, 0);
  window_set_fullscreen(main_window, true);

  //Set up other stuff
  //srand(time(NULL));  // Seed randomizer
  srand(1);  // Seed randomizer
  
  accel_data_service_subscribe(0, NULL);  // We will be using the accelerometer
  //font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_PIXEL_8));  // Load font
  init_stars();
  player.x=0; player.y=0; player.speed=0; // init player
  //Begin
  window_stack_push(main_window, true /* Animated */); // Display window.  Callback will be called once layer is dirtied then written
}
  
static void deinit(void) {
  accel_data_service_unsubscribe();
  window_destroy(main_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}

// ------------------------------------------------------------------------ //
//  Notes
// ------------------------------------------------------------------------ //
/*
#define FONT_KEY_FONT_FALLBACK "RESOURCE_ID_FONT_FALLBACK"
#define FONT_KEY_GOTHIC_14 "RESOURCE_ID_GOTHIC_14"
#define FONT_KEY_GOTHIC_14_BOLD "RESOURCE_ID_GOTHIC_14_BOLD"
#define FONT_KEY_GOTHIC_18 "RESOURCE_ID_GOTHIC_18"
#define FONT_KEY_GOTHIC_18_BOLD "RESOURCE_ID_GOTHIC_18_BOLD"
#define FONT_KEY_GOTHIC_24 "RESOURCE_ID_GOTHIC_24"
#define FONT_KEY_GOTHIC_24_BOLD "RESOURCE_ID_GOTHIC_24_BOLD"
#define FONT_KEY_GOTHIC_28 "RESOURCE_ID_GOTHIC_28"
#define FONT_KEY_GOTHIC_28_BOLD "RESOURCE_ID_GOTHIC_28_BOLD"
#define FONT_KEY_BITHAM_30_BLACK "RESOURCE_ID_BITHAM_30_BLACK"
#define FONT_KEY_BITHAM_42_BOLD "RESOURCE_ID_BITHAM_42_BOLD"
#define FONT_KEY_BITHAM_42_LIGHT "RESOURCE_ID_BITHAM_42_LIGHT"
#define FONT_KEY_BITHAM_42_MEDIUM_NUMBERS "RESOURCE_ID_BITHAM_42_MEDIUM_NUMBERS"
#define FONT_KEY_BITHAM_34_MEDIUM_NUMBERS "RESOURCE_ID_BITHAM_34_MEDIUM_NUMBERS"
#define FONT_KEY_BITHAM_34_LIGHT_SUBSET "RESOURCE_ID_BITHAM_34_LIGHT_SUBSET"
#define FONT_KEY_BITHAM_18_LIGHT_SUBSET "RESOURCE_ID_BITHAM_18_LIGHT_SUBSET"
#define FONT_KEY_ROBOTO_CONDENSED_21 "RESOURCE_ID_ROBOTO_CONDENSED_21"
#define FONT_KEY_ROBOTO_BOLD_SUBSET_49 "RESOURCE_ID_ROBOTO_BOLD_SUBSET_49"
#define FONT_KEY_DROID_SERIF_28_BOLD "RESOURCE_ID_DROID_SERIF_28_BOLD"





*/