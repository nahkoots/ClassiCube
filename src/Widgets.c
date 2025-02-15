#include "Widgets.h"
#include "Graphics.h"
#include "Drawer2D.h"
#include "ExtMath.h"
#include "Funcs.h"
#include "Window.h"
#include "Inventory.h"
#include "IsometricDrawer.h"
#include "Utils.h"
#include "Model.h"
#include "Screens.h"
#include "Platform.h"
#include "Server.h"
#include "Event.h"
#include "Chat.h"
#include "Game.h"
#include "Logger.h"
#include "Bitmap.h"
#include "Block.h"

#define Widget_UV(u1,v1, u2,v2) Tex_UV(u1/256.0f,v1/256.0f, u2/256.0f,v2/256.0f)
static void Widget_NullFunc(void* widget) { }
static Size2D Size2D_Empty;

static bool Widget_Mouse(void* elem, int x, int y, MouseButton btn) { return false; }
static bool Widget_KeyDown(void* elem, Key key, bool was) { return false; }
static bool Widget_KeyUp(void* elem, Key key) { return false; }
static bool Widget_KeyPress(void* elem, char keyChar) { return false; }
static bool Widget_MouseMove(void* elem, int x, int y) { return false; }
static bool Widget_MouseScroll(void* elem, float delta) { return false; }

/*########################################################################################################################*
*-------------------------------------------------------TextWidget--------------------------------------------------------*
*#########################################################################################################################*/
static void TextWidget_Render(void* widget, double delta) {
	struct TextWidget* w = (struct TextWidget*)widget;
	if (w->texture.ID) { Texture_RenderShaded(&w->texture, w->col); }
}

static void TextWidget_Free(void* widget) {
	struct TextWidget* w = (struct TextWidget*)widget;
	Gfx_DeleteTexture(&w->texture.ID);
}

static void TextWidget_Reposition(void* widget) {
	struct TextWidget* w = (struct TextWidget*)widget;
	int oldX = w->x, oldY = w->y;

	Widget_CalcPosition(w);
	w->texture.X += w->x - oldX;
	w->texture.Y += w->y - oldY;
}

static struct WidgetVTABLE TextWidget_VTABLE = {
	Widget_NullFunc, TextWidget_Render, TextWidget_Free,  Gui_DefaultRecreate,
	Widget_KeyDown,	 Widget_KeyUp,      Widget_KeyPress,
	Widget_Mouse,    Widget_Mouse,      Widget_MouseMove, Widget_MouseScroll,
	TextWidget_Reposition,
};
void TextWidget_Make(struct TextWidget* w) {
	PackedCol col = PACKEDCOL_WHITE;
	Widget_Reset(w);
	w->VTABLE = &TextWidget_VTABLE;
	w->col    = col;
}

void TextWidget_Set(struct TextWidget* w, const String* text, const FontDesc* font) {
	struct DrawTextArgs args;
	Gfx_DeleteTexture(&w->texture.ID);

	if (Drawer2D_IsEmptyText(text)) {
		w->texture.Width  = 0; 
		w->texture.Height = Drawer2D_FontHeight(font, true);
	} else {	
		DrawTextArgs_Make(&args, text, font, true);
		Drawer2D_MakeTextTexture(&w->texture, &args, 0, 0);
	}

	if (w->reducePadding) {
		Drawer2D_ReducePadding_Tex(&w->texture, font->Size, 4);
	}

	w->width = w->texture.Width; w->height = w->texture.Height;
	Widget_Reposition(w);
	w->texture.X = w->x; w->texture.Y = w->y;
}

void TextWidget_Create(struct TextWidget* w, const String* text, const FontDesc* font) {
	TextWidget_Make(w);
	TextWidget_Set(w, text, font);
}


/*########################################################################################################################*
*------------------------------------------------------ButtonWidget-------------------------------------------------------*
*#########################################################################################################################*/
#define BUTTON_uWIDTH (200.0f / 256.0f)
#define BUTTON_MIN_WIDTH 40

static struct Texture Button_ShadowTex   = { GFX_NULL, Tex_Rect(0,0, 0,0), Widget_UV(0,66, 200,86)  };
static struct Texture Button_SelectedTex = { GFX_NULL, Tex_Rect(0,0, 0,0), Widget_UV(0,86, 200,106) };
static struct Texture Button_DisabledTex = { GFX_NULL, Tex_Rect(0,0, 0,0), Widget_UV(0,46, 200,66)  };

static void ButtonWidget_Free(void* widget) {
	struct ButtonWidget* w = (struct ButtonWidget*)widget;
	Gfx_DeleteTexture(&w->texture.ID);
}

static void ButtonWidget_Reposition(void* widget) {
	struct ButtonWidget* w = (struct ButtonWidget*)widget;
	int oldX = w->x, oldY = w->y;
	Widget_CalcPosition(w);
	
	w->texture.X += w->x - oldX;
	w->texture.Y += w->y - oldY;
}

static void ButtonWidget_Render(void* widget, double delta) {
	PackedCol normCol     = PACKEDCOL_CONST(224, 224, 224, 255);
	PackedCol activeCol   = PACKEDCOL_CONST(255, 255, 160, 255);
	PackedCol disabledCol = PACKEDCOL_CONST(160, 160, 160, 255);
	PackedCol col, white  = PACKEDCOL_WHITE;

	struct ButtonWidget* w = (struct ButtonWidget*)widget;
	struct Texture back;	
	float scale;
		
	back = w->active ? Button_SelectedTex : Button_ShadowTex;
	if (w->disabled) back = Button_DisabledTex;

	back.ID = Gui_ClassicTexture ? Gui_GuiClassicTex : Gui_GuiTex;
	back.X = w->x; back.Width  = w->width;
	back.Y = w->y; back.Height = w->height;

	if (w->width == 400) {
		/* Button can be drawn normally */
		Texture_Render(&back);
	} else {
		/* Split button down the middle */
		scale = (w->width / 400.0f) * 0.5f;
		Gfx_BindTexture(back.ID); /* avoid bind twice */

		back.Width = (w->width / 2);
		back.uv.U1 = 0.0f; back.uv.U2 = BUTTON_uWIDTH * scale;
		Gfx_Draw2DTexture(&back, white);

		back.X += (w->width / 2);
		back.uv.U1 = BUTTON_uWIDTH * (1.0f - scale); back.uv.U2 = BUTTON_uWIDTH;
		Gfx_Draw2DTexture(&back, white);
	}

	if (!w->texture.ID) return;
	col = w->disabled ? disabledCol : (w->active ? activeCol : normCol);
	Texture_RenderShaded(&w->texture, col);
}

static struct WidgetVTABLE ButtonWidget_VTABLE = {
	Widget_NullFunc, ButtonWidget_Render, ButtonWidget_Free, Gui_DefaultRecreate,
	Widget_KeyDown,	 Widget_KeyUp,        Widget_KeyPress,
	Widget_Mouse,    Widget_Mouse,        Widget_MouseMove,  Widget_MouseScroll,
	ButtonWidget_Reposition,
};
void ButtonWidget_Create(struct ButtonWidget* w, int minWidth, const String* text, const FontDesc* font, Widget_LeftClick onClick) {
	Widget_Reset(w);
	w->VTABLE    = &ButtonWidget_VTABLE;
	w->optName   = NULL;
	w->minWidth  = minWidth;
	w->MenuClick = onClick;
	ButtonWidget_Set(w, text, font);
}

void ButtonWidget_Set(struct ButtonWidget* w, const String* text, const FontDesc* font) {
	struct DrawTextArgs args;
	Gfx_DeleteTexture(&w->texture.ID);

	if (Drawer2D_IsEmptyText(text)) {
		w->texture.Width  = 0;
		w->texture.Height = Drawer2D_FontHeight(font, true);
	} else {
		DrawTextArgs_Make(&args, text, font, true);
		Drawer2D_MakeTextTexture(&w->texture, &args, 0, 0);
	}

	w->width  = max(w->texture.Width,  w->minWidth);
	w->height = max(w->texture.Height, BUTTON_MIN_WIDTH);

	Widget_Reposition(w);
	w->texture.X = w->x + (w->width  / 2 - w->texture.Width  / 2);
	w->texture.Y = w->y + (w->height / 2 - w->texture.Height / 2);
}


/*########################################################################################################################*
*-----------------------------------------------------ScrollbarWidget-----------------------------------------------------*
*#########################################################################################################################*/
#define TABLE_MAX_ROWS_DISPLAYED 8
#define SCROLL_WIDTH 22
#define SCROLL_BORDER 2
#define SCROLL_NUBS_WIDTH 3
static PackedCol Scroll_BackCol  = PACKEDCOL_CONST( 10,  10,  10, 220);
static PackedCol Scroll_BarCol   = PACKEDCOL_CONST(100, 100, 100, 220);
static PackedCol Scroll_HoverCol = PACKEDCOL_CONST(122, 122, 122, 220);

static void ScrollbarWidget_ClampTopRow(struct ScrollbarWidget* w) {
	int maxTop = w->totalRows - TABLE_MAX_ROWS_DISPLAYED;
	if (w->topRow >= maxTop) w->topRow = maxTop;
	if (w->topRow < 0) w->topRow = 0;
}

static float ScrollbarWidget_GetScale(struct ScrollbarWidget* w) {
	float rows = (float)w->totalRows;
	return (w->height - SCROLL_BORDER * 2) / rows;
}

static void ScrollbarWidget_GetScrollbarCoords(struct ScrollbarWidget* w, int* y, int* height) {
	float scale = ScrollbarWidget_GetScale(w);
	*y = Math_Ceil(w->topRow * scale) + SCROLL_BORDER;
	*height = Math_Ceil(TABLE_MAX_ROWS_DISPLAYED * scale);
	*height = min(*y + *height, w->height - SCROLL_BORDER) - *y;
}

static void ScrollbarWidget_Render(void* widget, double delta) {
	struct ScrollbarWidget* w = (struct ScrollbarWidget*)widget;
	int x, y, width, height;
	PackedCol barCol;
	bool hovered;

	x = w->x; width = w->width;
	Gfx_Draw2DFlat(x, w->y, width, w->height, Scroll_BackCol);

	ScrollbarWidget_GetScrollbarCoords(w, &y, &height);
	x += SCROLL_BORDER; y += w->y;
	width -= SCROLL_BORDER * 2; 

	hovered = Gui_Contains(x, y, width, height, Mouse_X, Mouse_Y);
	barCol  = hovered ? Scroll_HoverCol : Scroll_BarCol;
	Gfx_Draw2DFlat(x, y, width, height, barCol);

	if (height < 20) return;
	x += SCROLL_NUBS_WIDTH; y += (height / 2);
	width -= SCROLL_NUBS_WIDTH * 2;

	Gfx_Draw2DFlat(x, y - 1 - 4, width, SCROLL_BORDER, Scroll_BackCol);
	Gfx_Draw2DFlat(x, y - 1,     width, SCROLL_BORDER, Scroll_BackCol);
	Gfx_Draw2DFlat(x, y - 1 + 4, width, SCROLL_BORDER, Scroll_BackCol);
}

static bool ScrollbarWidget_MouseDown(void* widget, int x, int y, MouseButton btn) {
	struct ScrollbarWidget* w = (struct ScrollbarWidget*)widget;
	int posY, height;

	if (w->draggingMouse) return true;
	if (btn != MOUSE_LEFT) return false;
	if (x < w->x || x >= w->x + w->width) return false;

	y -= w->y;
	ScrollbarWidget_GetScrollbarCoords(w, &posY, &height);

	if (y < posY) {
		w->topRow -= TABLE_MAX_ROWS_DISPLAYED;
	} else if (y >= posY + height) {
		w->topRow += TABLE_MAX_ROWS_DISPLAYED;
	} else {
		w->draggingMouse = true;
		w->mouseOffset = y - posY;
	}
	ScrollbarWidget_ClampTopRow(w);
	return true;
}

static bool ScrollbarWidget_MouseUp(void* widget, int x, int y, MouseButton btn) {
	struct ScrollbarWidget* w = (struct ScrollbarWidget*)widget;
	w->draggingMouse = false;
	w->mouseOffset   = 0;
	return true;
}

static bool ScrollbarWidget_MouseScroll(void* widget, float delta) {
	struct ScrollbarWidget* w = (struct ScrollbarWidget*)widget;
	int steps = Utils_AccumulateWheelDelta(&w->scrollingAcc, delta);

	w->topRow -= steps;
	ScrollbarWidget_ClampTopRow(w);
	return true;
}

static bool ScrollbarWidget_MouseMove(void* widget, int x, int y) {
	struct ScrollbarWidget* w = (struct ScrollbarWidget*)widget;
	float scale;

	if (w->draggingMouse) {
		y -= w->y;
		scale = ScrollbarWidget_GetScale(w);
		w->topRow = (int)((y - w->mouseOffset) / scale);
		ScrollbarWidget_ClampTopRow(w);
		return true;
	}
	return false;
}

static struct WidgetVTABLE ScrollbarWidget_VTABLE = {
	Widget_NullFunc,           ScrollbarWidget_Render,  Widget_NullFunc,           Gui_DefaultRecreate,
	Widget_KeyDown,	           Widget_KeyUp,            Widget_KeyPress,
	ScrollbarWidget_MouseDown, ScrollbarWidget_MouseUp, ScrollbarWidget_MouseMove, ScrollbarWidget_MouseScroll,
	Widget_CalcPosition,
};
void ScrollbarWidget_Create(struct ScrollbarWidget* w) {
	Widget_Reset(w);
	w->VTABLE = &ScrollbarWidget_VTABLE;
	w->width  = SCROLL_WIDTH;
	w->totalRows     = 0;
	w->topRow       = 0;
	w->scrollingAcc  = 0.0f;
	w->draggingMouse = false;
	w->mouseOffset   = 0;
}


