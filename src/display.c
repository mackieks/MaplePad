/* display.c
 *  drawing functions for all OLEDs
 */

#include "display.h"
#include "ssd1331.h"
#include "ssd1306.h"

#define TRUE 1
#define FALSE 0

// Math constants we'll use
double const pi=3.1415926535897932384626433;	// pi
double const twopi=2.0*pi;			// pi times 2
double const two_over_pi= 2.0/pi;		// 2/pi
double const halfpi=pi/2.0;			// pi divided by 2
double const threehalfpi=3.0*pi/2.0;  		// pi times 3/2, used in tan routines
double const four_over_pi=4.0/pi;		// 4/pi, used in tan routines
double const qtrpi=pi/4.0;			// pi/4.0, used in tan routines
double const sixthpi=pi/6.0;			// pi/6.0, used in atan routines
double const twelfthpi=pi/12.0;			// pi/12.0, used in atan routines


float cos_32s(float x)
{
const float c1= 0.99940307;
const float c2=-0.49558072;
const float c3= 0.03679168;

float x2;							// The input argument squared

x2=x * x;
return (c1 + x2*(c2 + c3 * x2));
}

//
//  This is the main cosine approximation "driver"
// It reduces the input argument's range to [0, pi/2],
// and then calls the approximator. 
// See the notes for an explanation of the range reduction.
//
float cos32(float x){
  int q;						// what quadrant are we in?

	x=fmod(x, twopi);				// Get rid of values > 2* pi
	if(x<0)x=-x;					// cos(-x) = cos(x)
	q=(x * two_over_pi);	
	switch (q){
	case 0: return  cos_32s(x);
	case 1: return -cos_32s(pi-x);
	case 2: return -cos_32s(x-pi);
	case 3: return  cos_32s(twopi-x);
	}
}
//
//   The sine is just cosine shifted a half-pi, so
// we'll adjust the argument and call the cosine approximation.
//
float sin32(float x){
	return cos32(halfpi-x);
}

double atan_66s(double x)
{
const double c1=1.6867629106;
const double c2=0.4378497304;
const double c3=1.6867633134;


double x2;							// The input argument squared

x2=x * x;
return (x*(c1 + x2*c2)/(c3 + x2));
}

double atan66(double x){
double tansixthpi=tan(sixthpi);		// tan(pi/6), used in atan routines
double tantwelfthpi=tan(twelfthpi);	// tan(pi/12), used in atan routines
double y;							// return from atan__s function
int complement= FALSE;				// true if arg was >1 
int region= FALSE;					// true depending on region arg is in
int sign= FALSE;					// true if arg was < 0

if (x <0 ){
	x=-x;
	sign=TRUE;						// arctan(-x)=-arctan(x)
}
if (x > 1.0){
	x=1.0/x;						// keep arg between 0 and 1
	complement=TRUE;
}
if (x > tantwelfthpi){
	x = (x-tansixthpi)/(1+tansixthpi*x);	// reduce arg to under tan(pi/12)
	region=TRUE;
}

y=atan_66s(x);						// run the approximation
if (region) y+=sixthpi;				// correct for region we're in
if (complement)y=halfpi-y;			// correct for 1/x if we did that
if (sign)y=-y;						// correct for negative arg
return (y);

}


#define HSV_HUE_SEXTANT		256
#define HSV_HUE_STEPS		(6 * HSV_HUE_SEXTANT)

#define HSV_HUE_MIN		0
#define HSV_HUE_MAX		(HSV_HUE_STEPS - 1)

#define HSV_SWAPPTR(a,b)	do { uint8_t *tmp = (a); (a) = (b); (b) = tmp; } while(0)
#define HSV_POINTER_SWAP(sextant,r,g,b) \
	do { \
		if((sextant) & 2) { \
			HSV_SWAPPTR((r), (b)); \
		} \
		if((sextant) & 4) { \
			HSV_SWAPPTR((g), (b)); \
		} \
		if(!((sextant) & 6)) { \
			if(!((sextant) & 1)) { \
				HSV_SWAPPTR((r), (g)); \
			} \
		} else { \
			if((sextant) & 1) { \
				HSV_SWAPPTR((r), (g)); \
			} \
		} \
	} while(0)

