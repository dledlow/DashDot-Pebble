#include <pebble.h>
#include <ctype.h>

Window *window;	
static TextLayer *s_time_layer;

void send_message(void);
void send_custom_message(char* msg);
static void main_window_update(Window *window, char* text);
static void update_time();
void addMorseLetter(int buttonPushed);

static bool showingWatchFace = true;
	
// Key values for AppMessage Dictionary
enum {
	STATUS_KEY = 0,	
	MESSAGE_KEY = 1,
  STRLEN_KEY = 2,
  STR_KEY = 3
};

char* currPhrase;

int letterIndex = 0;
char* morseLetter;

int wordIndex = 0;
char* sendingWord;

struct codeData {
  char* code;
  int codelen;
  int index;
};

static int const dotlength = 200;
static uint32_t dahs[] = { 600 };
static uint32_t dits[] = { 200 };

VibePattern dah = {
  .durations = dahs,
  .num_segments = ARRAY_LENGTH(dahs),
};
VibePattern dit = {
  .durations = dits,
  .num_segments = ARRAY_LENGTH(dits),
};

static Layer *slayer;

static void tap_handler(AccelAxisType axis, int32_t direction) {
  if (wordIndex == 0){
    return;
  }
  //send word
  addMorseLetter(2);
  wordIndex = 0;
  send_custom_message(sendingWord);
  free(sendingWord);
  sendingWord = calloc(256, sizeof(char));
}

char lookupMorseLetter(char* lookup) {
  if (strcmp("01",lookup) == 0){
    return 'A';
  } else if (strcmp("1000",lookup) == 0){
    return 'B';
  } else if (strcmp("1010",lookup) == 0){
    return 'C';
  } else if (strcmp("100",lookup) == 0){
    return 'D';
  } else if (strcmp("0",lookup) == 0){
    return 'E';
  } else if (strcmp("0010",lookup) == 0){
    return 'F';
  } else if (strcmp("110",lookup) == 0){
    return 'G';
  } else if (strcmp("0000",lookup) == 0){
    return 'H';
  } else if (strcmp("00",lookup) == 0){
    return 'I';
  } else if (strcmp("0111",lookup) == 0){
    return 'J';
  } else if (strcmp("101",lookup) == 0){
    return 'K';
  } else if (strcmp("0100",lookup) == 0){
    return 'L';
  } else if (strcmp("11",lookup) == 0){
    return 'M';
  } else if (strcmp("10",lookup) == 0){
    return 'N';
  } else if (strcmp("111",lookup) == 0){
    return 'O';
  } else if (strcmp("0110",lookup) == 0){
    return 'P';
  } else if (strcmp("1101",lookup) == 0){
    return 'Q';
  } else if (strcmp("010",lookup) == 0){
    return 'R';
  } else if (strcmp("000",lookup) == 0){
    return 'S';
  } else if (strcmp("1",lookup) == 0){
    return 'T';
  } else if (strcmp("001",lookup) == 0){
    return 'U';
  } else if (strcmp("0001",lookup) == 0){
    return 'V';
  } else if (strcmp("011",lookup) == 0){
    return 'W';
  } else if (strcmp("1001",lookup) == 0){
    return 'X';
  } else if (strcmp("1011",lookup) == 0){
    return 'Y';
  } else if (strcmp("1100",lookup) == 0){
    return 'Z';
  }
  return '\0';
}