/*########################################################################################################################*
*------------------------------------------------------HotbarWidget-------------------------------------------------------*
*#########################################################################################################################*/
static void HotbarWidget_RenderHotbarOutline(struct HotbarWidget* w) {
	PackedCol white = PACKEDCOL_WHITE;
	GfxResourceID tex;
	float width;
	int i, x;
	
	tex = Gui_ClassicTexture ? Gui_GuiClassicTex : Gui_GuiTex;
	w->backTex.ID = tex;
	Texture_Render(&w->backTex);

	i     = Inventory.SelectedIndex;
	width = w->elemSize + w->borderSize;
	x     = (int)(w->x + w->barXOffset + width * i + w->elemSize / 2);

	w->selTex.ID = tex;
	w->selTex.X  = (int)(x - w->selBlockSize / 2);
	Gfx_Draw2DTexture(&w->selTex, white);
}

static void HotbarWidget_RenderHotbarBlocks(struct HotbarWidget* w) {
	/* TODO: Should hotbar use its own VB? */
	VertexP3fT2fC4b vertices[INVENTORY_BLOCKS_PER_HOTBAR * ISOMETRICDRAWER_MAXVERTICES];
	float width, scale;
	int i, x, y;

	IsometricDrawer_BeginBatch(vertices, Models.Vb);
	width =  w->elemSize + w->borderSize;
	scale = (w->elemSize * 13.5f/16.0f) / 2.0f;

	for (i = 0; i < INVENTORY_BLOCKS_PER_HOTBAR; i++) {
		x = (int)(w->x + w->barXOffset + width * i + w->elemSize / 2);
		y = (int)(w->y + (w->height - w->barHeight / 2));
		IsometricDrawer_DrawBatch(Inventory_Get(i), scale, x, y);
	}
	IsometricDrawer_EndBatch();
}

static void HotbarWidget_RepositonBackgroundTexture(struct HotbarWidget* w) {
	w->backTex.ID = GFX_NULL;
	Tex_SetRect(w->backTex, w->x,w->y, w->width,w->height);
	Tex_SetUV(w->backTex,   0,0, 182/256.0f,22/256.0f);
}

static void HotbarWidget_RepositionSelectionTexture(struct HotbarWidget* w) {
	float scale = Game_GetHotbarScale();
	int hSize = (int)w->selBlockSize;
	int vSize = (int)(22.0f * scale);
	int y = w->y + (w->height - (int)(23.0f * scale));

	w->selTex.ID = GFX_NULL;
	Tex_SetRect(w->selTex, 0,y, hSize,vSize);
	Tex_SetUV(w->selTex,   0,22/256.0f, 24/256.0f,44/256.0f);
}

static int HotbarWidget_ScrolledIndex(struct HotbarWidget* w, float delta, int index, int dir) {
	int steps = Utils_AccumulateWheelDelta(&w->scrollAcc, delta);
	index += (dir * steps) % INVENTORY_BLOCKS_PER_HOTBAR;

	if (index < 0) index += INVENTORY_BLOCKS_PER_HOTBAR;
	if (index >= INVENTORY_BLOCKS_PER_HOTBAR) {
		index -= INVENTORY_BLOCKS_PER_HOTBAR;
	}
	return index;
}

static void HotbarWidget_Reposition(void* widget) {
	struct HotbarWidget* w = (struct HotbarWidget*)widget;
	float scale = Game_GetHotbarScale();

	w->barHeight = (float)Math_Floor(22.0f * scale);
	w->width     = (int)(182 * scale);
	w->height    = (int)w->barHeight;

	w->selBlockSize = (float)Math_Ceil(24.0f * scale);
	w->elemSize     = 16.0f * scale;
	w->barXOffset   = 3.1f * scale;
	w->borderSize   = 4.0f * scale;

	Widget_CalcPosition(w);
	HotbarWidget_RepositonBackgroundTexture(w);
	HotbarWidget_RepositionSelectionTexture(w);
}

static void HotbarWidget_Init(void* widget) {
	struct HotbarWidget* w = (struct HotbarWidget*)widget;
	Widget_Reposition(w);
}

static void HotbarWidget_Render(void* widget, double delta) {
	struct HotbarWidget* w = (struct HotbarWidget*)widget;
	HotbarWidget_RenderHotbarOutline(w);
	HotbarWidget_RenderHotbarBlocks(w);
}

static bool HotbarWidget_KeyDown(void* widget, Key key, bool was) {
	struct HotbarWidget* w = (struct HotbarWidget*)widget;
	int index;
	if (key < '1' || key > '9') return false;

	index = key - '1';
	if (KeyBind_IsPressed(KEYBIND_HOTBAR_SWITCH)) {
		/* Pick from first to ninth row */
		Inventory_SetHotbarIndex(index);
		w->altHandled = true;
	} else {
		Inventory_SetSelectedIndex(index);
	}
	return true;
}

static bool HotbarWidget_KeyUp(void* widget, Key key) {
	struct HotbarWidget* w = (struct HotbarWidget*)widget;
	int index;

	/* Need to handle these cases:
	     a) user presses alt then number
	     b) user presses alt
	   We only do case b) if case a) did not happen */
	if (key != KeyBinds[KEYBIND_HOTBAR_SWITCH]) return false;
	if (w->altHandled) { w->altHandled = false; return true; } /* handled already */

	/* Don't switch hotbar when alt+tab */
	if (!Window_Focused) return true;

	/* Alternate between first and second row */
	index = Inventory.Offset == 0 ? 1 : 0;
	Inventory_SetHotbarIndex(index);
	return true;
}

static bool HotbarWidget_MouseDown(void* widget, int x, int y, MouseButton btn) {
	struct HotbarWidget* w = (struct HotbarWidget*)widget;
	struct Screen* screen;
	int width, height;
	int i, cellX, cellY;

	if (btn != MOUSE_LEFT || !Widget_Contains(w, x, y)) return false;
	screen = Gui_GetActiveScreen();
	if (screen != InventoryScreen_UNSAFE_RawPointer) return false;

	width  = (int)(w->elemSize + w->borderSize);
	height = Math_Ceil(w->barHeight);

	for (i = 0; i < INVENTORY_BLOCKS_PER_HOTBAR; i++) {
		cellX = (int)(w->x + width * i);
		cellY = (int)(w->y + (w->height - height));

		if (Gui_Contains(cellX, cellY, width, height, x, y)) {
			Inventory_SetSelectedIndex(i);
			return true;
		}
	}
	return false;
}

static bool HotbarWidget_MouseScroll(void* widget, float delta) {
	struct HotbarWidget* w = (struct HotbarWidget*)widget;
	int index;

	if (KeyBind_IsPressed(KEYBIND_HOTBAR_SWITCH)) {
		index = Inventory.Offset / INVENTORY_BLOCKS_PER_HOTBAR;
		index = HotbarWidget_ScrolledIndex(w, delta, index, 1);
		Inventory_SetHotbarIndex(index);
		w->altHandled = true;
	} else {
		index = HotbarWidget_ScrolledIndex(w, delta, Inventory.SelectedIndex, -1);
		Inventory_SetSelectedIndex(index);
	}
	return true;
}

static struct WidgetVTABLE HotbarWidget_VTABLE = {
	HotbarWidget_Init,      HotbarWidget_Render, Widget_NullFunc,  Gui_DefaultRecreate,
	HotbarWidget_KeyDown,	HotbarWidget_KeyUp,  Widget_KeyPress,
	HotbarWidget_MouseDown, Widget_Mouse,        Widget_MouseMove, HotbarWidget_MouseScroll,
	HotbarWidget_Reposition,
};
void HotbarWidget_Create(struct HotbarWidget* w) {
	Widget_Reset(w);
	w->VTABLE    = &HotbarWidget_VTABLE;
	w->horAnchor = ANCHOR_CENTRE;
	w->verAnchor = ANCHOR_MAX;
}


/*########################################################################################################################*
*-------------------------------------------------------TableWidget-------------------------------------------------------*
*#########################################################################################################################*/
static int Table_X(struct TableWidget* w) { return w->x - 5 - 10; }
static int Table_Y(struct TableWidget* w) { return w->y - 5 - 30; }
static int Table_Width(struct TableWidget* w) {
	return w->elementsPerRow * w->cellSize + 10 + 20; 
}
static int Table_Height(struct TableWidget* w) {
	return min(w->rowsCount, TABLE_MAX_ROWS_DISPLAYED) * w->cellSize + 10 + 40;
}

#define TABLE_MAX_VERTICES (8 * 10 * ISOMETRICDRAWER_MAXVERTICES)

static bool TableWidget_GetCoords(struct TableWidget* w, int i, int* cellX, int* cellY) {
	int x, y;
	x = i % w->elementsPerRow;
	y = i / w->elementsPerRow - w->scroll.topRow;

	*cellX = w->x + w->cellSize * x;
	*cellY = w->y + w->cellSize * y + 3;
	return y >= 0 && y < TABLE_MAX_ROWS_DISPLAYED;
}

static void TableWidget_UpdateScrollbarPos(struct TableWidget* w) {
	struct ScrollbarWidget* scroll = &w->scroll;
	scroll->x = Table_X(w) + Table_Width(w);
	scroll->y = Table_Y(w);
	scroll->height    = Table_Height(w);
	scroll->totalRows = w->rowsCount;
}

static void TableWidget_MoveCursorToSelected(struct TableWidget* w) {
	int x, y, idx;
	if (w->selectedIndex == -1) return;

	idx = w->selectedIndex;
	TableWidget_GetCoords(w, idx, &x, &y);

	x += w->cellSize / 2; y += w->cellSize / 2;
	Cursor_SetPosition(x, y);
}

static void TableWidget_MakeBlockDesc(String* desc, BlockID block) {
	String name;
	int block_ = block;
	if (Game_PureClassic) { String_AppendConst(desc, "Select block"); return; }

	name = Block_UNSAFE_GetName(block);
	String_AppendString(desc, &name);
	if (Game_ClassicMode) return;

	String_Format1(desc, " (ID %i&f", &block_);
	if (!Blocks.CanPlace[block])  { String_AppendConst(desc,  ", place &cNo&f"); }
	if (!Blocks.CanDelete[block]) { String_AppendConst(desc, ", delete &cNo&f"); }
	String_Append(desc, ')');
}

static void TableWidget_UpdateDescTexPos(struct TableWidget* w) {
	w->descTex.X = w->x + w->width / 2 - w->descTex.Width / 2;
	w->descTex.Y = w->y - w->descTex.Height - 5;
}

static void TableWidget_UpdatePos(struct TableWidget* w) {
	int rowsDisplayed = min(TABLE_MAX_ROWS_DISPLAYED, w->rowsCount);
	w->width  = w->cellSize * w->elementsPerRow;
	w->height = w->cellSize * rowsDisplayed;
	w->x = Window_Width  / 2 - w->width  / 2;
	w->y = Window_Height / 2 - w->height / 2;
	TableWidget_UpdateDescTexPos(w);
}

static void TableWidget_RecreateDescTex(struct TableWidget* w) {
	if (w->selectedIndex == w->lastCreatedIndex) return;
	if (w->elementsCount == 0) return;
	w->lastCreatedIndex = w->selectedIndex;

	Gfx_DeleteTexture(&w->descTex.ID);
	if (w->selectedIndex == -1) return;
	TableWidget_MakeDescTex(w, w->elements[w->selectedIndex]);
}

void TableWidget_MakeDescTex(struct TableWidget* w, BlockID block) {
	String desc; char descBuffer[STRING_SIZE * 2];
	struct DrawTextArgs args;

	Gfx_DeleteTexture(&w->descTex.ID);
	if (block == BLOCK_AIR) return;
	String_InitArray(desc, descBuffer);
	TableWidget_MakeBlockDesc(&desc, block);
	
	DrawTextArgs_Make(&args, &desc, &w->font, true);
	Drawer2D_MakeTextTexture(&w->descTex, &args, 0, 0);
	TableWidget_UpdateDescTexPos(w);
}

static bool TableWidget_RowEmpty(struct TableWidget* w, int start) {
	int i, end = min(start + w->elementsPerRow, Array_Elems(Inventory.Map));

	for (i = start; i < end; i++) {
		if (Inventory.Map[i] != BLOCK_AIR) return false;
	}
	return true;
}

static void TableWidget_RecreateElements(struct TableWidget* w) {
	int i, max = Game_UseCPEBlocks ? BLOCK_COUNT : BLOCK_ORIGINAL_COUNT;
	BlockID block;
	w->elementsCount = 0;

	for (i = 0; i < Array_Elems(Inventory.Map); ) {
		if ((i % w->elementsPerRow) == 0 && TableWidget_RowEmpty(w, i)) {
			i += w->elementsPerRow; continue;
		}

		block = Inventory.Map[i];
		if (block < max) { w->elements[w->elementsCount++] = block; }
		i++;
	}

	w->rowsCount = Math_CeilDiv(w->elementsCount, w->elementsPerRow);
	TableWidget_UpdateScrollbarPos(w);
	TableWidget_UpdatePos(w);
}

static void TableWidget_Init(void* widget) {
	struct TableWidget* w = (struct TableWidget*)widget;
	w->lastX = Mouse_X; w->lastY = Mouse_Y;

	ScrollbarWidget_Create(&w->scroll);
	TableWidget_RecreateElements(w);
	Widget_Reposition(w);
}