#define HSV_SEXTANT_TEST(sextant) \
	do { \
		if((sextant) > 5) { \
			(sextant) = 5; \
		} \
	} while(0)

void fast_hsv2rgb_32bit(uint16_t h, uint8_t s, uint8_t v, uint8_t *r, uint8_t *g , uint8_t *b)
{

	uint8_t sextant = h >> 8;

	HSV_SEXTANT_TEST(sextant);	

	HSV_POINTER_SWAP(sextant, r, g, b);	// Swap pointers depending which sextant we are in 

	*g = v;		// Top level

	// Perform actual calculations

	/*
	 * Bottom level: v * (1.0 - s)
	 * --> (v * (255 - s) + error_corr + 1) / 256
	 */
	uint16_t ww;		// Intermediate result
	ww = v * (255 - s);	// We don't use ~s to prevent size-promotion side effects
	ww += 1;		// Error correction
	ww += ww >> 8;		// Error correction
	*b = ww >> 8;

	uint8_t h_fraction = h & 0xff;	// 0...255
	uint32_t d;			// Intermediate result

	if(!(sextant & 1)) {
		// *r = ...slope_up...;
		d = v * (uint32_t)((255 << 8) - (uint16_t)(s * (256 - h_fraction)));
		d += d >> 8;	// Error correction
		d += v;		// Error correction
		*r = d >> 16;
	} else {
		// *r = ...slope_down...;
		d = v * (uint32_t)((255 << 8) - (uint16_t)(s * h_fraction));
		d += d >> 8;	// Error correction
		d += v;		// Error correction
		*r = d >> 16;
	}
}

void setPixel(uint8_t x, uint8_t y, uint16_t color)
{
  if (oledType) // 1
    setPixelSSD1331(x, y, color);
  else // 0
    setPixelSSD1306(x + 16, y, color ? 1 : 0);
}

// bool getPixel(uint8_t x, uint8_t y)
// {
//   if (oledType) // 1
//     return(getPixelSSD1331(x, y));
//   else // 0
//     return(getPixelSSD1306(x + 16, y));
// }


void drawEllipse(uint8_t xc, uint8_t yc, uint8_t xr, uint8_t yr, int angle, uint16_t color, bool fill){
  int step = 120;
  float theta = 0;
  float sangle, cangle = 0;
  float rangle = 0;
  float kf = 0;
  float x, y = 0;
  int xrot, yrot = 0;
  int exmax, eymax = 0;
  int exmin, eymin = 128;

  uint8_t ellipse_start, ellipse_end = 0;

  rangle = angle * M_PI / 180.0;
  kf = (360 * M_PI / 180.0) / step;
  sangle = sin32(rangle);
  cangle = cos32(rangle);

  for(int i = 0; i < step; i++){
    theta = i * kf;
    x = xc + xr * cos32(theta);
    y = yc + yr * sin32(theta);
    xrot = round(xc + (x - xc) * cangle - (y - yc) * sangle);
    yrot = round(yc + (x - xc) * sangle + (y - yc) * cangle);
    setPixel(xrot, yrot, color);

    // grab ellipse bounding box
    if(xrot < exmin) exmin = xrot;
    if(xrot > exmax) exmax = xrot;
    if(yrot < eymin) eymin = yrot;
    if(yrot > eymax) eymax = yrot;

  }

  // ...there must be a faster way to do this 😖
  // if(fill){
  //   for(int j = eymin; j <= eymax; j++){

  //     int i = 0;
  //     for(i = exmin; !getPixel(i,j); i++){}
  //     ellipse_start = i;

  //     for(i = exmax; !getPixel(i,j); i--){}
  //     ellipse_end = i;

  //     for(i = ellipse_start; i <= ellipse_end; i++){
  //       setPixel(i, j, color);
  //     }
  //   }

  // }
};

