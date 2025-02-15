#ifndef CC_WIDGETS_H
#define CC_WIDGETS_H
#include "Gui.h"
#include "BlockID.h"
#include "Constants.h"
#include "Entity.h"
/* Contains all 2D widget implementations.
   Copyright 2014-2019 ClassiCube | Licensed under BSD-3
*/

/* A text label. */
struct TextWidget {
	Widget_Layout
	struct Texture texture;

	bool reducePadding;
	PackedCol col;
};
/* Resets state of the given text widget to default. */
CC_NOINLINE void TextWidget_Make(struct TextWidget* w);
/* Draws the given text into a texture, then updates the position and size of this widget. */
CC_NOINLINE void TextWidget_Set(struct TextWidget* w, const String* text, const FontDesc* font);
/* Shorthand for TextWidget_Make then TextWidget_Set */
CC_NOINLINE void TextWidget_Create(struct TextWidget* w, const String* text, const FontDesc* font);


typedef void (*Button_Get)(String* raw);
typedef void (*Button_Set)(const String* raw);
/* A labelled button that can be clicked on. */
struct ButtonWidget {
	Widget_Layout
	struct Texture texture;
	int minWidth;

	const char* optName;
	Button_Get GetValue;
	Button_Set SetValue;
};
/* Resets state of the given button widget, then calls ButtonWidget_Set */
CC_NOINLINE void ButtonWidget_Create(struct ButtonWidget* w, int minWidth, const String* text, const FontDesc* font, Widget_LeftClick onClick);
/* Draws the given text into a texture, then updates the position and size of this widget. */
CC_NOINLINE void ButtonWidget_Set(struct ButtonWidget* w, const String* text, const FontDesc* font);

/* Clickable and draggable scrollbar. */
struct ScrollbarWidget {
	Widget_Layout
	int totalRows, topRow;
	float scrollingAcc;
	int mouseOffset;
	bool draggingMouse;
};
/* Resets state of the given scrollbar widget to default. */
CC_NOINLINE void ScrollbarWidget_Create(struct ScrollbarWidget* w);

/* A row of blocks with a background. */
struct HotbarWidget {
	Widget_Layout
	struct Texture selTex, backTex;
	float barHeight, selBlockSize, elemSize;
	float barXOffset, borderSize;
	float scrollAcc;
	bool altHandled;
};
/* Resets state of the given hotbar widget to default. */
CC_NOINLINE void HotbarWidget_Create(struct HotbarWidget* w);


/* A table of blocks. */
struct TableWidget {
	Widget_Layout
	int elementsCount, elementsPerRow, rowsCount;
	int lastCreatedIndex;
	FontDesc font;
	int selectedIndex, cellSize;
	float selBlockExpand;
	GfxResourceID vb;
	bool pendingClose;

	BlockID elements[BLOCK_COUNT];
	struct ScrollbarWidget scroll;
	struct Texture descTex;
	int lastX, lastY;
};

CC_NOINLINE void TableWidget_Create(struct TableWidget* w);
/* Sets the selected block in the table to the given block. */
/* Also adjusts scrollbar and moves cursor to be over the given block. */
CC_NOINLINE void TableWidget_SetBlockTo(struct TableWidget* w, BlockID block);
CC_NOINLINE void TableWidget_OnInventoryChanged(struct TableWidget* w);
CC_NOINLINE void TableWidget_MakeDescTex(struct TableWidget* w, BlockID block);


#define INPUTWIDGET_MAX_LINES 3
#define INPUTWIDGET_LEN STRING_SIZE
struct InputWidget {
	Widget_Layout
	FontDesc font;		
	int (*GetMaxLines)(void);
	void (*RemakeTexture)(void* elem);  /* Remakes the raw texture containing all the chat lines. Also updates dimensions. */
	void (*OnPressedEnter)(void* elem); /* Invoked when the user presses enter. */
	bool (*AllowedChar)(void* elem, char c);