static void TableWidget_Render(void* widget, double delta) {
	struct TableWidget* w = (struct TableWidget*)widget;
	VertexP3fT2fC4b vertices[TABLE_MAX_VERTICES];
	int cellSize, size;
	float off;
	int i, x, y;

	/* These were sourced by taking a screenshot of vanilla
	Then using paint to extract the colour components
	Then using wolfram alpha to solve the glblendfunc equation */
	PackedCol topBackCol    = PACKEDCOL_CONST( 34,  34,  34, 168);
	PackedCol bottomBackCol = PACKEDCOL_CONST( 57,  57, 104, 202);
	PackedCol topSelCol     = PACKEDCOL_CONST(255, 255, 255, 142);
	PackedCol bottomSelCol  = PACKEDCOL_CONST(255, 255, 255, 192);

	Gfx_Draw2DGradient(Table_X(w), Table_Y(w),
		Table_Width(w), Table_Height(w), topBackCol, bottomBackCol);

	if (w->rowsCount > TABLE_MAX_ROWS_DISPLAYED) {
		Elem_Render(&w->scroll, delta);
	}

	cellSize = w->cellSize;
	if (w->selectedIndex != -1 && Game_ClassicMode) {
		TableWidget_GetCoords(w, w->selectedIndex, &x, &y);

		off  = cellSize * 0.1f;
		size = (int)(cellSize + off * 2);
		Gfx_Draw2DGradient((int)(x - off), (int)(y - off),
			size, size, topSelCol, bottomSelCol);
	}
	Gfx_SetTexturing(true);
	Gfx_SetVertexFormat(VERTEX_FORMAT_P3FT2FC4B);

	IsometricDrawer_BeginBatch(vertices, w->vb);
	for (i = 0; i < w->elementsCount; i++) {
		if (!TableWidget_GetCoords(w, i, &x, &y)) continue;

		/* We want to always draw the selected block on top of others */
		if (i == w->selectedIndex) continue;
		IsometricDrawer_DrawBatch(w->elements[i], cellSize * 0.7f / 2.0f,
			x + cellSize / 2, y + cellSize / 2);
	}

	i = w->selectedIndex;
	if (i != -1) {
		TableWidget_GetCoords(w, i, &x, &y);

		IsometricDrawer_DrawBatch(w->elements[i],
			(cellSize + w->selBlockExpand) * 0.7f / 2.0f,
			x + cellSize / 2, y + cellSize / 2);
	}
	IsometricDrawer_EndBatch();

	if (w->descTex.ID) { Texture_Render(&w->descTex); }
	Gfx_SetTexturing(false);
}

static void TableWidget_Free(void* widget) {
	struct TableWidget* w = (struct TableWidget*)widget;
	Gfx_DeleteVb(&w->vb);
	Gfx_DeleteTexture(&w->descTex.ID);
	w->lastCreatedIndex = -1000;
}

static void TableWidget_Recreate(void* widget) {
	struct TableWidget* w = (struct TableWidget*)widget;
	Elem_TryFree(w);
	w->vb = Gfx_CreateDynamicVb(VERTEX_FORMAT_P3FT2FC4B, TABLE_MAX_VERTICES);
	TableWidget_RecreateDescTex(w);
}

static void TableWidget_Reposition(void* widget) {
	struct TableWidget* w = (struct TableWidget*)widget;
	float scale = Game_GetInventoryScale();
	w->cellSize = (int)(50 * Math_SqrtF(scale));
	w->selBlockExpand = 25.0f * Math_SqrtF(scale);

	TableWidget_UpdatePos(w);
	TableWidget_UpdateScrollbarPos(w);
}

static void TableWidget_ScrollRelative(struct TableWidget* w, int delta) {
	int start = w->selectedIndex, index = start;
	index += delta;
	if (index < 0) index -= delta;
	if (index >= w->elementsCount) index -= delta;
	w->selectedIndex = index;

	/* adjust scrollbar by number of rows moved up/down */
	w->scroll.topRow += (index / w->elementsPerRow) - (start / w->elementsPerRow);
	ScrollbarWidget_ClampTopRow(&w->scroll);

	TableWidget_RecreateDescTex(w);
	TableWidget_MoveCursorToSelected(w);
}

static bool TableWidget_MouseDown(void* widget, int x, int y, MouseButton btn) {
	struct TableWidget* w = (struct TableWidget*)widget;
	w->pendingClose = false;
	if (btn != MOUSE_LEFT) return false;

	if (Elem_HandlesMouseDown(&w->scroll, x, y, btn)) {
		return true;
	} else if (w->selectedIndex != -1 && w->elements[w->selectedIndex] != BLOCK_AIR) {
		Inventory_SetSelectedBlock(w->elements[w->selectedIndex]);
		w->pendingClose = true;
		return true;
	} else if (Gui_Contains(Table_X(w), Table_Y(w), Table_Width(w), Table_Height(w), x, y)) {
		return true;
	}
	return false;
}

static bool TableWidget_MouseUp(void* widget, int x, int y, MouseButton btn) {
	struct TableWidget* w = (struct TableWidget*)widget;
	return Elem_HandlesMouseUp(&w->scroll, x, y, btn);
}

static bool TableWidget_MouseScroll(void* widget, float delta) {
	struct TableWidget* w = (struct TableWidget*)widget;
	int origTopRow, index;

	bool bounds = Gui_Contains(Table_X(w), Table_Y(w),
		Table_Width(w) + w->scroll.width, Table_Height(w), Mouse_X, Mouse_Y);
	if (!bounds) return false;

	origTopRow = w->scroll.topRow;
	Elem_HandlesMouseScroll(&w->scroll, delta);
	if (w->selectedIndex == -1) return true;

	index = w->selectedIndex;
	index += (w->scroll.topRow - origTopRow) * w->elementsPerRow;
	if (index >= w->elementsCount) index = -1;

	w->selectedIndex = index;
	TableWidget_RecreateDescTex(w);
	return true;
}

static bool TableWidget_MouseMove(void* widget, int x, int y) {
	struct TableWidget* w = (struct TableWidget*)widget;
	int cellSize, maxHeight;
	int i, cellX, cellY;

	if (Elem_HandlesMouseMove(&w->scroll, x, y)) return true;
	if (w->lastX == x && w->lastY == y) return true;
	w->lastX = x; w->lastY = y;

	w->selectedIndex = -1;
	cellSize  = w->cellSize;
	maxHeight = cellSize * TABLE_MAX_ROWS_DISPLAYED;

	if (Gui_Contains(w->x, w->y + 3, w->width, maxHeight - 3 * 2, x, y)) {
		for (i = 0; i < w->elementsCount; i++) {
			TableWidget_GetCoords(w, i, &cellX, &cellY);

			if (Gui_Contains(cellX, cellY, cellSize, cellSize, x, y)) {
				w->selectedIndex = i;
				break;
			}
		}
	}
	TableWidget_RecreateDescTex(w);
	return true;
}

static bool TableWidget_KeyDown(void* widget, Key key, bool was) {
	struct TableWidget* w = (struct TableWidget*)widget;
	if (w->selectedIndex == -1) return false;

	if (key == KEY_LEFT || key == KEY_KP4) {
		TableWidget_ScrollRelative(w, -1);
	} else if (key == KEY_RIGHT || key == KEY_KP6) {
		TableWidget_ScrollRelative(w, 1);
	} else if (key == KEY_UP || key == KEY_KP8) {
		TableWidget_ScrollRelative(w, -w->elementsPerRow);
	} else if (key == KEY_DOWN || key == KEY_KP2) {
		TableWidget_ScrollRelative(w, w->elementsPerRow);
	} else {
		return false;
	}
	return true;
}

static struct WidgetVTABLE TableWidget_VTABLE = {
	TableWidget_Init,      TableWidget_Render,  TableWidget_Free,      TableWidget_Recreate,
	TableWidget_KeyDown,   Widget_KeyUp,        Widget_KeyPress,
	TableWidget_MouseDown, TableWidget_MouseUp, TableWidget_MouseMove, TableWidget_MouseScroll,
	TableWidget_Reposition,
};
void TableWidget_Create(struct TableWidget* w) {	
	Widget_Reset(w);
	w->VTABLE = &TableWidget_VTABLE;
	w->lastCreatedIndex = -1000;
}

void TableWidget_SetBlockTo(struct TableWidget* w, BlockID block) {
	int i;
	w->selectedIndex = -1;
	
	for (i = 0; i < w->elementsCount; i++) {
		if (w->elements[i] == block) w->selectedIndex = i;
	}
	/* When holding air, inventory should open at middle */
	if (block == BLOCK_AIR) w->selectedIndex = -1;

	w->scroll.topRow = w->selectedIndex / w->elementsPerRow;
	w->scroll.topRow -= (TABLE_MAX_ROWS_DISPLAYED - 1);
	ScrollbarWidget_ClampTopRow(&w->scroll);
	TableWidget_MoveCursorToSelected(w);
	TableWidget_RecreateDescTex(w);
}

void TableWidget_OnInventoryChanged(struct TableWidget* w) {
	TableWidget_RecreateElements(w);
	if (w->selectedIndex >= w->elementsCount) {
		w->selectedIndex = w->elementsCount - 1;
	}
	w->lastX = -1; w->lastY = -1;

	w->scroll.topRow = w->selectedIndex / w->elementsPerRow;
	ScrollbarWidget_ClampTopRow(&w->scroll);
	TableWidget_RecreateDescTex(w);
}


/*########################################################################################################################*
*-------------------------------------------------------InputWidget-------------------------------------------------------*
*#########################################################################################################################*/
static void InputWidget_FormatLine(struct InputWidget* w, int i, String* line) {
	String src = w->lines[i];
	if (!w->convertPercents) { String_AppendString(line, &src); return; }

	for (i = 0; i < src.length; i++) {
		char c = src.buffer[i];
		if (c == '%' && Drawer2D_ValidColCodeAt(&src, i + 1)) { c = '&'; }
		String_Append(line, c);
	}
}

static void InputWidget_CalculateLineSizes(struct InputWidget* w) {
	String line; char lineBuffer[STRING_SIZE];
	struct DrawTextArgs args;
	Size2D size;
	int y;

	for (y = 0; y < INPUTWIDGET_MAX_LINES; y++) {
		w->lineSizes[y] = Size2D_Empty;
	}
	w->lineSizes[0].Width = w->prefixWidth;
	DrawTextArgs_MakeEmpty(&args, &w->font, true);

	String_InitArray(line, lineBuffer);
	for (y = 0; y < w->GetMaxLines(); y++) {
		line.length = 0;
		InputWidget_FormatLine(w, y, &line);

		args.text = line;
		size = Drawer2D_MeasureText(&args);
		w->lineSizes[y].Width += size.Width;
		w->lineSizes[y].Height = size.Height;
	}

	if (w->lineSizes[0].Height == 0) {
		w->lineSizes[0].Height = w->prefixHeight;
	}
}

static char InputWidget_GetLastCol(struct InputWidget* w, int x, int y) {
	String line; char lineBuffer[STRING_SIZE];
	char col;
	String_InitArray(line, lineBuffer);

	for (; y >= 0; y--) {
		line.length = 0;
		InputWidget_FormatLine(w, y, &line);

		col = Drawer2D_LastCol(&line, x);
		if (col) return col;
		if (y > 0) { x = w->lines[y - 1].length; }
	}
	return '\0';
}

static void InputWidget_UpdateCaret(struct InputWidget* w) {
	BitmapCol col;
	String line; char lineBuffer[STRING_SIZE];
	struct DrawTextArgs args;
	int maxChars, lineWidth;
	char colCode;
	
	maxChars = w->GetMaxLines() * INPUTWIDGET_LEN;
	if (w->caretPos >= maxChars) w->caretPos = -1;
	WordWrap_GetCoords(w->caretPos, w->lines, w->GetMaxLines(), &w->caretX, &w->caretY);

	DrawTextArgs_MakeEmpty(&args, &w->font, false);
	w->caretAccumulator = 0;
	w->caretTex.Width   = w->caretWidth;

	/* Caret is at last character on line */
	if (w->caretX == INPUTWIDGET_LEN) {
		lineWidth = w->lineSizes[w->caretY].Width;	
	} else {
		String_InitArray(line, lineBuffer);
		InputWidget_FormatLine(w, w->caretY, &line);

		args.text = String_UNSAFE_Substring(&line, 0, w->caretX);
		lineWidth = Drawer2D_TextWidth(&args);
		if (w->caretY == 0) lineWidth += w->prefixWidth;

		if (w->caretX < line.length) {
			args.text = String_UNSAFE_Substring(&line, w->caretX, 1);
			args.useShadow = true;
			w->caretTex.Width = Drawer2D_TextWidth(&args);
		}
	}

	w->caretTex.X = w->x + w->padding + lineWidth;
	w->caretTex.Y = w->inputTex.Y + w->caretY * w->lineSizes[0].Height + 2;
	colCode = InputWidget_GetLastCol(w, w->caretX, w->caretY);

	if (colCode) {
		col = Drawer2D_GetCol(colCode);
		w->caretCol.B = col.B; w->caretCol.G = col.G; 
		w->caretCol.R = col.R; w->caretCol.A = col.A;
	} else {
		PackedCol white = PACKEDCOL_WHITE;
		w->caretCol = PackedCol_Scale(white, 0.8f);
	}
}

static void InputWidget_RenderCaret(struct InputWidget* w, double delta) {
	float second;
	if (!w->showCaret) return;
	w->caretAccumulator += delta;

	second = Math_Mod1((float)w->caretAccumulator);
	if (second < 0.5f) Texture_RenderShaded(&w->caretTex, w->caretCol);
}

static void InputWidget_OnPressedEnter(void* widget) {
	struct InputWidget* w = (struct InputWidget*)widget;
	InputWidget_Clear(w);
	w->height = w->prefixHeight;
}

void InputWidget_Clear(struct InputWidget* w) {
	int i;
	w->text.length = 0;
	
	for (i = 0; i < Array_Elems(w->lines); i++) {
		w->lines[i] = String_Empty;
	}

	w->caretPos = -1;
	Gfx_DeleteTexture(&w->inputTex.ID);
}

static bool InputWidget_AllowedChar(void* widget, char c) {
	return Server.SupportsFullCP437 || (Convert_CP437ToUnicode(c) == c);
}

static void InputWidget_AppendChar(struct InputWidget* w, char c) {
	if (w->caretPos == -1) {
		String_InsertAt(&w->text, w->text.length, c);
	} else {
		String_InsertAt(&w->text, w->caretPos, c);
		w->caretPos++;
		if (w->caretPos >= w->text.length) { w->caretPos = -1; }
	}
}

static bool InputWidget_TryAppendChar(struct InputWidget* w, char c) {
	int maxChars = w->GetMaxLines() * INPUTWIDGET_LEN;
	if (w->text.length >= maxChars) return false;
	if (!w->AllowedChar(w, c)) return false;

	InputWidget_AppendChar(w, c);
	return true;
}