void drawLine(int x0, int y0, int w, uint16_t color){
  hagl_draw_line(x0, y0, x0 + w, y0, color);
}

void hagl_draw_line(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color)
{

    int16_t dx;
    int16_t sx;
    int16_t dy;
    int16_t sy;
    int16_t err;
    int16_t e2;

    dx = (x1 - x0);
    sx = x0 < x1 ? 1 : -1;
    dy = (y1 - y0);
    sy = y0 < y1 ? 1 : -1;
    err = (dx > dy ? dx : -dy) / 2;

    while (1) {
        setPixel(x0, y0, color);

        if (x0 == x1 && y0 == y1) {
            break;
        };

        e2 = err + err;

        if (e2 > -dx) {
            err -= dy;
            x0 += sx;
        }

        if (e2 < dy) {
            err += dx;
            y0 += sy;
        }
    }
}

void fillRect(int x0, int x1, int y0, int y1, uint16_t color){
  for(int i = y0; i <= y1; i++){
    hagl_draw_line(x0, i, x1, i, color);
  }
}

void fillCircle(int x0, int y0, int r, uint16_t color){
int f = 1 - r;
  int ddF_x = 1;
  int ddF_y = -2 * r;
  int x = 0;
  int y = r;
  
  fillRect(x0, x0 ,y0 - r,y0 + r, color); 
  fillRect(x0 - r, x0 + r ,y0,y0, color);  
  
  while(x < y)
  {
    if(f >= 0) 
    {
      y--;
      ddF_y += 2;
      f += ddF_y;
    }
    x++;
    ddF_x += 2;
    f += ddF_x;    

    fillRect(x0-x, x0+x ,y0 +y, y0 + y, color);    
    fillRect(x0-x, x0+x ,y0 - y, y0 - y, color); 
    fillRect(x0-y, x0+y ,y0 + x, y0 + x, color);         
    fillRect(x0-y, x0+y ,y0 - x, y0 - x, color); 
  }
}

void drawCursor(int iy, uint16_t color){
  for(int i = 87; i <= 95; i++){
    for(int j = 0; j <= 63; j++){
      setPixel(i,j,0x00);
    }
  }
  for(int i = 93; i >= 90; i--){
    for(int j = 57; j >= 53; j--){
      if(i > 90)
        setPixel(i, j - (12 * iy), color);
      else if ((j > 53) && (j < 57))
        setPixel(i, j - (12 * iy), color);
    }
  }
  setPixel(89, 55 - (12 * iy), color);
}

void drawToggle(int iy, uint16_t color, bool on){
  if(on){
    for(int i = 20; i >= 3; i--){
      for(int j = 58; j >= 52; j--){
        if( !(( i >= 4 && i <= 8) && (j >= 53 && j <= 57)) )
          setPixel(i, j - (12 * iy), color);
      }
    }
    setPixel(20, 58 - (12 * iy), 0x0000);
    setPixel(20, 57 - (12 * iy), 0x0000);
    setPixel(20, 53 - (12 * iy), 0x0000);
    setPixel(20, 52 - (12 * iy), 0x0000);
    setPixel(19, 58 - (12 * iy), 0x0000);
    setPixel(19, 52 - (12 * iy), 0x0000);

    setPixel(3, 58 - (12 * iy), 0x0000);
    setPixel(3, 57 - (12 * iy), 0x0000);
    setPixel(3, 53 - (12 * iy), 0x0000);
    setPixel(3, 52 - (12 * iy), 0x0000);
    setPixel(4, 58 - (12 * iy), 0x0000);
    setPixel(4, 52 - (12 * iy), 0x0000);

    setPixel(4, 57 - (12 * iy), color);
    setPixel(8, 57 - (12 * iy), color);
    setPixel(8, 53 - (12 * iy), color);
    setPixel(4, 53 - (12 * iy), color);
  } else {
    for(int i = 18; i >= 5; i--){
      setPixel(i, 58 - (12 * iy), color);
      setPixel(i, 52 - (12 * iy), color);
    }
    for(int j = 56; j >= 54; j--){
      setPixel(3, j - (12 * iy), color);
      setPixel(14, j - (12 * iy), color);
      setPixel(20, j - (12 * iy), color);
    }
    setPixel(4, 57 - (12 * iy), color);
    setPixel(4, 53 - (12 * iy), color);
    setPixel(15, 57 - (12 * iy), color);
    setPixel(15, 53 - (12 * iy), color);
    setPixel(19, 57 - (12 * iy), color);
    setPixel(19, 53 - (12 * iy), color);
  }
}