	String text;
	String lines[INPUTWIDGET_MAX_LINES];     /* raw text of each line */
	Size2D lineSizes[INPUTWIDGET_MAX_LINES]; /* size of each line in pixels */
	struct Texture inputTex;
	String prefix;
	int prefixWidth, prefixHeight;
	bool convertPercents;

	uint8_t padding;
	bool showCaret;
	int caretWidth;
	int caretX, caretY; /* Coordinates of caret in lines */
	int caretPos;       /* Position of caret, -1 for at end of string */
	PackedCol caretCol;
	struct Texture caretTex;
	double caretAccumulator;
};

/* Removes all characters and then deletes the input texture. */
CC_NOINLINE void InputWidget_Clear(struct InputWidget* w);
/* Tries appending all characters from the given string, then update the input texture. */
CC_NOINLINE void InputWidget_AppendString(struct InputWidget* w, const String* text);
/* Tries appending the given character, then updates the input texture. */
CC_NOINLINE void InputWidget_Append(struct InputWidget* w, char c);


struct InputValidator;
struct InputValidatorVTABLE {
	/* Returns a description of the range of valid values (e.g. "0 - 100") */
	void (*GetRange)(struct InputValidator*      v, String* range);
	/* Whether the given character is acceptable for this input */
	bool (*IsValidChar)(struct InputValidator*   v, char c);
	/* Whether the characters of the given string are acceptable for this input */
	/* e.g. for an integer, '-' is only valid for the first character */
	bool (*IsValidString)(struct InputValidator* v, const String* s);
	/* Whether the characters of the given string produce a valid value */
	bool (*IsValidValue)(struct InputValidator*  v, const String* s);
};

struct InputValidator {
	struct InputValidatorVTABLE* VTABLE;
	union {
		struct { const char** Names; int Count; } e;
		struct { int Min, Max; } i;
		struct { float Min, Max; } f;
	} Meta;
};

extern struct InputValidatorVTABLE HexValidator_VTABLE;
extern struct InputValidatorVTABLE IntValidator_VTABLE;
extern struct InputValidatorVTABLE SeedValidator_VTABLE;
extern struct InputValidatorVTABLE FloatValidator_VTABLE;
extern struct InputValidatorVTABLE PathValidator_VTABLE;
extern struct InputValidatorVTABLE StringValidator_VTABLE;

#define InputValidator_Hex(v) v.VTABLE = &HexValidator_VTABLE;
#define InputValidator_Int(v, lo, hi) v.VTABLE = &IntValidator_VTABLE; v.Meta.i.Min = lo; v.Meta.i.Max = hi;
#define InputValidator_Seed(v) v.VTABLE = &SeedValidator_VTABLE; v.Meta.i.Min = Int32_MinValue; v.Meta.i.Max = Int32_MaxValue;
#define InputValidator_Float(v, lo, hi) v.VTABLE = &FloatValidator_VTABLE; v.Meta.f.Min = lo; v.Meta.f.Max = hi;
#define InputValidator_Path(v) v.VTABLE = &PathValidator_VTABLE;
#define InputValidator_Enum(v, names, count) v.VTABLE = NULL; v.Meta.e.Names = names; v.Meta.e.Count = count;
#define InputValidator_String(v) v.VTABLE = &StringValidator_VTABLE;

struct MenuInputWidget {
	struct InputWidget base;
	int minWidth, minHeight;
	struct InputValidator validator;
	char _textBuffer[INPUTWIDGET_LEN];
};
CC_NOINLINE void MenuInputWidget_Create(struct MenuInputWidget* w, int width, int height, const String* text, const FontDesc* font, struct InputValidator* v);


struct ChatInputWidget {
	struct InputWidget base;
	int typingLogPos;
	String origStr;
	char _textBuffer[INPUTWIDGET_MAX_LINES * INPUTWIDGET_LEN];
	char _origBuffer[INPUTWIDGET_MAX_LINES * INPUTWIDGET_LEN];	
};

CC_NOINLINE void ChatInputWidget_Create(struct ChatInputWidget* w, const FontDesc* font);