void addMorseLetter(int buttonPushed) {
  // 0 = dit, 1 = dah, 2 = endletter
  if (buttonPushed == 0) {
    morseLetter[letterIndex] = '0';
    letterIndex++;
  } else if (buttonPushed == 1) {
    morseLetter[letterIndex] = '1';
    letterIndex++;
  } else if (letterIndex != 0){
    letterIndex = 0;
    sendingWord[wordIndex] = lookupMorseLetter(morseLetter);
    free(morseLetter);
    morseLetter = calloc(5, sizeof(char));
    wordIndex++;
  }
  
	APP_LOG(APP_LOG_LEVEL_DEBUG, "morseChar: '%c', %d, %d", morseLetter[letterIndex], letterIndex, buttonPushed); 
  
  if (letterIndex >= 4) {
    letterIndex = 0;
    sendingWord[wordIndex] = lookupMorseLetter(morseLetter);
    free(morseLetter);
    morseLetter = calloc(5, sizeof(char));
    wordIndex++;
  }
  if (wordIndex >= 256) {
    if (wordIndex == 0){
      return;
    }
    wordIndex = 0;
    //send word
    send_custom_message(sendingWord);
    free(sendingWord);
    sendingWord = calloc(256, sizeof(char));
  }
  
}

void down_single_click_handler(ClickRecognizerRef recognizer, void *context) {
  //send_custom_message("down");
  addMorseLetter(1);
	APP_LOG(APP_LOG_LEVEL_DEBUG, "morse: '%s', '%s'", morseLetter, sendingWord); 
}
void up_single_click_handler(ClickRecognizerRef recognizer, void *context) {
  //send_custom_message("up");
  addMorseLetter(0);
	APP_LOG(APP_LOG_LEVEL_DEBUG, "morse: '%s', '%s'", morseLetter, sendingWord); 
}
void select_single_click_handler(ClickRecognizerRef recognizer, void *context) {
  //send_custom_message("select");
  addMorseLetter(2);
}
void double_click_handler(ClickRecognizerRef recognizer, void *context) {
    //send word
    addMorseLetter(2);
    wordIndex = 0;
    send_custom_message(sendingWord);
    free(sendingWord);
    sendingWord = calloc(256, sizeof(char));
}
void triple_click_handler(ClickRecognizerRef recognizer, void *context) {
  Window *window = (Window *)context;
  showingWatchFace = !showingWatchFace;
  if (!showingWatchFace) {
    main_window_update(window, currPhrase);
  } else {
    update_time();
  }
  //send_custom_message("triple click");
}

void config_provider(Window *window) {
 // single click / repeat-on-hold config:
  window_single_click_subscribe(BUTTON_ID_DOWN, down_single_click_handler);
  window_single_click_subscribe(BUTTON_ID_UP, up_single_click_handler);
  window_single_click_subscribe(BUTTON_ID_SELECT, select_single_click_handler);
  window_multi_click_subscribe(BUTTON_ID_SELECT, 3, 3, 300, true, triple_click_handler);
  window_multi_click_subscribe(BUTTON_ID_UP, 2, 2, 300, true, double_click_handler);


  // multi click config:
  //window_multi_click_subscribe(BUTTON_ID_SELECT, 2, 10, 0, true, select_multi_click_handler);

  // long click config:
  //window_long_click_subscribe(BUTTON_ID_SELECT, 700, select_long_click_handler, select_long_click_release_handler);
}

GBitmap* image;

static void some_update_proc(Layer *this_layer, GContext *ctx) {
  // Draw things here using ctx
  APP_LOG(APP_LOG_LEVEL_INFO, "%d free [post]", (int)heap_bytes_free());
  if (image == NULL)
    image = gbitmap_create_with_resource(RESOURCE_ID_PLATYPUS2);
  graphics_draw_bitmap_in_rect(ctx, image, GRect(0, 0, 130, 100));
  APP_LOG(APP_LOG_LEVEL_INFO, "%d free [post]", (int)heap_bytes_free());
}

static void update_time() {
  // Get a tm structure
  time_t temp = time(NULL); 
  struct tm *tick_time = localtime(&temp);

  // Create a long-lived buffer
  static char buffer[] = "00:00";

  // Write the current hours and minutes into the buffer
  if(clock_is_24h_style() == true) {
    //Use 2h hour format
    strftime(buffer, sizeof("00:00"), "%H:%M", tick_time);
  } else {
    //Use 12 hour format
    strftime(buffer, sizeof("00:00"), "%I:%M", tick_time);
  }

  // Display this time on the TextLayer
  text_layer_set_text(s_time_layer, buffer);
  layer_set_hidden(slayer, false);
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  if (showingWatchFace) {
    update_time();    
    APP_LOG(APP_LOG_LEVEL_INFO, "%d free [post]", (int)heap_bytes_free());
  } else {
    
  }
}

