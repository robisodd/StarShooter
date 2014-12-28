//56b37c6f-792a-480f-b962-9a0db8c32aa4
#include "pebble.h"
#define UPDATE_MS 50 // Refresh rate in milliseconds

static bool up_button_depressed = false;
static bool dn_button_depressed = false;
static bool sl_button_depressed = false;

static Window *main_window;
static Layer *graphics_layer;

struct xyz {
	uint8_t x;
  uint8_t y;
  uint8_t z;
};

#define MAX_STARS 200
struct xyz *star[MAX_STARS];

struct {
	int8_t x;
  int8_t y;
  int8_t z;
} speed;

struct playerstruct {
  struct xyz pos;
  struct xyz spd;
  struct xyz fac;
} player;

int16_t sign16(int16_t x) {return ((x > 0) - (x < 0));}
uint16_t a=0, b=0;

// ------------------------------------------------------------------------ //
//  Timer Functions
// ------------------------------------------------------------------------ //
static void timer_callback(void *data) {
  AccelData accel = (AccelData) { .x = 0, .y = 0, .z = 0 };
  accel_service_peek(&accel);        // Read accelerometer

 
  for(uint16_t i = 0; i < MAX_STARS; i++) {
    star[i]->x -= speed.x;
    star[i]->y -= speed.y;
    star[i]->z -= speed.z;
  }
  
  if(up_button_depressed) { // stop rotation, but don't stop movement
    a=0; b=0;
  }
  
  if(dn_button_depressed) { // move
    a += accel.x;
    b -= accel.y;
  }
  
  if(sl_button_depressed) { // thrust
    int32_t px, py, pz;
    int32_t qx, qy, qz;
    
    px = 0;
    py = 0;
    pz = 1024;
    
    qx = ((px * cos_lookup(a)) + (pz * sin_lookup(a))) / TRIG_MAX_RATIO;
    qy = py;
    qz = ((px * -sin_lookup(a)) + (pz * cos_lookup(a))) / TRIG_MAX_RATIO;
    
    px = qx;
    py = ((qy * cos_lookup(b)) + (qz * -sin_lookup(b))) / TRIG_MAX_RATIO;
    pz = ((qy * sin_lookup(b)) + (qz * cos_lookup(b))) / TRIG_MAX_RATIO;
    
    speed.x += qx >> 8; if(speed.x>15) speed.x=15; if(speed.x<-15) speed.x=-15;
    speed.y += qy >> 8; if(speed.y>15) speed.y=15; if(speed.y<-15) speed.y=-15;
    speed.z += qz >> 8; if(speed.z>15) speed.z=15; if(speed.z<-15) speed.z=-15;
  }
  
  //player.x += accel.x>>6;
  //player.y -= accel.y>>6;

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
  //uint8_t tx, ty;
  int32_t px, py, pz;
  int32_t qx, qy, qz;
  
	for(uint16_t i = 0; i < MAX_STARS; i++) {
    px = star[i]->x;
    py = star[i]->y;
    pz = star[i]->z;
    px -= 128; py -= 128; pz -= 128;
    
    qx = ((px * cos_lookup(a)) + (pz * sin_lookup(a))) / TRIG_MAX_RATIO;
    qy = py;
    qz = ((px * -sin_lookup(a)) + (pz * cos_lookup(a))) / TRIG_MAX_RATIO;
    
    px = qx;
    py = ((qy * cos_lookup(b)) + (qz * -sin_lookup(b))) / TRIG_MAX_RATIO;
    pz = ((qy * sin_lookup(b)) + (qz * cos_lookup(b))) / TRIG_MAX_RATIO;
    
    if(pz>0){
      px = (px << 8) / pz;
      py = (py << 8) / pz;
      graphics_draw_pixel(ctx, GPoint(px + frame.origin.x + (frame.size.w/2), py + frame.origin.y + (frame.size.h / 2)));
    }
    
    
/*
x' = (x * cos(a)) + (y * -sin(a))
y' = (x * sin(a)) + (y * cos(a))
z' = z
Likewise, a rotation around the Y axis keeps the Y value alone, but involves trig functions on the X and Z values. Here's the function list for rotating around the Y axis:
x' = (x * cos(a)) + (z * sin(a))
y' = y
z' = (x * -sin(a)) + (z * cos(a))
... and rotation about the x axis uses a similar set of functions.
x' = x
y' = (y * cos(a)) + (z * -sin(a))
z' = (y * sin(a)) + (z * cos(a))
*/

/*
    // take difference, loop over 65536
    tx = star[i]->x - player.x;
    ty = star[i]->y - player.y;
    
    // convert [0-65536] to [-32768 - 32767]
    px = tx; py = ty;
    px -= 128; py -= 128;
    
    // calculate pixel x and y
    px = (px << 8) / star[i]->z;
    py = (py << 8) / star[i]->z;
*/
    //graphics_draw_pixel(ctx, GPoint(px+72,py+84));
    
  }  

}

static void graphics_layer_update(Layer *me, GContext *ctx) {

  draw_stars(ctx, GRect(1, 22, 142, 143));
  
  static char text[40];  //Buffer to hold text
  //snprintf(text, sizeof(text), "(%d,%d) %d", player.x, player.y, player.speed);  // What text to draw
  snprintf(text, sizeof(text), "%d %d", a, b);  // What text to draw
  draw_textbox(ctx, GRect(1, 1, 142, 20), text);

  app_timer_register(UPDATE_MS, timer_callback, NULL); // Schedule a callback
}


// ------------------------------------------------------------------------ //
//  Star Functions
// ------------------------------------------------------------------------ //
static void init_stars() {
	for(uint16_t i = 0; i < MAX_STARS; i++) {
    struct xyz *temp = malloc(sizeof(struct xyz));
    star[i] = temp;
    // change to star[i] = malloc(sizeof(struct xyz));
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
  //player.x=0; player.y=0; player.speed=0; // init player
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