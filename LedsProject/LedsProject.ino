#include<FastLED.h>

#define LED_PIN     23
#define LED_SKIP     4
#define LED_TYPE    WS2811
#define COLOR_ORDER RGB

#define kMatrixWidth 50
#define kMatrixHeight 4
#define NUM_LEDS (kMatrixWidth * kMatrixHeight)

// The leds
CRGB leds[kMatrixWidth * kMatrixHeight];

// The 16 bit version of our coordinates
static uint16_t x;
static uint16_t y;
static uint16_t z;

uint16_t speed = 10; // speed is set dynamically once we've started up
uint16_t scale = 800; // scale is set dynamically once we've started up
uint8_t  noise[kMatrixWidth][kMatrixHeight];
uint8_t  colorLoop = 1;

int r,g,b;
int inbyte;
bool fixed;


uint8_t from[kMatrixWidth][kMatrixHeight];
uint8_t to[kMatrixWidth][kMatrixHeight];
uint8_t totlen[kMatrixWidth][kMatrixHeight];
uint8_t done[kMatrixWidth][kMatrixHeight];

// Fill the x/y array of 8-bit noise values using the inoise8 function.
void fillnoiselin() {
  for(int i = 0; i < kMatrixWidth; i++) {
    int ioffset = scale * i;
    for(int j = 0; j < kMatrixHeight; j++) {
      done[i][j] += 1;
      uint16_t pre = from[i][j]*(totlen[i][j]-done[i][j]);
      uint16_t post = to[i][j]*done[i][j];
      noise[i][j] = ((pre+post)/2)/totlen[i][j];
      if (done[i][j] > totlen[i][j]) {
        done[i][j] = 0;
        totlen[i][j] = random(128,255);
        from[i][j] = to[i][j];
        to[i][j] = random(255);
      }
    }
  }
}

// Fill the x/y array of 8-bit noise values using the inoise8 function.
void fillnoise8() {
  for(int i = 0; i < kMatrixWidth; i++) {
    int ioffset = scale * i;
    for(int j = 0; j < kMatrixHeight; j++) {
      int joffset = scale * j;
      uint8_t data = inoise8(x + ioffset,y + joffset);//,z);

      // The range of the inoise8 function is roughly 16-238.
      // These two operations expand those values out to roughly 0..255
      // You can comment them out if you want the raw noise data.
      data = qsub8(data,16);
      data = qadd8(data,scale8(data,39));
      noise[i][j] = data;
    }
  }
  
  z += speed;
  
  // apply slow drift to X and Y, just for visual variation.
  x += speed / 8;
  y -= speed / 16;
}

void mapNoiseToLEDsUsingPalette()
{
  static uint8_t ihue=0;
  
  for(int i = 0; i < kMatrixWidth; i++) {
    for(int j = 0; j < kMatrixHeight; j++) {
      uint8_t index = noise[j][i];
      // if this palette is a 'loop', add a slowly-changing base value
      if( colorLoop) { 
        index += ihue;
      }
      CRGB color = CHSV(index,255,255);//ColorFromPalette( currentPalette, index, bri);
      leds[XY(i,j)] = color;
    }
  }
  ihue+=1;
}

bool doSerial() {
    if (Serial.available() > 0) {
      inbyte = Serial.read();
      if (inbyte == 'u') {
        Serial.println("Unsetting");
        fixed = false;
      } else if (inbyte == 'b') {
        Serial.println("Please specify B");
        LEDS.setBrightness(Serial.parseInt());
        FastSPI_LED.show();
      } else if (inbyte == 's') {
        Serial.println("Please specify RGB");
        r = Serial.parseInt();
        g = Serial.parseInt();
        b = Serial.parseInt();
        Serial.println("Done");
        fixed = true;
        for (int i = 0 ; i < NUM_LEDS; i++ ) {
          leds[i].setRGB(r,g,b);
        }
        FastSPI_LED.show();
      } else if (inbyte == 'p') {
        Serial.println("RGB is");
        Serial.print(' ');
        Serial.print(r);
        Serial.print(' ');
        Serial.print(g);
        Serial.print(' ');
        Serial.print(b);
      }
      return true;
    } 
    return false;
}

void setup() {
  Serial.begin(9600);
  delay(3000);
  LEDS.addLeds<LED_TYPE,LED_PIN+(0*LED_SKIP),COLOR_ORDER>(leds,50);
  LEDS.addLeds<LED_TYPE,LED_PIN+(1*LED_SKIP),COLOR_ORDER>(leds,50);
  LEDS.addLeds<LED_TYPE,LED_PIN+(2*LED_SKIP),COLOR_ORDER>(leds,50);
  LEDS.addLeds<LED_TYPE,LED_PIN+(3*LED_SKIP),COLOR_ORDER>(leds,50);
  LEDS.setBrightness(0);

  // Initialize our coordinates to some random values
  x = random16();
  y = random16();
  z = random16();
  
  fillnoise8();
  mapNoiseToLEDsUsingPalette();
  for (int i=0; i<=255; i+=(1+i*0.02)) {
    LEDS.setBrightness(i);
    delay(20);
    LEDS.show();
  }
  LEDS.setBrightness(255);
}

void loop() {
  doSerial();
  if (!fixed) {
    //fillnoiselin();//fillnoise8();
    fillnoise8();
    mapNoiseToLEDsUsingPalette();
  }
  LEDS.show();
}

uint16_t XY( uint8_t x, uint8_t y)
{
  return (y * kMatrixWidth) + x;
}