static void main_window_load(Window *window) {
  // Create time TextLayer
  GRect bounds = layer_get_bounds(window_get_root_layer(window));
  s_time_layer = text_layer_create(GRect(0, 0, bounds.size.w, bounds.size.h));
  text_layer_set_background_color(s_time_layer, GColorClear);
  text_layer_set_text_color(s_time_layer, GColorBlack);
  //text_layer_set_text(s_time_layer, "Ready");

  // Improve the layout to be more like a watchface
  text_layer_set_font(s_time_layer, fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD));
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);

  // Add it as a child layer to the Window's root layer
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_time_layer));
  
  window_set_click_config_provider(window, (ClickConfigProvider) config_provider);
  
  // Register with TickTimerService
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
  
  currPhrase = "Ready";
  morseLetter = calloc(5, sizeof(char));
  sendingWord = calloc(256, sizeof(char));
  
  slayer = layer_create(GRect(20, 50, 130, 130));
  layer_add_child((Layer *)s_time_layer, slayer);
  layer_set_update_proc(slayer, some_update_proc);
  
  //accel_tap_service_subscribe(tap_handler);
}

static void main_window_update(Window *window, char* text) {
  // Create time TextLayer
  text_layer_set_background_color(s_time_layer, GColorClear);
  text_layer_set_text_color(s_time_layer, GColorBlack);
  text_layer_set_text(s_time_layer, text);

  // Improve the layout to be more like a watchface
  text_layer_set_font(s_time_layer, fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD));
  text_layer_set_overflow_mode(s_time_layer, GTextOverflowModeWordWrap);
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);
  
  layer_set_hidden(slayer, true);
}

static void main_window_unload(Window *window) {
    // Destroy TextLayer
    text_layer_destroy(s_time_layer);
  
    free(morseLetter);
    free(sendingWord);
}

// Write message to buffer & send
void send_message(void){
	DictionaryIterator *iter;
	
	app_message_outbox_begin(&iter);
	dict_write_uint8(iter, STATUS_KEY, 0x1);
	
	dict_write_end(iter);
  app_message_outbox_send();
}

void send_custom_message(char* msg){
	DictionaryIterator *iter;
	
	app_message_outbox_begin(&iter);
	dict_write_uint8(iter, STATUS_KEY, 0x2);
	dict_write_cstring(iter, MESSAGE_KEY, msg);
	
	dict_write_end(iter);
  app_message_outbox_send();
}

static void process_code(struct codeData *data){
  if (data->codelen > 0 && data->index < data->codelen && data->index != -1){
    vibes_cancel();
		APP_LOG(APP_LOG_LEVEL_DEBUG, "ddata: %s, %d, %d", data->code, data->index, data->codelen); 
    if (data->code[data->index] == '0') {
      vibes_enqueue_custom_pattern(dit);
    } else if (data->code[data->index] == '1') {
      vibes_enqueue_custom_pattern(dah);
    }
    data->index+=1;
    int waitLength = dotlength * 4;
    if (data->code[data->index] == '3') {
      waitLength = dotlength * 8;
    }
    app_timer_register(waitLength, (AppTimerCallback)process_code, data);
  }
}

static bool matchesOne(char c, char* m, int numM) {
  for (int i = 0; i < numM; i++) {
    if (c == m[i])
      return 1;
  }
  return 0;
}