void InputWidget_AppendString(struct InputWidget* w, const String* text) {
	int i, appended = 0;
	for (i = 0; i < text->length; i++) {
		if (InputWidget_TryAppendChar(w, text->buffer[i])) appended++;
	}

	if (!appended) return;
	Elem_Recreate(w);
}

void InputWidget_Append(struct InputWidget* w, char c) {
	if (!InputWidget_TryAppendChar(w, c)) return;
	Elem_Recreate(w);
}

static void InputWidget_DeleteChar(struct InputWidget* w) {
	if (!w->text.length) return;

	if (w->caretPos == -1) {
		String_DeleteAt(&w->text, w->text.length - 1);
	} else if (w->caretPos > 0) {
		w->caretPos--;
		String_DeleteAt(&w->text, w->caretPos);
	}
}

static bool InputWidget_CheckCol(struct InputWidget* w, int index) {
	char code, col;
	if (index < 0) return false;

	code = w->text.buffer[index];
	col  = w->text.buffer[index + 1];
	return (code == '%' || code == '&') && Drawer2D_ValidColCode(col);
}

static void InputWidget_BackspaceKey(struct InputWidget* w) {
	int i, len;

	if (Key_IsActionPressed()) {
		if (w->caretPos == -1) { w->caretPos = w->text.length - 1; }
		len = WordWrap_GetBackLength(&w->text, w->caretPos);
		if (!len) return;

		w->caretPos -= len;
		if (w->caretPos < 0) { w->caretPos = 0; }

		for (i = 0; i <= len; i++) {
			String_DeleteAt(&w->text, w->caretPos);
		}

		if (w->caretPos >= w->text.length) { w->caretPos = -1; }
		if (w->caretPos == -1 && w->text.length > 0) {
			String_InsertAt(&w->text, w->text.length, ' ');
		} else if (w->caretPos >= 0 && w->text.buffer[w->caretPos] != ' ') {
			String_InsertAt(&w->text, w->caretPos, ' ');
		}
		Elem_Recreate(w);
	} else if (w->text.length > 0 && w->caretPos != 0) {
		int index = w->caretPos == -1 ? w->text.length - 1 : w->caretPos;
		if (InputWidget_CheckCol(w, index - 1)) {
			InputWidget_DeleteChar(w); /* backspace XYZ%e to XYZ */
		} else if (InputWidget_CheckCol(w, index - 2)) {
			InputWidget_DeleteChar(w); 
			InputWidget_DeleteChar(w); /* backspace XYZ%eH to XYZ */
		}

		InputWidget_DeleteChar(w);
		Elem_Recreate(w);
	}
}

static void InputWidget_DeleteKey(struct InputWidget* w) {
	if (w->text.length > 0 && w->caretPos != -1) {
		String_DeleteAt(&w->text, w->caretPos);
		if (w->caretPos >= w->text.length) { w->caretPos = -1; }
		Elem_Recreate(w);
	}
}

static void InputWidget_LeftKey(struct InputWidget* w) {
	if (Key_IsActionPressed()) {
		if (w->caretPos == -1) { w->caretPos = w->text.length - 1; }
		w->caretPos -= WordWrap_GetBackLength(&w->text, w->caretPos);
		InputWidget_UpdateCaret(w);
		return;
	}

	if (w->text.length > 0) {
		if (w->caretPos == -1) { w->caretPos = w->text.length; }
		w->caretPos--;
		if (w->caretPos < 0) { w->caretPos = 0; }
		InputWidget_UpdateCaret(w);
	}
}

static void InputWidget_RightKey(struct InputWidget* w) {
	if (Key_IsActionPressed()) {
		w->caretPos += WordWrap_GetForwardLength(&w->text, w->caretPos);
		if (w->caretPos >= w->text.length) { w->caretPos = -1; }
		InputWidget_UpdateCaret(w);
		return;
	}

	if (w->text.length > 0 && w->caretPos != -1) {
		w->caretPos++;
		if (w->caretPos >= w->text.length) { w->caretPos = -1; }
		InputWidget_UpdateCaret(w);
	}
}

static void InputWidget_HomeKey(struct InputWidget* w) {
	if (!w->text.length) return;
	w->caretPos = 0;
	InputWidget_UpdateCaret(w);
}

static void InputWidget_EndKey(struct InputWidget* w) {
	w->caretPos = -1;
	InputWidget_UpdateCaret(w);
}

static void InputWidget_CopyFromClipboard(String* text, void* w) {
	InputWidget_AppendString((struct InputWidget*)w, text);
}

static bool InputWidget_OtherKey(struct InputWidget* w, Key key) {
	int maxChars = w->GetMaxLines() * INPUTWIDGET_LEN;
	if (!Key_IsActionPressed()) return false;

	if (key == 'V' && w->text.length < maxChars) {
		Clipboard_RequestText(InputWidget_CopyFromClipboard, w);
		return true;
	} else if (key == 'C') {
		if (!w->text.length) return true;
		Clipboard_SetText(&w->text);
		return true;
	}
	return false;
}

static void InputWidget_Init(void* widget) {
	struct InputWidget* w = (struct InputWidget*)widget;
	int lines = w->GetMaxLines();

	if (lines > 1) {
		WordWrap_Do(&w->text, w->lines, lines, INPUTWIDGET_LEN);
	} else {
		w->lines[0] = w->text;
	}

	InputWidget_CalculateLineSizes(w);
	w->RemakeTexture(w);
	InputWidget_UpdateCaret(w);
}

static void InputWidget_Free(void* widget) {
	struct InputWidget* w = (struct InputWidget*)widget;
	Gfx_DeleteTexture(&w->inputTex.ID);
	Gfx_DeleteTexture(&w->caretTex.ID);
}

static void InputWidget_Recreate(void* widget) {
	struct InputWidget* w = (struct InputWidget*)widget;
	Gfx_DeleteTexture(&w->inputTex.ID);
	InputWidget_Init(w);
}

static void InputWidget_Reposition(void* widget) {
	struct InputWidget* w = (struct InputWidget*)widget;
	int oldX = w->x, oldY = w->y;
	Widget_CalcPosition(w);
	
	w->caretTex.X += w->x - oldX; w->caretTex.Y += w->y - oldY;
	w->inputTex.X += w->x - oldX; w->inputTex.Y += w->y - oldY;
}

static bool InputWidget_KeyDown(void* widget, Key key, bool was) {
	struct InputWidget* w = (struct InputWidget*)widget;
	if (key == KEY_LEFT) {
		InputWidget_LeftKey(w);
	} else if (key == KEY_RIGHT) {
		InputWidget_RightKey(w);
	} else if (key == KEY_BACKSPACE) {
		InputWidget_BackspaceKey(w);
	} else if (key == KEY_DELETE) {
		InputWidget_DeleteKey(w);
	} else if (key == KEY_HOME) {
		InputWidget_HomeKey(w);
	} else if (key == KEY_END) {
		InputWidget_EndKey(w);
	} else if (!InputWidget_OtherKey(w, key)) {
		return false;
	}
	return true;
}

static bool InputWidget_KeyUp(void* widget, Key key) { return true; }

static bool InputWidget_KeyPress(void* widget, char keyChar) {
	struct InputWidget* w = (struct InputWidget*)widget;
	InputWidget_Append(w, keyChar);
	return true;
}

static bool InputWidget_MouseDown(void* widget, int x, int y, MouseButton button) {
	String line; char lineBuffer[STRING_SIZE];
	struct InputWidget* w = (struct InputWidget*)widget;
	struct DrawTextArgs args;
	int cx, cy, offset = 0;
	int charX, charWidth, charHeight;

	if (button != MOUSE_LEFT) return true;
	x -= w->inputTex.X; y -= w->inputTex.Y;

	DrawTextArgs_MakeEmpty(&args, &w->font, true);
	charHeight = w->caretTex.Height;
	String_InitArray(line, lineBuffer);

	for (cy = 0; cy < w->GetMaxLines(); cy++) {
		line.length = 0;
		InputWidget_FormatLine(w, cy, &line);
		if (!line.length) continue;

		for (cx = 0; cx < line.length; cx++) {
			args.text = String_UNSAFE_Substring(&line, 0, cx);
			charX     = Drawer2D_TextWidth(&args);
			if (cy == 0) charX += w->prefixWidth;

			args.text = String_UNSAFE_Substring(&line, cx, 1);
			charWidth = Drawer2D_TextWidth(&args);

			if (Gui_Contains(charX, cy * charHeight, charWidth, charHeight, x, y)) {
				w->caretPos = offset + cx;
				InputWidget_UpdateCaret(w);
				return true;
			}
		}
		offset += line.length;
	}

	w->caretPos = -1;
	InputWidget_UpdateCaret(w);
	return true;
}

CC_NOINLINE static void InputWidget_Create(struct InputWidget* w, const FontDesc* font, STRING_REF const String* prefix) {
	static const String caret = String_FromConst("_");
	struct DrawTextArgs args;
	Size2D size;
	Widget_Reset(w);

	w->font           = *font;
	w->prefix         = *prefix;
	w->caretPos       = -1;
	w->OnPressedEnter = InputWidget_OnPressedEnter;
	w->AllowedChar    = InputWidget_AllowedChar;
	
	DrawTextArgs_Make(&args, &caret, font, true);
	Drawer2D_MakeTextTexture(&w->caretTex, &args, 0, 0);
	w->caretTex.Width = (uint16_t)((w->caretTex.Width * 3) / 4);
	w->caretWidth     = w->caretTex.Width;

	if (!prefix->length) return;
	DrawTextArgs_Make(&args, prefix, font, true);
	size = Drawer2D_MeasureText(&args);
	w->prefixWidth  = size.Width;  w->width  = size.Width;
	w->prefixHeight = size.Height; w->height = size.Height;
}


/*########################################################################################################################*
*---------------------------------------------------InputValidator----------------------------------------------------*
*#########################################################################################################################*/
static void Hex_Range(struct InputValidator* v, String* range) {
	String_AppendConst(range, "&7(#000000 - #FFFFFF)");
}

