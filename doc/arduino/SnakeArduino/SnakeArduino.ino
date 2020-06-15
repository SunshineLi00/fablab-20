#include <Adafruit_NeoPixel.h>
#include <LibPrintf.h>

#define PIN         4
#define NUMPIXELS   128

Adafruit_NeoPixel pixels(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);

void setup() {
  pixels.begin();
  pixels.clear();
  init_snake();

  Serial.begin(9600);
}

int screen_xy_to_1d(int row, int col) {
  if (col<8)
    return (row%2)?(8*row+col):(8*row+7-col); 
  else
    return (row%2==0)?(64+8*(7-row)+(15-col)):(64+8*(7-row)+7-(15-col)); 
}

unsigned char snake[128];
unsigned char food;
int snake_head, snake_length;
int died=0;

enum SNAKE_INPUT { Noin=0,Left,Right,Up,Down};
enum SNAKE_INPUT s_last;
enum SNAKE_INPUT s_in;

unsigned long last_advect = millis();
unsigned long last_death=0;

inline void packed_xy_from_pos(unsigned char pos, int *row, int *col) {
  *row = pos / 16; *col = pos - *row * 16;
}

inline unsigned char packed_pos_from_xy(int row, int col) {
  return row * 16 + col; 
}

#define SET_PIXEL(row, col, r, g, b)    pixels.setPixelColor(screen_xy_to_1d(row, col), pixels.Color(r, g, b));
#define SET_FOOD(row, col)          SET_PIXEL(row, col, 25, 0, 0)

void init_snake() {
  snake[0] = packed_pos_from_xy(4, 6);
  snake[1] = packed_pos_from_xy(4, 7);
  snake[2] = packed_pos_from_xy(4, 8);
  food = packed_pos_from_xy(4, 4);
  snake_head = 0;
  snake_length = 3;

  died = 0;
  s_last = Left;
  s_in = Noin;
}

unsigned char& snake_pos(int idx) {
  while (idx<0) idx+=128;
  return snake[idx % 128];
}

int put_input()
{
  int flag=0;
   while(Serial.available()>0)
   {
    char testchar=Serial.read();
    if(testchar=='w' && s_last != Down)
      {s_in=Up;flag=1;}
    else if(testchar=='a' && s_last != Right)
      {s_in=Left;flag=1;}
    else if(testchar=='s' && s_last != Up)
      {s_in=Down;flag=1;}
    else if(testchar=='d' && s_last != Left)
      {s_in=Right;flag=1;}
   }
   delay(1);
   int value=0;
   value=analogRead(A0);
   if(value<200 && s_last != Right) {s_in=Left;flag=1;}
   if(value>1000 && s_last != Left) {s_in=Right;flag=1;}
   value=analogRead(A1);
   if(value<200 && s_last != Down) {s_in=Up;flag=1;}
   if(value>1000 && s_last != Up) {s_in=Down;flag=1;}
   return flag;
}

int food_phase = 0;
#define food_phase_max 8

void draw_screen() {
  pixels.clear();

  int row, col;
  packed_xy_from_pos(food, &row, &col);
  float food_g;
  food_g = 15 + 5 * sinf((float)food_phase++ / food_phase_max * 6.28);
  SET_PIXEL(row, col, 2, food_g, 2);

  for (int i=0;i<snake_length;i++) {
    packed_xy_from_pos(snake_pos(snake_head+i), &row, &col);
    float delta = (float)i/(float)(snake_length-1);
    if(died)
    {
      delta =1- sqrtf(delta); 
      float r = 18.0 * delta + 12.0 * (1-delta);
      float g = 0 * delta + 12.0 * (1-delta);
      float b = 0 * delta + 12.0 * (1-delta);
      SET_PIXEL(row, col, r, g, b);
    } else if(s_in==Noin)
    {
      float r = 0.5 * delta + 12.0 * (1-delta);
      float g = 0.5 * delta + 12.0 * (1-delta);
      float b = 0.5 * delta + 12.0 * (1-delta);
      if (r < 1) {
        r=1;g=1;b=1;
      }
      SET_PIXEL(row, col, r, g, b);
    }
    else{
      delta = 1-sqrtf(delta);
      float r = 0 * delta + 12.0 * (1-delta);
      float g = 0 * delta + 12.0 * (1-delta);
      float b = 25.0 * delta + 12.0 * (1-delta);
      SET_PIXEL(row, col, r, g, b);
    }
    
  }
  
  pixels.show();
}

void snake_die() {
  died = 1;
  last_death = millis();
}
void try_eat(int row, int col)
{
    int food_row,food_col;
    packed_xy_from_pos(food,&food_row,&food_col);
    if(food_row==row && food_col==col)
    {
     snake_length++;  
     random_food();
    }
}

int whether_on_snake(int obj_row, int obj_col, int include_tail) {
  int flag = 1;
  int row, col;
  int i=0;
  while(flag && i<snake_length)
  {
    if (i==snake_length-1 && include_tail==0) break;
    packed_xy_from_pos(snake_pos(snake_head+i), &row, &col);
    if(row==obj_row && col==obj_col) flag=0;
    i++;
  }
  return 1-flag;
}

void random_food()
{
  int food_row,food_col;
  while(1){
    food_row=random(0,7);
    food_col=random(0,15);
    if(whether_on_snake(food_row,food_col,1)==0) break;
  }
  food=packed_pos_from_xy(food_row,food_col);
}

#define ADVECT_DIR(change_pos, out_of_border) \
  change_pos; \
  try_eat(row,col); \
  if (whether_on_snake(row, col, 0)) { \
    snake_die(); \
  } else if (out_of_border) { \
    snake_pos(snake_head-1) = packed_pos_from_xy(row, col); \
    snake_head--; \
  } else { \
    snake_die(); \
  } \


void advect_snake() {
  int row, col;
  packed_xy_from_pos(snake_pos(snake_head), &row, &col);
  printf("Advection: head is idx%d.its length=%d,at row %d col %d.\n",snake_head,snake_length,row,col);

  if (s_in == Noin) s_in = s_last;

  if(s_in==Left)
  {
    ADVECT_DIR(col--, col>=0);
  }
  else if(s_in==Right)
  {
    ADVECT_DIR(col++, col<=15);
  }
  else if(s_in==Up)
  {
    ADVECT_DIR(row--, row>=0);
  }
  else{
    ADVECT_DIR(row++, row<=7);
  }

  s_last = s_in;
  s_in = Noin;
  draw_screen();
}

void loop() {

  if(died) {
    if(millis()-last_death>=6000)
    {
      init_snake();
      last_advect = millis();
      draw_screen();
    } else {
      while(Serial.available()>0) Serial.read();
      draw_screen();
      delay(10);
    }
  } else if (millis() - last_advect > 400) {
    advect_snake();
    last_advect = millis();
  } else{
    put_input();
    draw_screen();
    delay(10);
  }
}