static int addCodeData(char* data, char code, int idx) {
  if (code == 'A') {
    data[idx] = '0';
    data[idx+1] = '1';
    data[idx+2] = '2';
    return 3;
  } else if (code == 'B') {
    data[idx] = '1';
    data[idx+1] = '0';
    data[idx+2] = '0';
    data[idx+3] = '0';
    data[idx+4] = '2';
    return 5;
  } else if (code == 'C') {
    data[idx] = '1';
    data[idx+1] = '0';
    data[idx+2] = '1';
    data[idx+3] = '0';
    data[idx+4] = '2';
    return 5;
  } else if (code == 'D') {
    data[idx] = '1';
    data[idx+1] = '0';
    data[idx+2] = '0';
    data[idx+3] = '2';
    return 4;
  } else if (code == 'E') {
    data[idx] = '0';
    data[idx+1] = '2';
    return 2;
  } else if (code == 'F') {
    data[idx] = '0';
    data[idx+1] = '0';
    data[idx+2] = '1';
    data[idx+3] = '0';
    data[idx+4] = '2';
    return 5;
  } else if (code == 'G') {
    data[idx] = '1';
    data[idx+1] = '1';
    data[idx+2] = '0';
    data[idx+3] = '2';
    return 4;
  } else if (code == 'H') {
    data[idx] = '0';
    data[idx+1] = '0';
    data[idx+2] = '0';
    data[idx+3] = '0';
    data[idx+4] = '2';
    return 5;
  } else if (code == 'I') {
    data[idx] = '0';
    data[idx+1] = '0';
    data[idx+2] = '2';
    return 3;
  } else if (code == 'J') {
    data[idx] = '0';
    data[idx+1] = '1';
    data[idx+2] = '1';
    data[idx+3] = '1';
    data[idx+4] = '2';
    return 5;
  } else if (code == 'K') {
    data[idx] = '1';
    data[idx+1] = '0';
    data[idx+2] = '1';
    data[idx+3] = '2';
    return 4;
  } else if (code == 'L') {
    data[idx] = '0';
    data[idx+1] = '1';
    data[idx+2] = '0';
    data[idx+3] = '0';
    data[idx+4] = '2';
    return 5;
  } else if (code == 'M') {
    data[idx] = '1';
    data[idx+1] = '1';
    data[idx+2] = '2';
    return 3;
  } else if (code == 'N') {
    data[idx] = '1';
    data[idx+1] = '0';
    data[idx+2] = '2';
    return 3;
  } else if (code == 'O') {
    data[idx] = '1';
    data[idx+1] = '1';
    data[idx+2] = '1';
    data[idx+3] = '2';
    return 4;
  } else if (code == 'P') {
    data[idx] = '0';
    data[idx+1] = '1';
    data[idx+2] = '1';
    data[idx+3] = '0';
    data[idx+4] = '2';
    return 5;
  } else if (code == 'Q') {
    data[idx] = '1';
    data[idx+1] = '1';
    data[idx+2] = '0';
    data[idx+3] = '1';
    data[idx+4] = '2';
    return 5;
  } else if (code == 'R') {
    data[idx] = '1';
    data[idx+1] = '0';
    data[idx+2] = '1';
    data[idx+3] = '2';
    return 4;
  } else if (code == 'S') {
    data[idx] = '0';
    data[idx+1] = '0';
    data[idx+2] = '0';
    data[idx+3] = '2';
    return 4;
  } else if (code == 'T') {
    data[idx] = '1';
    data[idx+1] = '2';
    return 2;
  } else if (code == 'U') {
    data[idx] = '0';
    data[idx+1] = '0';
    data[idx+2] = '1';
    data[idx+3] = '2';
    return 4;
  } else if (code == 'V') {
    data[idx] = '0';
    data[idx+1] = '0';
    data[idx+2] = '0';
    data[idx+3] = '1';
    data[idx+4] = '2';
    return 5;
  } else if (code == 'W') {
    data[idx] = '0';
    data[idx+1] = '1';
    data[idx+2] = '1';
    data[idx+3] = '2';
    return 4;
  } else if (code == 'X') {
    data[idx] = '1';
    data[idx+1] = '0';
    data[idx+2] = '0';
    data[idx+3] = '1';
    data[idx+4] = '2';
    return 5;
  } else if (code == 'Y') {
    data[idx] = '1';
    data[idx+1] = '0';
    data[idx+2] = '1';
    data[idx+3] = '1';
    data[idx+4] = '2';
    return 5;
  } else if (code == 'Z') {
    data[idx] = '1';
    data[idx+1] = '1';
    data[idx+2] = '0';
    data[idx+3] = '0';
    data[idx+4] = '2';
    return 5;
  }
  data[idx] = '3';
  return 1;
}