static bool Hex_ValidChar(struct InputValidator* v, char c) {
	return (c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f');
}

static bool Hex_ValidString(struct InputValidator* v, const String* s) {
	return s->length <= 6;
}

static bool Hex_ValidValue(struct InputValidator* v, const String* s) {
	PackedCol col;
	return PackedCol_TryParseHex(s, &col);
}

struct InputValidatorVTABLE HexValidator_VTABLE = {
	Hex_Range, Hex_ValidChar, Hex_ValidString, Hex_ValidValue,
};

static void Int_Range(struct InputValidator* v, String* range) {
	String_Format2(range, "&7(%i - %i)", &v->Meta.i.Min, &v->Meta.i.Max);
}

static bool Int_ValidChar(struct InputValidator* v, char c) {
	return (c >= '0' && c <= '9') || c == '-';
}

static bool Int_ValidString(struct InputValidator* v, const String* s) {
	int value;
	if (s->length == 1 && s->buffer[0] == '-') return true; /* input is just a minus sign */
	return Convert_ParseInt(s, &value);
}

static bool Int_ValidValue(struct InputValidator* v, const String* s) {
	int value, min = v->Meta.i.Min, max = v->Meta.i.Max;
	return Convert_ParseInt(s, &value) && min <= value && value <= max;
}

struct InputValidatorVTABLE IntValidator_VTABLE = {
	Int_Range, Int_ValidChar, Int_ValidString, Int_ValidValue,
};

static void Seed_Range(struct InputValidator* v, String* range) {
	String_AppendConst(range, "&7(an integer)");
}

struct InputValidatorVTABLE SeedValidator_VTABLE = {
	Seed_Range, Int_ValidChar, Int_ValidString, Int_ValidValue,
};

static void Float_Range(struct InputValidator* v, String* range) {
	String_Format2(range, "&7(%f2 - %f2)", &v->Meta.f.Min, &v->Meta.f.Max);
}

static bool Float_ValidChar(struct InputValidator* v, char c) {
	return (c >= '0' && c <= '9') || c == '-' || c == '.' || c == ',';
}

static bool Float_ValidString(struct InputValidator* v, const String* s) {
	float value;
	if (s->length == 1 && Float_ValidChar(v, s->buffer[0])) return true;
	return Convert_ParseFloat(s, &value);
}

static bool Float_ValidValue(struct InputValidator* v, const String* s) {
	float value, min = v->Meta.f.Min, max = v->Meta.f.Max;
	return Convert_ParseFloat(s, &value) && min <= value && value <= max;
}

struct InputValidatorVTABLE FloatValidator_VTABLE = {
	Float_Range, Float_ValidChar, Float_ValidString, Float_ValidValue,
};

static void Path_Range(struct InputValidator* v, String* range) {
	String_AppendConst(range, "&7(Enter name)");
}

static bool Path_ValidChar(struct InputValidator* v, char c) {
	return !(c == '/' || c == '\\' || c == '?' || c == '*' || c == ':'
		|| c == '<' || c == '>' || c == '|' || c == '"' || c == '.');
}
static bool Path_ValidString(struct InputValidator* v, const String* s) { return true; }

struct InputValidatorVTABLE PathValidator_VTABLE = {
	Path_Range, Path_ValidChar, Path_ValidString, Path_ValidString,
};

static void String_Range(struct InputValidator* v, String* range) {
	String_AppendConst(range, "&7(Enter text)");
}

static bool String_ValidChar(struct InputValidator* v, char c) {
	return c != '&';
}

static bool String_ValidString(struct InputValidator* v, const String* s) {
	return s->length <= STRING_SIZE;
}

struct InputValidatorVTABLE StringValidator_VTABLE = {
	String_Range, String_ValidChar, String_ValidString, String_ValidString,
};


/*########################################################################################################################*
*-----------------------------------------------------MenuInputWidget-----------------------------------------------------*
*#########################################################################################################################*/
static void MenuInputWidget_Render(void* widget, double delta) {
	struct InputWidget* w = (struct InputWidget*)widget;
	PackedCol backCol = PACKEDCOL_CONST(30, 30, 30, 200);

	Gfx_SetTexturing(false);
	Gfx_Draw2DFlat(w->x, w->y, w->width, w->height, backCol);
	Gfx_SetTexturing(true);

	Texture_Render(&w->inputTex);
	InputWidget_RenderCaret(w, delta);
}

static void MenuInputWidget_RemakeTexture(void* widget) {
	String range; char rangeBuffer[STRING_SIZE];
	struct MenuInputWidget* w = (struct MenuInputWidget*)widget;
	struct InputValidator* v;
	struct DrawTextArgs args;
	struct Texture* tex;
	Size2D size, adjSize;
	int hintX, hintWidth;
	Bitmap bmp;

	DrawTextArgs_Make(&args, &w->base.lines[0], &w->base.font, false);
	size.Width   = Drawer2D_TextWidth(&args);
	/* Text may be empty, but don't want 0 height if so */
	size.Height  = Drawer2D_FontHeight(&w->base.font, false);
	w->base.caretAccumulator = 0.0;

	String_InitArray(range, rangeBuffer);
	v = &w->validator;
	v->VTABLE->GetRange(v, &range);

	w->base.width  = max(size.Width,  w->minWidth);
	w->base.height = max(size.Height, w->minHeight);
	adjSize = size; adjSize.Width = w->base.width;

	Bitmap_AllocateClearedPow2(&bmp, adjSize.Width, adjSize.Height);
	{
		Drawer2D_DrawText(&bmp, &args, w->base.padding, 0);

		args.text = range;
		hintWidth = Drawer2D_MeasureText(&args).Width;
		hintX     = adjSize.Width - hintWidth;
		if (size.Width + 3 < hintX) {
			Drawer2D_DrawText(&bmp, &args, hintX, 0);
		}
	}

	tex = &w->base.inputTex;
	Drawer2D_Make2DTexture(tex, &bmp, adjSize, 0, 0);
	Mem_Free(bmp.Scan0);

	Widget_Reposition(&w->base);
	tex->X = w->base.x; tex->Y = w->base.y;
	if (size.Height < w->minHeight) {
		tex->Y += w->minHeight / 2 - size.Height / 2;
	}
}

static bool MenuInputWidget_AllowedChar(void* widget, char c) {
	struct InputWidget* w = (struct InputWidget*)widget;
	struct InputValidator* v;
	int maxChars;
	bool valid;

	if (c == '&') return false;
	v = &((struct MenuInputWidget*)w)->validator;

	if (!v->VTABLE->IsValidChar(v, c)) return false;
	maxChars = w->GetMaxLines() * INPUTWIDGET_LEN;
	if (w->text.length == maxChars) return false;

	/* See if the new string is in valid format */
	InputWidget_AppendChar(w, c);
	valid = v->VTABLE->IsValidString(v, &w->text);
	InputWidget_DeleteChar(w);
	return valid;
}

static int MenuInputWidget_GetMaxLines(void) { return 1; }
static struct WidgetVTABLE MenuInputWidget_VTABLE = {
	InputWidget_Init,      MenuInputWidget_Render, InputWidget_Free,     InputWidget_Recreate,
	InputWidget_KeyDown,   InputWidget_KeyUp,      InputWidget_KeyPress,
	InputWidget_MouseDown, Widget_Mouse,           Widget_MouseMove,     Widget_MouseScroll,
	InputWidget_Reposition,
};
void MenuInputWidget_Create(struct MenuInputWidget* w, int width, int height, const String* text, const FontDesc* font, struct InputValidator* validator) {
	InputWidget_Create(&w->base, font, &String_Empty);
	w->base.VTABLE = &MenuInputWidget_VTABLE;

	w->minWidth  = width;
	w->minHeight = height;
	w->validator = *validator;

	w->base.convertPercents = false;
	w->base.padding = 3;
	String_InitArray(w->base.text, w->_textBuffer);

	w->base.GetMaxLines   = MenuInputWidget_GetMaxLines;
	w->base.RemakeTexture = MenuInputWidget_RemakeTexture;
	w->base.AllowedChar   = MenuInputWidget_AllowedChar;

	Elem_Init(&w->base);
	InputWidget_AppendString(&w->base, text);
}


/*########################################################################################################################*
*-----------------------------------------------------ChatInputWidget-----------------------------------------------------*
*#########################################################################################################################*/
static void ChatInputWidget_RemakeTexture(void* widget) {
	String line; char lineBuffer[STRING_SIZE + 2];
	struct InputWidget* w = (struct InputWidget*)widget;
	struct DrawTextArgs args;
	Size2D size = { 0, 0 };
	Bitmap bmp; 
	char lastCol;
	int i, x, y = 0;

	for (i = 0; i < w->GetMaxLines(); i++) {
		size.Height += w->lineSizes[i].Height;
		size.Width   = max(size.Width, w->lineSizes[i].Width);
	}
	Bitmap_AllocateClearedPow2(&bmp, size.Width, size.Height);

	DrawTextArgs_MakeEmpty(&args, &w->font, true);
	if (w->prefix.length) {
		args.text = w->prefix;
		Drawer2D_DrawText(&bmp, &args, 0, 0);
	}

	String_InitArray(line, lineBuffer);
	for (i = 0; i < Array_Elems(w->lines); i++) {
		if (!w->lines[i].length) break;
		line.length = 0;

		/* Colour code continues in next line */
		lastCol = InputWidget_GetLastCol(w, 0, i);
		if (!Drawer2D_IsWhiteCol(lastCol)) {
			String_Append(&line, '&'); String_Append(&line, lastCol);
		}
		/* Convert % to & for colour codes */
		InputWidget_FormatLine(w, i, &line);
		args.text = line;

		x = i == 0 ? w->prefixWidth : 0;
		Drawer2D_DrawText(&bmp, &args, x, y);
		y += w->lineSizes[i].Height;
	}

	Drawer2D_Make2DTexture(&w->inputTex, &bmp, size, 0, 0);
	Mem_Free(bmp.Scan0);
	w->caretAccumulator = 0;

	w->width  = size.Width;
	w->height = y == 0 ? w->prefixHeight : y;
	Widget_Reposition(w);
	w->inputTex.X = w->x + w->padding;
	w->inputTex.Y = w->y;
}

static void ChatInputWidget_Render(void* widget, double delta) {
	PackedCol backCol = PACKEDCOL_CONST(0, 0, 0, 127);
	struct InputWidget* w = (struct InputWidget*)widget;
	int x = w->x, y = w->y;
	bool caretAtEnd;
	int i, width;

	Gfx_SetTexturing(false);
	for (i = 0; i < INPUTWIDGET_MAX_LINES; i++) {
		if (i > 0 && !w->lineSizes[i].Height) break;

		caretAtEnd = (w->caretY == i) && (w->caretX == INPUTWIDGET_LEN || w->caretPos == -1);
		width      = w->lineSizes[i].Width + (caretAtEnd ? w->caretTex.Width : 0);
		/* Cover whole window width to match original classic behaviour */
		if (Game_PureClassic) { width = max(width, Window_Width - x * 4); }
	
		Gfx_Draw2DFlat(x, y, width + w->padding * 2, w->prefixHeight, backCol);
		y += w->lineSizes[i].Height;
	}

	Gfx_SetTexturing(true);
	Texture_Render(&w->inputTex);
	InputWidget_RenderCaret(w, delta);
}

static void ChatInputWidget_OnPressedEnter(void* widget) {
	struct ChatInputWidget* w = (struct ChatInputWidget*)widget;
	/* Don't want trailing spaces in output message */
	String text = w->base.text;
	String_UNSAFE_TrimEnd(&text);
	if (text.length) { Chat_Send(&text, true); }

	w->origStr.length = 0;
	w->typingLogPos = Chat_InputLog.count; /* Index of newest entry + 1. */

	Chat_AddOf(&String_Empty, MSG_TYPE_CLIENTSTATUS_2);
	InputWidget_OnPressedEnter(widget);
}

static void ChatInputWidget_UpKey(struct InputWidget* w) {
	struct ChatInputWidget* W = (struct ChatInputWidget*)w;
	String prevInput;
	int pos;

	if (Key_IsActionPressed()) {
		pos = w->caretPos == -1 ? w->text.length : w->caretPos;
		if (pos < INPUTWIDGET_LEN) return;

		w->caretPos = pos - INPUTWIDGET_LEN;
		InputWidget_UpdateCaret(w);
		return;
	}

	if (W->typingLogPos == Chat_InputLog.count) {
		String_Copy(&W->origStr, &w->text);
	}

	if (!Chat_InputLog.count) return;
	W->typingLogPos--;
	w->text.length = 0;

	if (W->typingLogPos < 0) W->typingLogPos = 0;
	prevInput = StringsBuffer_UNSAFE_Get(&Chat_InputLog, W->typingLogPos);
	String_AppendString(&w->text, &prevInput);

	w->caretPos = -1;
	Elem_Recreate(w);
}

static void ChatInputWidget_DownKey(struct InputWidget* w) {
	struct ChatInputWidget* W = (struct ChatInputWidget*)w;
	String prevInput;

	if (Key_IsActionPressed()) {
		if (w->caretPos == -1) return;

		w->caretPos += INPUTWIDGET_LEN;
		if (w->caretPos >= w->text.length) { w->caretPos = -1; }
		InputWidget_UpdateCaret(w);
		return;
	}

	if (!Chat_InputLog.count) return;
	W->typingLogPos++;
	w->text.length = 0;

	if (W->typingLogPos >= Chat_InputLog.count) {
		W->typingLogPos = Chat_InputLog.count;
		String_AppendString(&w->text, &W->origStr);
	} else {
		prevInput = StringsBuffer_UNSAFE_Get(&Chat_InputLog, W->typingLogPos);
		String_AppendString(&w->text, &prevInput);
	}

	w->caretPos = -1;
	Elem_Recreate(w);
}

static bool ChatInputWidget_IsNameChar(char c) {
	return c == '_' || c == '.' || (c >= '0' && c <= '9')
		|| (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

static void ChatInputWidget_TabKey(struct InputWidget* w) {
	String str; char strBuffer[STRING_SIZE];
	EntityID matches[TABLIST_MAX_NAMES];
	String part, name;
	int beg, end, len;
	int i, j, numMatches;
	char* buffer;

	end = w->caretPos == -1 ? w->text.length - 1 : w->caretPos;
	beg = end;
	buffer = w->text.buffer;

	/* e.g. if player typed "hi Nam", backtrack to "N" */
	while (beg >= 0 && ChatInputWidget_IsNameChar(buffer[beg])) beg--;
	beg++;
	if (end < 0 || beg > end) return;

	part = String_UNSAFE_Substring(&w->text, beg, (end + 1) - beg);
	Chat_AddOf(&String_Empty, MSG_TYPE_CLIENTSTATUS_2);
	numMatches = 0;

	for (i = 0; i < TABLIST_MAX_NAMES; i++) {
		if (!TabList.NameOffsets[i]) continue;

		name = TabList_UNSAFE_GetPlayer(i);
		if (!String_CaselessContains(&name, &part)) continue;
		matches[numMatches++] = (EntityID)i;
	}

	if (numMatches == 1) {
		if (w->caretPos == -1) end++;
		len = end - beg;

		/* Following on from above example, delete 'N','a','m' */
		/* Then insert the e.g. matching 'nAME1' player name */
		for (j = 0; j < len; j++) {
			String_DeleteAt(&w->text, beg);
		}

		if (w->caretPos != -1) w->caretPos -= len;
		name = TabList_UNSAFE_GetPlayer(matches[0]);
		InputWidget_AppendString(w, &name);
	} else if (numMatches > 1) {
		String_InitArray(str, strBuffer);
		String_Format1(&str, "&e%i matching names: ", &numMatches);

		for (i = 0; i < numMatches; i++) {
			name = TabList_UNSAFE_GetPlayer(matches[i]);
			if ((str.length + name.length + 1) > STRING_SIZE) break;

			String_AppendString(&str, &name);
			String_Append(&str, ' ');
		}
		Chat_AddOf(&str, MSG_TYPE_CLIENTSTATUS_2);
	}
}

static bool ChatInputWidget_KeyDown(void* widget, Key key, bool was) {
	struct InputWidget* w = (struct InputWidget*)widget;
	if (key == KEY_TAB)  { ChatInputWidget_TabKey(w);  return true; }
	if (key == KEY_UP)   { ChatInputWidget_UpKey(w);   return true; }
	if (key == KEY_DOWN) { ChatInputWidget_DownKey(w); return true; }
	return InputWidget_KeyDown(w, key, was);
}

static int ChatInputWidget_GetMaxLines(void) {
	return !Game_ClassicMode && Server.SupportsPartialMessages ? INPUTWIDGET_MAX_LINES : 1;
}

static struct WidgetVTABLE ChatInputWidget_VTABLE = {
	InputWidget_Init,        ChatInputWidget_Render, InputWidget_Free,     InputWidget_Recreate,
	ChatInputWidget_KeyDown, InputWidget_KeyUp,      InputWidget_KeyPress,
	InputWidget_MouseDown,   Widget_Mouse,           Widget_MouseMove,     Widget_MouseScroll,
	InputWidget_Reposition,
};
void ChatInputWidget_Create(struct ChatInputWidget* w, const FontDesc* font) {
	static const String prefix = String_FromConst("> ");

	InputWidget_Create(&w->base, font, &prefix);
	w->typingLogPos = Chat_InputLog.count; /* Index of newest entry + 1. */
	w->base.VTABLE  = &ChatInputWidget_VTABLE;

	w->base.convertPercents = !Game_ClassicMode;
	w->base.showCaret       = true;
	w->base.padding         = 5;
	w->base.GetMaxLines    = ChatInputWidget_GetMaxLines;
	w->base.RemakeTexture  = ChatInputWidget_RemakeTexture;
	w->base.OnPressedEnter = ChatInputWidget_OnPressedEnter;

	String_InitArray(w->base.text, w->_textBuffer);
	String_InitArray(w->origStr,   w->_origBuffer);
}	


/*########################################################################################################################*
*----------------------------------------------------PlayerListWidget-----------------------------------------------------*
*#########################################################################################################################*/
#define GROUP_NAME_ID UInt16_MaxValue
#define LIST_COLUMN_PADDING 5
#define LIST_BOUNDS_SIZE 10
#define LIST_NAMES_PER_COLUMN 16

static void PlayerListWidget_DrawName(struct Texture* tex, struct PlayerListWidget* w, const String* name) {
	String tmp; char tmpBuffer[STRING_SIZE];
	struct DrawTextArgs args;

	if (Game_PureClassic) {
		String_InitArray(tmp, tmpBuffer);
		String_AppendColorless(&tmp, name);
	} else {
		tmp = *name;
	}

	DrawTextArgs_Make(&args, &tmp, &w->font, !w->classic);
	Drawer2D_MakeTextTexture(tex, &args, 0, 0);
	Drawer2D_ReducePadding_Tex(tex, w->font.Size, 3);
}

static int PlayerListWidget_HighlightedName(struct PlayerListWidget* w, int x, int y) {
	struct Texture tex;
	int i;
	if (!w->active) return -1;
	
	for (i = 0; i < w->namesCount; i++) {
		if (!w->textures[i].ID || w->ids[i] == GROUP_NAME_ID) continue;

		tex = w->textures[i];
		if (Gui_Contains(tex.X, tex.Y, tex.Width, tex.Height, x, y)) return i;
	}
	return -1;
}

void PlayerListWidget_GetNameUnder(struct PlayerListWidget* w, int x, int y, String* name) {
	String player;
	int i = PlayerListWidget_HighlightedName(w, x, y);
	if (i == -1) return;

	player = TabList_UNSAFE_GetPlayer(w->ids[i]);
	String_AppendString(name, &player);
}

static void PlayerListWidget_UpdateTableDimensions(struct PlayerListWidget* w) {
	int width = w->xMax - w->xMin, height = w->yHeight;
	w->x = (w->xMin                       ) - LIST_BOUNDS_SIZE;
	w->y = (Window_Height / 2 - height / 2) - LIST_BOUNDS_SIZE;
	w->width  = width  + LIST_BOUNDS_SIZE * 2;
	w->height = height + LIST_BOUNDS_SIZE * 2;
}

static int PlayerListWidget_GetColumnWidth(struct PlayerListWidget* w, int column) {
	int i   = column * LIST_NAMES_PER_COLUMN;
	int end = min(w->namesCount, i + LIST_NAMES_PER_COLUMN);
	int maxWidth = 0;

	for (; i < end; i++) {
		maxWidth = max(maxWidth, w->textures[i].Width);
	}
	return maxWidth + LIST_COLUMN_PADDING + w->elementOffset;
}

static int PlayerListWidget_GetColumnHeight(struct PlayerListWidget* w, int column) {
	int i   = column * LIST_NAMES_PER_COLUMN;
	int end = min(w->namesCount, i + LIST_NAMES_PER_COLUMN);
	int height = 0;

	for (; i < end; i++) {
		height += w->textures[i].Height + 1;
	}
	return height;
}

static void PlayerListWidget_SetColumnPos(struct PlayerListWidget* w, int column, int x, int y) {
	struct Texture tex;
	int i   = column * LIST_NAMES_PER_COLUMN;
	int end = min(w->namesCount, i + LIST_NAMES_PER_COLUMN);

	for (; i < end; i++) {
		tex = w->textures[i];
		tex.X = x; tex.Y = y - 10;

		y += tex.Height + 1;
		/* offset player names a bit, compared to group name */
		if (!w->classic && w->ids[i] != GROUP_NAME_ID) {
			tex.X += w->elementOffset;
		}
		w->textures[i] = tex;
	}
}

static void PlayerListWidget_RepositionColumns(struct PlayerListWidget* w) {
	int x, y, width = 0, height;
	int col, columns;

	w->yHeight = 0;
	columns    = Math_CeilDiv(w->namesCount, LIST_NAMES_PER_COLUMN);

	for (col = 0; col < columns; col++) {
		width += PlayerListWidget_GetColumnWidth(w, col);
		height = PlayerListWidget_GetColumnHeight(w, col);
		w->yHeight = max(height, w->yHeight);
	}

	if (width < 480) width = 480;
	w->xMin = Window_Width / 2 - width / 2;
	w->xMax = Window_Width / 2 + width / 2;

	x = w->xMin;
	y = Window_Height / 2 - w->yHeight / 2;

	for (col = 0; col < columns; col++) {
		PlayerListWidget_SetColumnPos(w, col, x, y);
		x += PlayerListWidget_GetColumnWidth(w, col);
	}
}

static void PlayerListWidget_Reposition(void* widget) {
	struct PlayerListWidget* w = (struct PlayerListWidget*)widget;
	int i, y, oldX, oldY;

	y = Window_Height / 4 - w->height / 2;
	w->yOffset = -max(0, y);

	oldX = w->x; oldY = w->y;
	Widget_CalcPosition(w);	

	for (i = 0; i < w->namesCount; i++) {
		w->textures[i].X += w->x - oldX;
		w->textures[i].Y += w->y - oldY;
	}
}

static void PlayerListWidget_AddName(struct PlayerListWidget* w, EntityID id, int index) {
	String name;
	/* insert at end of list */
	if (index == -1) { index = w->namesCount; w->namesCount++; }

	name = TabList_UNSAFE_GetList(id);
	w->ids[index] = id;
	PlayerListWidget_DrawName(&w->textures[index], w, &name);
}

static void PlayerListWidget_DeleteAt(struct PlayerListWidget* w, int i) {
	Gfx_DeleteTexture(&w->textures[i].ID);

	for (; i < w->namesCount - 1; i++) {
		w->ids[i]      = w->ids[i + 1];
		w->textures[i] = w->textures[i + 1];
	}

	w->namesCount--;
	w->ids[w->namesCount]         = 0;
	w->textures[w->namesCount].ID = GFX_NULL;
}

static void PlayerListWidget_AddGroup(struct PlayerListWidget* w, int id, int* index) {
	String group;
	int i;
	group = TabList_UNSAFE_GetGroup(id);

	for (i = Array_Elems(w->ids) - 1; i > (*index); i--) {
		w->ids[i]      = w->ids[i - 1];
		w->textures[i] = w->textures[i - 1];
	}
	
	w->ids[*index] = GROUP_NAME_ID;
	PlayerListWidget_DrawName(&w->textures[*index], w, &group);

	(*index)++;
	w->namesCount++;
}

static int PlayerListWidget_GetGroupCount(struct PlayerListWidget* w, int id, int i) {
	String group, curGroup;
	int count;
	group = TabList_UNSAFE_GetGroup(id);

	for (count = 0; i < w->namesCount; i++, count++) {
		curGroup = TabList_UNSAFE_GetGroup(w->ids[i]);
		if (!String_CaselessEquals(&group, &curGroup)) break;
	}
	return count;
}

static int PlayerListWidget_PlayerCompare(int x, int y) {
	String xName; char xNameBuffer[STRING_SIZE];
	String yName; char yNameBuffer[STRING_SIZE];
	uint8_t xRank, yRank;
	String xNameRaw, yNameRaw;

	xRank = TabList.GroupRanks[x];
	yRank = TabList.GroupRanks[y];
	if (xRank != yRank) return (xRank < yRank ? -1 : 1);
	
	String_InitArray(xName, xNameBuffer);
	xNameRaw = TabList_UNSAFE_GetList(x);
	String_AppendColorless(&xName, &xNameRaw);

	String_InitArray(yName, yNameBuffer);
	yNameRaw = TabList_UNSAFE_GetList(y);
	String_AppendColorless(&yName, &yNameRaw);

	return String_Compare(&xName, &yName);
}

static int PlayerListWidget_GroupCompare(int x, int y) {
	String xGroup, yGroup;
	/* TODO: should we use colourless comparison? ClassicalSharp sorts groups with colours */
	xGroup = TabList_UNSAFE_GetGroup(x);
	yGroup = TabList_UNSAFE_GetGroup(y);
	return String_Compare(&xGroup, &yGroup);
}

struct PlayerListWidget* List_SortObj;
int (*List_SortCompare)(int x, int y);
static void PlayerListWidget_QuickSort(int left, int right) {
	struct Texture* values = List_SortObj->textures; struct Texture value;
	uint16_t* keys = List_SortObj->ids; uint16_t key;

	while (left < right) {
		int i = left, j = right;
		int pivot = keys[(i + j) / 2];

		/* partition the list */
		while (i <= j) {
			while (List_SortCompare(pivot, keys[i]) > 0) i++;
			while (List_SortCompare(pivot, keys[j]) < 0) j--;
			QuickSort_Swap_KV_Maybe();
		}
		/* recurse into the smaller subset */
		QuickSort_Recurse(PlayerListWidget_QuickSort)
	}
}

static void PlayerListWidget_SortEntries(struct PlayerListWidget* w) {
	int i, id, count;
	if (!w->namesCount) return;

	List_SortObj = w;
	if (w->classic) {
		List_SortCompare = PlayerListWidget_PlayerCompare;
		PlayerListWidget_QuickSort(0, w->namesCount - 1);
		return;
	}

	/* Sort the list by group */
	/* Loop backwards, since DeleteAt() reduces NamesCount */
	for (i = w->namesCount - 1; i >= 0; i--) {
		if (w->ids[i] != GROUP_NAME_ID) continue;
		PlayerListWidget_DeleteAt(w, i);
	}
	List_SortCompare = PlayerListWidget_GroupCompare;
	PlayerListWidget_QuickSort(0, w->namesCount - 1);

	/* Sort the entries in each group */
	List_SortCompare = PlayerListWidget_PlayerCompare;
	for (i = 0; i < w->namesCount; ) {
		id = w->ids[i];
		PlayerListWidget_AddGroup(w, id, &i);

		count = PlayerListWidget_GetGroupCount(w, id, i);
		PlayerListWidget_QuickSort(i, i + (count - 1));
		i += count;
	}
}

static void PlayerListWidget_SortAndReposition(struct PlayerListWidget* w) {
	PlayerListWidget_SortEntries(w);
	PlayerListWidget_RepositionColumns(w);
	PlayerListWidget_UpdateTableDimensions(w);
	PlayerListWidget_Reposition(w);
}

static void PlayerListWidget_TabEntryAdded(void* widget, int id) {
	struct PlayerListWidget* w = (struct PlayerListWidget*)widget;
	PlayerListWidget_AddName(w, id, -1);
	PlayerListWidget_SortAndReposition(w);
}

static void PlayerListWidget_TabEntryChanged(void* widget, int id) {
	struct PlayerListWidget* w = (struct PlayerListWidget*)widget;
	struct Texture tex;
	int i;

	for (i = 0; i < w->namesCount; i++) {
		if (w->ids[i] != id) continue;
		tex = w->textures[i];

		Gfx_DeleteTexture(&tex.ID);
		PlayerListWidget_AddName(w, id, i);
		PlayerListWidget_SortAndReposition(w);
		return;
	}
}

static void PlayerListWidget_TabEntryRemoved(void* widget, int id) {
	struct PlayerListWidget* w = (struct PlayerListWidget*)widget;
	int i;
	for (i = 0; i < w->namesCount; i++) {
		if (w->ids[i] != id) continue;

		PlayerListWidget_DeleteAt(w, i);
		PlayerListWidget_SortAndReposition(w);
		return;
	}
}

static void PlayerListWidget_Init(void* widget) {
	static const String title = String_FromConst("Connected players:");
	struct PlayerListWidget* w = (struct PlayerListWidget*)widget;
	int id;

	for (id = 0; id < TABLIST_MAX_NAMES; id++) {
		if (!TabList.NameOffsets[id]) continue;
		PlayerListWidget_AddName(w, (EntityID)id, -1);
	}

	PlayerListWidget_SortAndReposition(w); 
	TextWidget_Create(&w->title, &title, &w->font);
	Widget_SetLocation(&w->title, ANCHOR_CENTRE, ANCHOR_MIN, 0, 0);

	Event_RegisterInt(&TabListEvents.Added,   w, PlayerListWidget_TabEntryAdded);
	Event_RegisterInt(&TabListEvents.Changed, w, PlayerListWidget_TabEntryChanged);
	Event_RegisterInt(&TabListEvents.Removed, w, PlayerListWidget_TabEntryRemoved);
}

static void PlayerListWidget_Render(void* widget, double delta) {
	struct PlayerListWidget* w = (struct PlayerListWidget*)widget;
	struct TextWidget* title = &w->title;
	struct Texture tex;
	int offset, height;
	int i, selectedI;
	PackedCol topCol    = PACKEDCOL_CONST( 0,  0,  0, 180);
	PackedCol bottomCol = PACKEDCOL_CONST(50, 50, 50, 205);

	Gfx_SetTexturing(false);
	offset = title->height + 10;
	height = max(300, w->height + title->height);
	Gfx_Draw2DGradient(w->x, w->y - offset, w->width, height, topCol, bottomCol);

	Gfx_SetTexturing(true);
	title->yOffset = w->y - offset + 5;
	Widget_Reposition(title);
	Elem_Render(title, delta);

	selectedI = PlayerListWidget_HighlightedName(w, Mouse_X, Mouse_Y);
	for (i = 0; i < w->namesCount; i++) {
		if (!w->textures[i].ID) continue;

		tex = w->textures[i];
		if (i == selectedI) tex.X += 4;
		Texture_Render(&tex);
	}
}

static void PlayerListWidget_Free(void* widget) {
	struct PlayerListWidget* w = (struct PlayerListWidget*)widget;
	int i;
	for (i = 0; i < w->namesCount; i++) {
		Gfx_DeleteTexture(&w->textures[i].ID);
	}

	Elem_TryFree(&w->title);
	Event_UnregisterInt(&TabListEvents.Added,   w, PlayerListWidget_TabEntryAdded);
	Event_UnregisterInt(&TabListEvents.Changed, w, PlayerListWidget_TabEntryChanged);
	Event_UnregisterInt(&TabListEvents.Removed, w, PlayerListWidget_TabEntryRemoved);
}

static struct WidgetVTABLE PlayerListWidget_VTABLE = {
	PlayerListWidget_Init, PlayerListWidget_Render, PlayerListWidget_Free, Gui_DefaultRecreate,
	Widget_KeyDown,	       Widget_KeyUp,            Widget_KeyPress,
	Widget_Mouse,          Widget_Mouse,            Widget_MouseMove,      Widget_MouseScroll,
	PlayerListWidget_Reposition,
};
void PlayerListWidget_Create(struct PlayerListWidget* w, const FontDesc* font, bool classic) {
	Widget_Reset(w);
	w->VTABLE     = &PlayerListWidget_VTABLE;
	w->horAnchor  = ANCHOR_CENTRE;
	w->verAnchor  = ANCHOR_CENTRE;

	w->namesCount = 0;
	w->font       = *font;
	w->classic    = classic;
	w->elementOffset = classic ? 0 : 10;
}


/*########################################################################################################################*
*-----------------------------------------------------TextGroupWidget-----------------------------------------------------*
*#########################################################################################################################*/
void TextGroupWidget_ShiftUp(struct TextGroupWidget* w) {
	int last, i;
	Gfx_DeleteTexture(&w->textures[0].ID);
	last = w->lines - 1;

	for (i = 0; i < last; i++) {
		w->textures[i] = w->textures[i + 1];
	}
	w->textures[last].ID = GFX_NULL; /* Delete() called by TextGroupWidget_Redraw otherwise */
	TextGroupWidget_Redraw(w, last);
}

void TextGroupWidget_ShiftDown(struct TextGroupWidget* w) {
	int last, i;
	last = w->lines - 1;
	Gfx_DeleteTexture(&w->textures[last].ID);

	for (i = last; i > 0; i--) {
		w->textures[i] = w->textures[i - 1];
	}
	w->textures[0].ID = GFX_NULL; /* Delete() called by TextGroupWidget_Redraw otherwise */
	TextGroupWidget_Redraw(w, 0);
}

static void TextGroupWidget_UpdateY(struct TextGroupWidget* w) {	
	struct Texture* textures = w->textures;
	int i, y;

	if (w->verAnchor == ANCHOR_MIN) {
		y = w->y;
		for (i = 0; i < w->lines; i++) {
			textures[i].Y = y;
			y += textures[i].Height;
		}
	} else {
		y = Window_Height - w->yOffset;
		for (i = w->lines - 1; i >= 0; i--) {
			y -= textures[i].Height;
			textures[i].Y = y;
		}
	}
}

void TextGroupWidget_SetUsePlaceHolder(struct TextGroupWidget* w, int index, bool placeHolder) {
	w->placeholderHeight[index] = placeHolder;
	if (w->textures[index].ID) return;

	w->textures[index].Height = placeHolder ? w->defaultHeight : 0;
	TextGroupWidget_UpdateY(w);
}

int TextGroupWidget_UsedHeight(struct TextGroupWidget* w) {
	struct Texture* textures = w->textures;
	int i, height = 0;

	for (i = 0; i < w->lines; i++) {
		if (textures[i].ID) break;
	}
	for (; i < w->lines; i++) {
		height += textures[i].Height;
	}
	return height;
}

static void TextGroupWidget_Reposition(void* widget) {
	struct TextGroupWidget* w = (struct TextGroupWidget*)widget;
	struct Texture* textures  = w->textures;
	int i, oldY = w->y;
	Widget_CalcPosition(w);

	for (i = 0; i < w->lines; i++) {
		textures[i].X = Gui_CalcPos(w->horAnchor, w->xOffset, textures[i].Width, Window_Width);
		textures[i].Y += w->y - oldY;
	}
}

static void TextGroupWidget_UpdateDimensions(struct TextGroupWidget* w) {	
	struct Texture* textures = w->textures;
	int i, width = 0, height = 0;

	for (i = 0; i < w->lines; i++) {
		width = max(width, textures[i].Width);
		height += textures[i].Height;
	}

	w->width  = width;
	w->height = height;
	Widget_Reposition(w);
}

struct Portion { int16_t Beg, Len, LineBeg, LineLen; };
#define TEXTGROUPWIDGET_HTTP_LEN 7 /* length of http:// */
#define TEXTGROUPWIDGET_URL 0x8000
#define TEXTGROUPWIDGET_PACKED_LEN 0x7FFF

static int TextGroupWidget_NextUrl(char* chars, int charsLen, int i) {
	int start, left;

	for (; i < charsLen; i++) {
		if (!(chars[i] == 'h' || chars[i] == '&')) continue;
		left = charsLen - i;
		if (left < TEXTGROUPWIDGET_HTTP_LEN) return charsLen;

		/* colour codes at start of URL */
		start = i;
		while (left >= 2 && chars[i] == '&') { left -= 2; i += 2; }
		if (left < TEXTGROUPWIDGET_HTTP_LEN) continue;

		/* Starts with "http" */
		if (chars[i] != 'h' || chars[i + 1] != 't' || chars[i + 2] != 't' || chars[i + 3] != 'p') continue;
		left -= 4; i += 4;

		/* And then with "s://" or "://" */
		if (chars[i] == 's') { left--; i++; }
		if (left >= 3 && chars[i] == ':' && chars[i + 1] == '/' && chars[i + 2] == '/') return start;
	}
	return charsLen;
}

static int TextGroupWidget_UrlEnd(char* chars, int charsLen, int32_t* begs, int begsLen, int i) {
	int start = i, j;
	int next, left;
	bool isBeg;

	for (; i < charsLen && chars[i] != ' '; i++) {
		/* Is this character the start of a line */
		isBeg = false;
		for (j = 0; j < begsLen; j++) {
			if (i == begs[j]) { isBeg = true; break; }
		}

		/* Definitely not a multilined URL */
		if (!isBeg || i == start) continue;
		if (chars[i] != '>') break;

		/* Does this line start with "> ", making it a multiline */
		next = i + 1; left = charsLen - next;
		while (left >= 2 && chars[next] == '&') { left -= 2; next += 2; }
		if (left == 0 || chars[next] != ' ') break;

		i = next;
	}
	return i;
}

static void TextGroupWidget_Output(struct Portion bit, int lineBeg, int lineEnd, struct Portion** portions) {
	struct Portion* cur;
	int overBy, underBy;
	if (bit.Beg >= lineEnd || !bit.Len) return;

	bit.LineBeg = bit.Beg;
	bit.LineLen = bit.Len & TEXTGROUPWIDGET_PACKED_LEN;

	/* Adjust this portion to be within this line */
	if (bit.Beg >= lineBeg) {
	} else if (bit.Beg + bit.LineLen > lineBeg) {
		/* Adjust start of portion to be within this line */
		underBy = lineBeg - bit.Beg;
		bit.LineBeg += underBy; bit.LineLen -= underBy;
	} else { return; }

	/* Limit length of portion to be within this line */
	overBy = (bit.LineBeg + bit.LineLen) - lineEnd;
	if (overBy > 0) bit.LineLen -= overBy;

	bit.LineBeg -= lineBeg;
	if (!bit.LineLen) return;

	cur = *portions; *cur++ = bit; *portions = cur;
}

static int TextGroupWidget_Reduce(struct TextGroupWidget* w, char* chars, int target, struct Portion* portions) {
	struct Portion* start = portions;	
	int32_t begs[TEXTGROUPWIDGET_MAX_LINES];
	int32_t ends[TEXTGROUPWIDGET_MAX_LINES];
	struct Portion bit;
	String line;
	int nextStart, i, total = 0, end;

	for (i = 0; i < w->lines; i++) {
		line = TextGroupWidget_UNSAFE_Get(w, i);
		begs[i] = -1; ends[i] = -1;
		if (!line.length) continue;

		begs[i] = total;
		Mem_Copy(&chars[total], line.buffer, line.length);
		total += line.length; ends[i] = total;
	}

	end = 0;
	for (;;) {
		nextStart = TextGroupWidget_NextUrl(chars, total, end);

		/* add normal portion between urls */
		bit.Beg = end;
		bit.Len = nextStart - end;
		TextGroupWidget_Output(bit, begs[target], ends[target], &portions);

		if (nextStart == total) break;
		end = TextGroupWidget_UrlEnd(chars, total, begs, w->lines, nextStart);

		/* add this url portion */
		bit.Beg = nextStart;
		bit.Len = (end - nextStart) | TEXTGROUPWIDGET_URL;
		TextGroupWidget_Output(bit, begs[target], ends[target], &portions);
	}
	return (int)(portions - start);
}

static void TextGroupWidget_FormatUrl(String* text, const String* url) {
	char* dst;
	int i;
	String_AppendColorless(text, url);

	/* Delete "> " multiline chars from URLs */
	dst = text->buffer;
	for (i = text->length - 2; i >= 0; i--) {
		if (dst[i] != '>' || dst[i + 1] != ' ') continue;

		String_DeleteAt(text, i + 1);
		String_DeleteAt(text, i);
	}
}

static bool TextGroupWidget_GetUrl(struct TextGroupWidget* w, String* text, int index, int mouseX) {
	char chars[TEXTGROUPWIDGET_MAX_LINES * TEXTGROUPWIDGET_LEN];
	struct Portion portions[2 * (TEXTGROUPWIDGET_LEN / TEXTGROUPWIDGET_HTTP_LEN)];
	struct Portion bit;
	struct DrawTextArgs args = { 0 };
	String line, url;
	int portionsCount;
	int i, x, width;

	mouseX -= w->textures[index].X;
	args.useShadow = true;
	line = TextGroupWidget_UNSAFE_Get(w, index);

	if (Game_ClassicMode) return false;
	portionsCount = TextGroupWidget_Reduce(w, chars, index, portions);

	for (i = 0, x = 0; i < portionsCount; i++) {
		bit = portions[i];
		args.text = String_UNSAFE_Substring(&line, bit.LineBeg, bit.LineLen);
		args.font = w->font;

		width = Drawer2D_TextWidth(&args);
		if ((bit.Len & TEXTGROUPWIDGET_URL) && mouseX >= x && mouseX < x + width) {
			bit.Len &= TEXTGROUPWIDGET_PACKED_LEN;
			url = String_Init(&chars[bit.Beg], bit.Len, bit.Len);

			TextGroupWidget_FormatUrl(text, &url);
			return true;
		}
		x += width;
	}
	return false;
}

void TextGroupWidget_GetSelected(struct TextGroupWidget* w, String* text, int x, int y) {
	struct Texture tex;
	String line;
	int i;

	for (i = 0; i < w->lines; i++) {
		if (!w->textures[i].ID) continue;
		tex = w->textures[i];
		if (!Gui_Contains(tex.X, tex.Y, tex.Width, tex.Height, x, y)) continue;

		if (!TextGroupWidget_GetUrl(w, text, i, x)) {
			line = TextGroupWidget_UNSAFE_Get(w, i);
			String_AppendString(text, &line);
		}
		return;
	}
}

static bool TextGroupWidget_MightHaveUrls(struct TextGroupWidget* w) {
	String line;
	int i;

	for (i = 0; i < w->lines; i++) {
		line = TextGroupWidget_UNSAFE_Get(w, i);
		if (String_IndexOf(&line, '/') >= 0) return true;
	}
	return false;
}

static void TextGroupWidget_DrawAdvanced(struct TextGroupWidget* w, struct Texture* tex, struct DrawTextArgs* args, int index, const String* text) {
	char chars[TEXTGROUPWIDGET_MAX_LINES * TEXTGROUPWIDGET_LEN];
	struct Portion portions[2 * (TEXTGROUPWIDGET_LEN / TEXTGROUPWIDGET_HTTP_LEN)];
	struct Portion bit;
	Size2D size = { 0, 0 };
	Size2D partSizes[Array_Elems(portions)];
	Bitmap bmp;
	int portionsCount;
	int i, x, ul;

	portionsCount = TextGroupWidget_Reduce(w, chars, index, portions);
	for (i = 0; i < portionsCount; i++) {
		bit = portions[i];
		args->text = String_UNSAFE_Substring(text, bit.LineBeg, bit.LineLen);

		partSizes[i] = Drawer2D_MeasureText(args);
		size.Height = max(partSizes[i].Height, size.Height);
		size.Width += partSizes[i].Width;
	}
	
	Bitmap_AllocateClearedPow2(&bmp, size.Width, size.Height);
	{
		x = 0;
		for (i = 0; i < portionsCount; i++) {
			bit = portions[i];
			ul  = (bit.Len & TEXTGROUPWIDGET_URL);
			args->text = String_UNSAFE_Substring(text, bit.LineBeg, bit.LineLen);

			if (ul) args->font.Style |= FONT_FLAG_UNDERLINE;
			Drawer2D_DrawText(&bmp, args, x, 0);
			if (ul) args->font.Style &= ~FONT_FLAG_UNDERLINE;

			x += partSizes[i].Width;
		}
		Drawer2D_Make2DTexture(tex, &bmp, size, 0, 0);
	}
	Mem_Free(bmp.Scan0);
}

void TextGroupWidget_RedrawAll(struct TextGroupWidget* w) {
	int i;
	for (i = 0; i < w->lines; i++) { TextGroupWidget_Redraw(w, i); }
}

void TextGroupWidget_Redraw(struct TextGroupWidget* w, int index) {
	String text;
	struct DrawTextArgs args;
	struct Texture tex = { 0 };
	Gfx_DeleteTexture(&w->textures[index].ID);

	text = TextGroupWidget_UNSAFE_Get(w, index);
	if (!Drawer2D_IsEmptyText(&text)) {
		DrawTextArgs_Make(&args, &text, &w->font, true);

		if (w->underlineUrls && TextGroupWidget_MightHaveUrls(w)) {
			TextGroupWidget_DrawAdvanced(w, &tex, &args, index, &text);
		} else {
			Drawer2D_MakeTextTexture(&tex, &args, 0, 0);
		}
		Drawer2D_ReducePadding_Tex(&tex, w->font.Size, 3);
	} else {
		tex.Height = w->placeholderHeight[index] ? w->defaultHeight : 0;
	}

	tex.X = Gui_CalcPos(w->horAnchor, w->xOffset, tex.Width, Window_Width);
	w->textures[index] = tex;
	TextGroupWidget_UpdateY(w);
	TextGroupWidget_UpdateDimensions(w);
}


static void TextGroupWidget_Init(void* widget) {
	struct TextGroupWidget* w = (struct TextGroupWidget*)widget;
	int i, height;
	
	height = Drawer2D_FontHeight(&w->font, true);
	Drawer2D_ReducePadding_Height(&height, w->font.Size, 3);
	w->defaultHeight = height;

	for (i = 0; i < w->lines; i++) {
		w->textures[i].Height = height;
		w->placeholderHeight[i] = true;
	}
	TextGroupWidget_UpdateDimensions(w);
}

static void TextGroupWidget_Render(void* widget, double delta) {
	struct TextGroupWidget* w = (struct TextGroupWidget*)widget;
	struct Texture* textures  = w->textures;
	int i;

	for (i = 0; i < w->lines; i++) {
		if (!textures[i].ID) continue;
		Texture_Render(&textures[i]);
	}
}

static void TextGroupWidget_Free(void* widget) {
	struct TextGroupWidget* w = (struct TextGroupWidget*)widget;
	int i;

	for (i = 0; i < w->lines; i++) {
		Gfx_DeleteTexture(&w->textures[i].ID);
	}
}

static struct WidgetVTABLE TextGroupWidget_VTABLE = {
	TextGroupWidget_Init, TextGroupWidget_Render, TextGroupWidget_Free, Gui_DefaultRecreate,
	Widget_KeyDown,	      Widget_KeyUp,           Widget_KeyPress,
	Widget_Mouse,         Widget_Mouse,           Widget_MouseMove,     Widget_MouseScroll,
	TextGroupWidget_Reposition,
};
void TextGroupWidget_Create(struct TextGroupWidget* w, int lines, const FontDesc* font, STRING_REF struct Texture* textures, TextGroupWidget_Get getLine) {
	Widget_Reset(w);
	w->VTABLE = &TextGroupWidget_VTABLE;

	w->lines    = lines;
	w->font     = *font;
	w->textures = textures;
	w->GetLine  = getLine;
}


/*########################################################################################################################*
*---------------------------------------------------SpecialInputWidget----------------------------------------------------*
*#########################################################################################################################*/
static void SpecialInputWidget_UpdateColString(struct SpecialInputWidget* w) {
	int i;
	String_InitArray(w->colString, w->_colBuffer);

	for (i = 0; i < DRAWER2D_MAX_COLS; i++) {
		if (i >= 'A' && i <= 'F') continue;
		if (!Drawer2D_Cols[i].A)  continue;

		String_Append(&w->colString, '&'); String_Append(&w->colString, (char)i);
		String_Append(&w->colString, '%'); String_Append(&w->colString, (char)i);
	}
}

static bool SpecialInputWidget_IntersectsTitle(struct SpecialInputWidget* w, int x, int y) {
	Size2D size;
	int i, titleX = 0;

	for (i = 0; i < Array_Elems(w->tabs); i++) {
		size = w->tabs[i].titleSize;
		if (Gui_Contains(titleX, 0, size.Width, size.Height, x, y)) {
			w->selectedIndex = i;
			return true;
		}
		titleX += size.Width;
	}
	return false;
}

static void SpecialInputWidget_IntersectsBody(struct SpecialInputWidget* w, int x, int y) {
	struct SpecialInputTab e = w->tabs[w->selectedIndex];
	String str;
	int i;

	y -= w->tabs[0].titleSize.Height;
	x /= w->elementSize.Width; y /= w->elementSize.Height;
	
	i = (x + y * e.itemsPerRow) * e.charsPerItem;
	if (i >= e.contents.length) return;

	/* TODO: need to insert characters that don't affect w->caretPos index, adjust w->caretPos colour */
	str = String_Init(&e.contents.buffer[i], e.charsPerItem, 0);

	/* TODO: Not be so hacky */
	if (w->selectedIndex == 0) str.length = 2;
	InputWidget_AppendString(w->target, &str);
}

static void SpecialInputTab_Init(struct SpecialInputTab* tab, STRING_REF String* title, int itemsPerRow, int charsPerItem, STRING_REF String* contents) {
	tab->title     = *title;
	tab->titleSize = Size2D_Empty;
	tab->contents  = *contents;
	tab->itemsPerRow  = itemsPerRow;
	tab->charsPerItem = charsPerItem;
}

static void SpecialInputWidget_InitTabs(struct SpecialInputWidget* w) {
	static String title_cols = String_FromConst("Colours");
	static String title_math = String_FromConst("Math");
	static String tab_math   = String_FromConst("\x9F\xAB\xAC\xE0\xE1\xE2\xE3\xE4\xE5\xE6\xE7\xE8\xE9\xEA\xEB\xEC\xED\xEE\xEF\xF0\xF1\xF2\xF3\xF4\xF5\xF6\xF7\xF8\xFB\xFC\xFD");
	static String title_line = String_FromConst("Line/Box");
	static String tab_line   = String_FromConst("\xB0\xB1\xB2\xB3\xB4\xB5\xB6\xB7\xB8\xB9\xBA\xBB\xBC\xBD\xBE\xBF\xC0\xC1\xC2\xC3\xC4\xC5\xC6\xC7\xC8\xC9\xCA\xCB\xCC\xCD\xCE\xCF\xD0\xD1\xD2\xD3\xD4\xD5\xD6\xD7\xD8\xD9\xDA\xDB\xDC\xDD\xDE\xDF\xFE");
	static String title_letters = String_FromConst("Letters");
	static String tab_letters   = String_FromConst("\x80\x81\x82\x83\x84\x85\x86\x87\x88\x89\x8A\x8B\x8C\x8D\x8E\x8F\x90\x91\x92\x93\x94\x95\x96\x97\x98\x99\x9A\xA0\xA1\xA2\xA3\xA4\xA5");
	static String title_other = String_FromConst("Other");
	static String tab_other   = String_FromConst("\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0A\x0B\x0C\x0D\x0E\x0F\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1A\x1B\x1C\x1D\x1E\x1F\x7F\x9B\x9C\x9D\x9E\xA6\xA7\xA8\xA9\xAA\xAD\xAE\xAF\xF9\xFA");

	SpecialInputWidget_UpdateColString(w);
	SpecialInputTab_Init(&w->tabs[0], &title_cols,    10, 4, &w->colString);
	SpecialInputTab_Init(&w->tabs[1], &title_math,    16, 1, &tab_math);
	SpecialInputTab_Init(&w->tabs[2], &title_line,    17, 1, &tab_line);
	SpecialInputTab_Init(&w->tabs[3], &title_letters, 17, 1, &tab_letters);
	SpecialInputTab_Init(&w->tabs[4], &title_other,   16, 1, &tab_other);
}

#define SPECIAL_TITLE_SPACING 10
#define SPECIAL_CONTENT_SPACING 5
static Size2D SpecialInputWidget_MeasureTitles(struct SpecialInputWidget* w) {
	struct DrawTextArgs args; 
	Size2D size = { 0, 0 };
	int i;

	DrawTextArgs_MakeEmpty(&args, &w->font, false);
	for (i = 0; i < Array_Elems(w->tabs); i++) {
		args.text = w->tabs[i].title;

		w->tabs[i].titleSize = Drawer2D_MeasureText(&args);
		w->tabs[i].titleSize.Width += SPECIAL_TITLE_SPACING;
		size.Width += w->tabs[i].titleSize.Width;
	}

	size.Height = w->tabs[0].titleSize.Height;
	return size;
}

static void SpecialInputWidget_DrawTitles(struct SpecialInputWidget* w, Bitmap* bmp) {
	BitmapCol col_selected = BITMAPCOL_CONST(30, 30, 30, 200);
	BitmapCol col_inactive = BITMAPCOL_CONST( 0,  0,  0, 127);
	BitmapCol col;

	struct DrawTextArgs args;
	Size2D size;
	int i, x = 0;

	DrawTextArgs_MakeEmpty(&args, &w->font, false);
	for (i = 0; i < Array_Elems(w->tabs); i++) {
		args.text = w->tabs[i].title;
		col  = i == w->selectedIndex ? col_selected : col_inactive;
		size = w->tabs[i].titleSize;

		Drawer2D_Clear(bmp, col, x, 0, size.Width, size.Height);
		Drawer2D_DrawText(bmp, &args, x + SPECIAL_TITLE_SPACING / 2, 0);
		x += size.Width;
	}
}

static Size2D SpecialInputWidget_MeasureContent(struct SpecialInputWidget* w, struct SpecialInputTab* tab) {
	struct DrawTextArgs args;
	Size2D size;
	int i, rows;
	
	int maxWidth = 0;
	DrawTextArgs_MakeEmpty(&args, &w->font, false);
	args.text.length = tab->charsPerItem;

	for (i = 0; i < tab->contents.length; i += tab->charsPerItem) {
		args.text.buffer = &tab->contents.buffer[i];
		size     = Drawer2D_MeasureText(&args);
		maxWidth = max(maxWidth, size.Width);
	}

	w->elementSize.Width  = maxWidth    + SPECIAL_CONTENT_SPACING;
	w->elementSize.Height = size.Height + SPECIAL_CONTENT_SPACING;
	rows = Math_CeilDiv(tab->contents.length / tab->charsPerItem, tab->itemsPerRow);

	size.Width  = w->elementSize.Width  * tab->itemsPerRow;
	size.Height = w->elementSize.Height * rows;
	return size;
}

static void SpecialInputWidget_DrawContent(struct SpecialInputWidget* w, struct SpecialInputTab* tab, Bitmap* bmp, int yOffset) {
	struct DrawTextArgs args;
	int i, x, y, item;	

	int wrap = tab->itemsPerRow;
	DrawTextArgs_MakeEmpty(&args, &w->font, false);
	args.text.length = tab->charsPerItem;

	for (i = 0; i < tab->contents.length; i += tab->charsPerItem) {
		args.text.buffer = &tab->contents.buffer[i];
		item = i / tab->charsPerItem;

		x = (item % wrap) * w->elementSize.Width;
		y = (item / wrap) * w->elementSize.Height + yOffset;
		Drawer2D_DrawText(bmp, &args, x, y);
	}
}

static void SpecialInputWidget_Make(struct SpecialInputWidget* w, struct SpecialInputTab* tab) {
	BitmapCol col = PACKEDCOL_CONST(30, 30, 30, 200);
	Size2D size, titles, content;
	Bitmap bmp;

	titles  = SpecialInputWidget_MeasureTitles(w);
	content = SpecialInputWidget_MeasureContent(w, tab);	

	size.Width  = max(titles.Width, content.Width);
	size.Height = titles.Height + content.Height;
	Gfx_DeleteTexture(&w->tex.ID);

	Bitmap_AllocateClearedPow2(&bmp, size.Width, size.Height);
	{
		SpecialInputWidget_DrawTitles(w, &bmp);
		Drawer2D_Clear(&bmp, col, 0, titles.Height, size.Width, content.Height);
		SpecialInputWidget_DrawContent(w, tab, &bmp, titles.Height);
	}
	Drawer2D_Make2DTexture(&w->tex, &bmp, size, w->x, w->y);
	Mem_Free(bmp.Scan0);
}

static void SpecialInputWidget_Redraw(struct SpecialInputWidget* w) {
	SpecialInputWidget_Make(w, &w->tabs[w->selectedIndex]);
	w->width  = w->tex.Width;
	w->height = w->tex.Height;
	w->pendingRedraw = false;
}

static void SpecialInputWidget_Init(void* widget) {
	struct SpecialInputWidget* w = (struct SpecialInputWidget*)widget;
	w->x = 5; w->y = 5;

	SpecialInputWidget_InitTabs(w);
	SpecialInputWidget_Redraw(w);
	SpecialInputWidget_SetActive(w, w->active);
}

static void SpecialInputWidget_Render(void* widget, double delta) {
	struct SpecialInputWidget* w = (struct SpecialInputWidget*)widget;
	Texture_Render(&w->tex);
}

static void SpecialInputWidget_Free(void* widget) {
	struct SpecialInputWidget* w = (struct SpecialInputWidget*)widget;
	Gfx_DeleteTexture(&w->tex.ID);
}

static bool SpecialInputWidget_MouseDown(void* widget, int x, int y, MouseButton btn) {
	struct SpecialInputWidget* w = (struct SpecialInputWidget*)widget;
	x -= w->x; y -= w->y;

	if (SpecialInputWidget_IntersectsTitle(w, x, y)) {
		SpecialInputWidget_Redraw(w);
	} else {
		SpecialInputWidget_IntersectsBody(w, x, y);
	}
	return true;
}

void SpecialInputWidget_UpdateCols(struct SpecialInputWidget* w) {
	SpecialInputWidget_UpdateColString(w);
	w->tabs[0].contents = w->colString;
	if (w->selectedIndex != 0) return;

	/* defer updating colours tab until visible */
	if (!w->active) { w->pendingRedraw = true; return; }
	SpecialInputWidget_Redraw(w);
}

void SpecialInputWidget_SetActive(struct SpecialInputWidget* w, bool active) {
	w->active = active;
	w->height = active ? w->tex.Height : 0;
	if (active && w->pendingRedraw) SpecialInputWidget_Redraw(w);
}

static struct WidgetVTABLE SpecialInputWidget_VTABLE = {
	SpecialInputWidget_Init,      SpecialInputWidget_Render, SpecialInputWidget_Free, Gui_DefaultRecreate,
	Widget_KeyDown,               Widget_KeyUp,              Widget_KeyPress,
	SpecialInputWidget_MouseDown, Widget_Mouse,              Widget_MouseMove,        Widget_MouseScroll,
	Widget_CalcPosition,
};
void SpecialInputWidget_Create(struct SpecialInputWidget* w, const FontDesc* font, struct InputWidget* target) {
	Widget_Reset(w);
	w->VTABLE    = &SpecialInputWidget_VTABLE;
	w->verAnchor = ANCHOR_MAX;
	w->font      = *font;
	w->target = target;
}