/* Retrieves the text for the i'th line in the group */
typedef String (*TextGroupWidget_Get)(void* obj, int i);
#define TEXTGROUPWIDGET_MAX_LINES 30
#define TEXTGROUPWIDGET_LEN (STRING_SIZE + (STRING_SIZE / 2))

/* A group of text labels. */
struct TextGroupWidget {
	Widget_Layout
	int lines, defaultHeight;
	FontDesc font;
	bool placeholderHeight[TEXTGROUPWIDGET_MAX_LINES];
	bool underlineUrls;
	struct Texture* textures;
	TextGroupWidget_Get GetLine;
	void* getLineObj;
};

CC_NOINLINE void TextGroupWidget_Create(struct TextGroupWidget* w, int lines, const FontDesc* font, STRING_REF struct Texture* textures, TextGroupWidget_Get getLine);
/* Sets whether the given line has non-zero height when that line has no text in it. */
/* By default, all lines are placeholder lines. */
CC_NOINLINE void TextGroupWidget_SetUsePlaceHolder(struct TextGroupWidget* w, int index, bool placeHolder);
/* Deletes first line, then moves all other lines upwards, then redraws last line. */
/* NOTE: GetLine must also adjust the lines it returns for this to behave properly. */
CC_NOINLINE void TextGroupWidget_ShiftUp(struct TextGroupWidget* w);
/* Deletes last line, then moves all other lines downwards, then redraws first line. */
/* NOTE: GetLine must also adjust the lines it returns for this to behave properly. */
CC_NOINLINE void TextGroupWidget_ShiftDown(struct TextGroupWidget* w);
/* Returns height of lines, except for the first 0 or more empty lines. */
CC_NOINLINE int  TextGroupWidget_UsedHeight(struct TextGroupWidget* w);
/* Returns either the URL or the line underneath the given coordinates. */
CC_NOINLINE void TextGroupWidget_GetSelected(struct TextGroupWidget* w, String* text, int mouseX, int mouseY);
/* Redraws the given line, updating the texture and Y position of other lines. */
CC_NOINLINE void TextGroupWidget_Redraw(struct TextGroupWidget* w, int index);
/* Calls TextGroupWidget_Redraw for all lines */
CC_NOINLINE void TextGroupWidget_RedrawAll(struct TextGroupWidget* w);
/* Gets the text for the i'th line. */
static String TextGroupWidget_UNSAFE_Get(struct TextGroupWidget* w, int i) { return w->GetLine(w->getLineObj, i); }


struct PlayerListWidget {
	Widget_Layout
	FontDesc font;
	int namesCount, elementOffset;
	int xMin, xMax, yHeight;
	bool classic;
	struct TextWidget title;
	uint16_t ids[TABLIST_MAX_NAMES * 2];
	struct Texture textures[TABLIST_MAX_NAMES * 2];
};
CC_NOINLINE void PlayerListWidget_Create(struct PlayerListWidget* w, const FontDesc* font, bool classic);
CC_NOINLINE void PlayerListWidget_GetNameUnder(struct PlayerListWidget* w, int mouseX, int mouseY, String* name);


typedef void (*SpecialInputAppendFunc)(void* userData, char c);
struct SpecialInputTab {
	int itemsPerRow, charsPerItem;
	Size2D titleSize;
	String title, contents;	
};

struct SpecialInputWidget {
	Widget_Layout
	Size2D elementSize;
	int selectedIndex;
	bool pendingRedraw;
	struct InputWidget* target;
	struct Texture tex;
	FontDesc font;
	struct SpecialInputTab tabs[5];
	String colString;
	char _colBuffer[DRAWER2D_MAX_COLS * 4];
};

CC_NOINLINE void SpecialInputWidget_Create(struct SpecialInputWidget* w, const FontDesc* font, struct InputWidget* target);
CC_NOINLINE void SpecialInputWidget_UpdateCols(struct SpecialInputWidget* w);
CC_NOINLINE void SpecialInputWidget_SetActive(struct SpecialInputWidget* w, bool active);
#endif