static struct codeData* convertAlphaToCode(char* chars, int numChars){
  for(int i = 0; i<numChars; i++){
    chars[i] = toupper((unsigned char)chars[i]);
  }
  
  int numCodes = 0;
  for (int i = 0; i < numChars; i++) {
    if (matchesOne(chars[i],"ET", 2)) {
      numCodes+=2;
    } else if (matchesOne(chars[i],"AINM", 4)) {
      numCodes+=3;
    } else if (matchesOne(chars[i],"DGKORSUW", 8)) {
      numCodes+=4;
    } else {
      numCodes+=5;
    }
  }
  
  struct codeData *data = malloc(sizeof(struct codeData));
  data->code = malloc(numCodes+1);
  
  int index = 0;
  for (int i = 0; i < numChars; i++){
    index += addCodeData(data->code, chars[i], index);
  }
  data->index = 0;
  data->codelen = numCodes;
  return data;
}

// Called when a message is received from PebbleKitJS
static void in_received_handler(DictionaryIterator *received, void *context) {
	Tuple *tuple;
	
	tuple = dict_find(received, STATUS_KEY);
	if(tuple) {
		APP_LOG(APP_LOG_LEVEL_DEBUG, "Received Status: %d", (int)tuple->value->uint32); 
	}
	
	tuple = dict_find(received, MESSAGE_KEY);
	if(tuple) {
		APP_LOG(APP_LOG_LEVEL_DEBUG, "Received Message: %s", tuple->value->cstring);
	}
  
  int strlen = -1;
	tuple = dict_find(received, STRLEN_KEY);
	if(tuple) {
    strlen = (int)tuple->value->uint32;
		APP_LOG(APP_LOG_LEVEL_DEBUG, "Received strlen: %d", (int)tuple->value->uint32);
	}
  
  tuple = dict_find(received, STR_KEY);
	if(tuple) {
    if (strlen != -1) {
      for (int i = 0; i < strlen; i++) {
        //tuple->value->cstring[i]
      }
      currPhrase = tuple->value->cstring;
      if (!showingWatchFace)
        main_window_update(window, tuple->value->cstring);
		  APP_LOG(APP_LOG_LEVEL_DEBUG, "Received str: %s", tuple->value->cstring);
    } else {
      if (!showingWatchFace)
        main_window_update(window, "");
    }
    
    struct codeData *data = convertAlphaToCode(tuple->value->cstring, strlen);
		APP_LOG(APP_LOG_LEVEL_DEBUG, "data: %s, %d, %d", data->code, data->index, data->codelen);
    process_code(data);
		APP_LOG(APP_LOG_LEVEL_DEBUG, "datadon: %s, %d, %d", data->code, data->index, data->codelen);
	}
}

// Called when an incoming message from PebbleKitJS is dropped
static void in_dropped_handler(AppMessageResult reason, void *context) {	
}

// Called when PebbleKitJS does not acknowledge receipt of a message
static void out_failed_handler(DictionaryIterator *failed, AppMessageResult reason, void *context) {
}

void init(void) {
	window = window_create();
  // Set handlers to manage the elements inside the Window
  window_set_window_handlers(window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload
  });
	window_stack_push(window, true);
	
	// Register AppMessage handlers
	app_message_register_inbox_received(in_received_handler); 
	app_message_register_inbox_dropped(in_dropped_handler); 
	app_message_register_outbox_failed(out_failed_handler);
		
	app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());
	
	send_message();
}

void deinit(void) {
	app_message_deregister_callbacks();
	window_destroy(window);
}

int main( void ) {
	init();
	app_event_loop();
	deinit();
}