void putLetter(int ix, int iy, int index, uint16_t color){
  tFont x = Font; 
  tChar y = Font.chars[index];  // select character from 0 - 54
  tImage z = *y.image;  // data array from tImage "Font_0x79"
  uint8_t* a = z.data; // row of character data

  for(int i = 0; i <= 9; i++){
    for(int j = 2; j <= 7; j++){  // iterate through bits in row of character
      if( !((1 << j) & a[i]) )
        setPixel((88-(ix*6))-j,(59-(iy*10)-(iy*2))-i,color);
      else
        setPixel((88-(ix*6))-j,(59-(iy*10)-(iy*2))-i,0x0000);
    }
  }
}

void putString(char* text, int ix, int iy, uint16_t color){
  for(int i = 0; text[i] != '\0'; i++){
    switch(text[i]){
      case ' ':
        putLetter(ix + i, iy, 0, color);
        break;

      case '-':
        putLetter(ix + i, iy, 1, color);
        break;

      case '.':
        putLetter(ix + i, iy, 2, color);
        break;

      case 'A':
        putLetter(ix + i, iy, 3, color);
        break;

      case 'B':
        putLetter(ix + i, iy, 4, color);
        break;

      case 'C':
        putLetter(ix + i, iy, 5, color);
        break;

      case 'D':
        putLetter(ix + i, iy, 6, color);
        break;

      case 'E':
        putLetter(ix + i, iy, 7, color);
        break;

      case 'F':
        putLetter(ix + i, iy, 8, color);
        break;

      case 'G':
        putLetter(ix + i, iy, 9, color);
        break;

      case 'H':
        putLetter(ix + i, iy, 10, color);
        break;

      case 'I':
        putLetter(ix + i, iy, 11, color);
        break;

      case 'J':
        putLetter(ix + i, iy, 12, color);
        break;

      case 'K':
        putLetter(ix + i, iy, 13, color);
        break;

      case 'L':
        putLetter(ix + i, iy, 14, color);
        break;

      case 'M':
        putLetter(ix + i, iy, 15, color);
        break;

      case 'N':
        putLetter(ix + i, iy, 16, color);
        break;

      case 'O':
        putLetter(ix + i, iy, 17, color);
        break;

      case 'P':
        putLetter(ix + i, iy, 18, color);
        break;

      case 'Q':
        putLetter(ix + i, iy, 19, color);
        break;

      case 'R':
        putLetter(ix + i, iy, 20, color);
        break;

      case 'S':
        putLetter(ix + i, iy, 21, color);
        break;

      case 'T':
        putLetter(ix + i, iy, 22, color);
        break;

      case 'U':
        putLetter(ix + i, iy, 23, color);
        break;

      case 'V':
        putLetter(ix + i, iy, 24, color);
        break;

      case 'W':
        putLetter(ix + i, iy, 25, color);
        break;

      case 'X':
        putLetter(ix + i, iy, 26, color);
        break;

      case 'Y':
        putLetter(ix + i, iy, 27, color);
        break;

      case 'Z':
        putLetter(ix + i, iy, 28, color);
        break;

      case 'a':
        putLetter(ix + i, iy, 29, color);
        break;

      case 'b':
        putLetter(ix + i, iy, 30, color);
        break;

      case 'c':
        putLetter(ix + i, iy, 31, color);
        break;

      case 'd':
        putLetter(ix + i, iy, 32, color);
        break;

      case 'e':
        putLetter(ix + i, iy, 33, color);
        break;

      case 'f':
        putLetter(ix + i, iy, 34, color);
        break;

      case 'g':
        putLetter(ix + i, iy, 35, color);
        break;

      case 'h':
        putLetter(ix + i, iy, 36, color);
        break;

      case 'i':
        putLetter(ix + i, iy, 37, color);
        break;

      case 'j':
        putLetter(ix + i, iy, 38, color);
        break;

      case 'k':
        putLetter(ix + i, iy, 39, color);
        break;

      case 'l':
        putLetter(ix + i, iy, 40, color);
        break;

      case 'm':
        putLetter(ix + i, iy, 41, color);
        break;

      case 'n':
        putLetter(ix + i, iy, 42, color);
        break;

      case 'o':
        putLetter(ix + i, iy, 43, color);
        break;

      case 'p':
        putLetter(ix + i, iy, 44, color);
        break;

      case 'q':
        putLetter(ix + i, iy, 45, color);
        break;

      case 'r':
        putLetter(ix + i, iy, 46, color);
        break;

      case 's':
        putLetter(ix + i, iy, 47, color);
        break;

      case 't':
        putLetter(ix + i, iy, 48, color);
        break;

      case 'u':
        putLetter(ix + i, iy, 49, color);
        break;

      case 'v':
        putLetter(ix + i, iy, 50, color);
        break;

      case 'w':
        putLetter(ix + i, iy, 51, color);
        break;

      case 'x':
        putLetter(ix + i, iy, 52, color);
        break;

      case 'y':
        putLetter(ix + i, iy, 53, color);
        break;

      case 'z':
        putLetter(ix + i, iy, 54, color);
        break;

      case '!':
        putLetter(ix + i, iy, 55, color);
        break;

      case '#':
        putLetter(ix + i, iy, 56, color);
        break;

      case '%':
        putLetter(ix + i, iy, 57, color);
        break;

      case '&':
        putLetter(ix + i, iy, 58, color);
        break;

      case '\'':
        putLetter(ix + i, iy, 59, color);
        break;
      
      case '(':
        putLetter(ix + i, iy, 60, color);
        break;
      
      case ')':
        putLetter(ix + i, iy, 61, color);
        break;
      
      case '*':
        putLetter(ix + i, iy, 62, color);
        break;

      case '+':
        putLetter(ix + i, iy, 63, color);
        break;

      case ',':
        putLetter(ix + i, iy, 64, color);
        break;

      case '0':
        putLetter(ix + i, iy, 65, color);
        break;

      case '1':
        putLetter(ix + i, iy, 66, color);
        break;

      case '2':
        putLetter(ix + i, iy, 67, color);
        break;

      case '3':
        putLetter(ix + i, iy, 68, color);
        break;

      case '4':
        putLetter(ix + i, iy, 69, color);
        break;
      
      case '5':
        putLetter(ix + i, iy, 70, color);
        break;

      case '6':
        putLetter(ix + i, iy, 71, color);
        break;

      case '7':
        putLetter(ix + i, iy, 72, color);
        break;

      case '8':
        putLetter(ix + i, iy, 73, color);
        break;

      case '9':
        putLetter(ix + i, iy, 74, color);
        break;

      case ':':
        putLetter(ix + i, iy, 75, color);
        break;

      case ';':
        putLetter(ix + i, iy, 76, color);
        break;

      case '=':
        putLetter(ix + i, iy, 77, color);
        break;

      default:
        break;
    }  
  }
}

void displayInit()
{
  if (oledType) // 1
    ssd1331_init();
  else // 0
    ssd1306_init();
}

void updateDisplay()
{
  if (oledType) // 1
    updateSSD1331();
  else // 0
    updateSSD1306();
}

void clearDisplay()
{
  if (oledType) // 1
    clearSSD1331();
  else // 0
    clearSSD1306();
}