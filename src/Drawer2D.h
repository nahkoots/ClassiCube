#ifndef CC_DRAWER2D_H
#define CC_DRAWER2D_H
#include "PackedCol.h"
#include "Constants.h"
#include "Bitmap.h"
/*  Performs a variety of drawing operations on bitmaps, and converts bitmaps into textures.
	Copyright 2014-2019 ClassiCube | Licensed under BSD-3
*/

struct DrawTextArgs { String text; FontDesc font; bool useShadow; };
struct Texture;
struct IGameComponent;
extern struct IGameComponent Drawer2D_Component;

void DrawTextArgs_Make(struct DrawTextArgs* args, STRING_REF const String* text, const FontDesc* font, bool useShadow);
void DrawTextArgs_MakeEmpty(struct DrawTextArgs* args, const FontDesc* font, bool useShadow);
/* Initialises the given font. When Drawer2D_BitmappedText is false, creates native font handle using Font_Make. */
CC_NOINLINE void Drawer2D_MakeFont(FontDesc* desc, int size, int style);

/* Whether text should be drawn and measured using the currently set font bitmap. */ 
/* If false, then text is instead draw using platform/system fonts. */
extern bool Drawer2D_BitmappedText;
/* Whether the shadows behind text (that uses shadows) is fully black. */
extern bool Drawer2D_BlackTextShadows;
/* List of all colours. (An A of 0 means the colour is not used) */
extern BitmapCol Drawer2D_Cols[DRAWER2D_MAX_COLS];
#define Drawer2D_GetCol(c) Drawer2D_Cols[(uint8_t)c]
/* Name of default system font. */
extern String Drawer2D_FontName;

/* Clamps the given rectangle to line inside the bitmap. */
/* Returns false if rectangle is completely outside bitmap's rectangle. */
bool Drawer2D_Clamp(Bitmap* bmp, int* x, int* y, int* width, int* height);

/* Fills the given area with a simple noisy pattern. */
/* Variation determines how 'visible/obvious' the noise is. */
CC_API void Gradient_Noise(Bitmap* bmp, BitmapCol col, int variation,
						   int x, int y, int width, int height);
/* Fills the given area with a vertical gradient, linerarly interpolating from a to b. */
/* First drawn row is filled with 'a', while last drawn is filled with 'b'. */
CC_API void Gradient_Vertical(Bitmap* bmp, BitmapCol a, BitmapCol b,
							  int x, int y, int width, int height);
/* Blends the given area with the given colour. */
/* Note that this only blends RGB, A is not blended. */
CC_API void Gradient_Blend(Bitmap* bmp, BitmapCol col, int blend,
						   int x, int y, int width, int height);
/* Tints the given area, linearly interpolating from a to b. */
/* Note that this only tints RGB, A is not tinted. */
CC_API void Gradient_Tint(Bitmap* bmp, uint8_t tintA, uint8_t tintB,
						  int x, int y, int width, int height);

/* Fills the given area using pixels from an indexed bitmap. */
/* TODO: Currently this only handles square areas. */
CC_API void Drawer2D_BmpIndexed(Bitmap* bmp, int x, int y, int size,
								uint8_t* indices, BitmapCol* palette);
/* Fills the given area using pixels from a region in the source bitmap, by repeatedly tiling the region. */
/* The pixels from the region are then scaled upwards or downwards depending on scale width and height. */
CC_API void Drawer2D_BmpScaled(Bitmap* dst, int x, int y, int width, int height,
							   Bitmap* src, int srcX, int srcY, int srcWidth, int srcHeight,
							   int scaleWidth, int scaleHeight);
/* Fills the given area using pixels from the source bitmap. */
CC_API void Drawer2D_BmpCopy(Bitmap* dst, int x, int y, Bitmap* src);
/* Fills the area with the given colour. */
CC_API void Drawer2D_Clear(Bitmap* bmp, BitmapCol col, int x, int y, int width, int height);

void Drawer2D_Underline(Bitmap* bmp, int x, int y, int width, int height, BitmapCol col);
/* Draws text using the given font at the given coordinates. */
CC_API void Drawer2D_DrawText(Bitmap* bmp, struct DrawTextArgs* args, int x, int y);
/* Returns how wide the given text would be when drawn. */
int Drawer2D_TextWidth(struct DrawTextArgs* args);
/* Returns size the given text would be when drawn. */
/* NOTE: Height returned only depends on the font. (see Drawer2D_FontHeight).*/
CC_API Size2D Drawer2D_MeasureText(struct DrawTextArgs* args);
/* Similar to Drawer2D_DrawText, but trims the text with trailing ".." if wider than maxWidth. */
void Drawer2D_DrawClippedText(Bitmap* bmp, struct DrawTextArgs* args, int x, int y, int maxWidth);
/* Returns the line height for drawing any character in the font. */
int Drawer2D_FontHeight(const FontDesc* font, bool useShadow);

/* Creates a texture consisting only of the given text drawn onto it. */
/* NOTE: The returned texture is always padded up to nearest power of two dimensions. */
CC_API void Drawer2D_MakeTextTexture(struct Texture* tex, struct DrawTextArgs* args, int X, int Y);
/* Creates a texture consisting of the pixels from the given bitmap. */
/* NOTE: bmp must always have power of two dimensions. */
/* used specifies what region of the texture actually should be drawn. */
CC_API void Drawer2D_Make2DTexture(struct Texture* tex, Bitmap* bmp, Size2D used, int X, int Y);

/* Returns whether the given colour code is used/valid. */
bool Drawer2D_ValidColCodeAt(const String* text, int i);
/* Returns whether the given colour code is used/valid. */
/* NOTE: This can change if the server defines custom colour codes. */
static CC_INLINE bool Drawer2D_ValidColCode(char c) {
	return Drawer2D_GetCol(c).A > 0;
}

/* Whether text is empty or consists purely of valid colour codes. */
bool Drawer2D_IsEmptyText(const String* text);
/* Returns the last valid colour code in the given input, or \0 if not found. */
char Drawer2D_LastCol(const String* text, int start);
/* Returns whether the colour code is f, F or \0. */
bool Drawer2D_IsWhiteCol(char c);

void Drawer2D_ReducePadding_Tex(struct Texture* tex, int point, int scale);
void Drawer2D_ReducePadding_Height(int* height, int point, int scale);
/* Sets the bitmap used for drawing bitmapped fonts. (i.e. default.png) */
/* The bitmap must be square and consist of a 16x16 tile layout. */
void Drawer2D_SetFontBitmap(Bitmap* bmp);
#endif
