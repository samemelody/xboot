/*
 * kernel/xui/xui.c
 *
 * Copyright(c) 2007-2019 Jianjun Jiang <8192542@qq.com>
 * Official site: http://xboot.org
 * Mobile phone: +86-18665388956
 * QQ: 8192542
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include <xboot.h>
#include <xui/xui.h>

static struct xui_style_t xui_style_default = {
	.bgcol = {
		.r = 90,
		.g = 95,
		.b = 100,
		.a = 255,
	},

	.font = NULL,
	.width = 68,
	.height = 10,
	.padding = 5,
	.spacing = 4,
	.indent = 24,
	.title_height = 24,
	.scrollbar_size = 12,
	.thumb_size = 8,

	.colors = {
		{ 230, 230, 230, 255 }, /* XUI_COLOR_TEXT */
		{ 25,  25,  25,  255 }, /* XUI_COLOR_BORDER */
		{ 50,  50,  50,  255 }, /* XUI_COLOR_WINDOW */
		{ 25,  25,  25,  255 }, /* XUI_COLOR_TITLEBG */
		{ 240, 240, 240, 255 }, /* XUI_COLOR_TITLETEXT */
		{ 0,   0,   0,   0   }, /* XUI_COLOR_PANEL */
		{ 75,  75,  75,  255 }, /* XUI_COLOR_BUTTON */
		{ 95,  95,  95,  255 }, /* XUI_COLOR_BUTTONHOVER */
		{ 115, 115, 115, 255 }, /* XUI_COLOR_BUTTONFOCUS */
		{ 30,  30,  30,  255 }, /* XUI_COLOR_BASE */
		{ 35,  35,  35,  255 }, /* XUI_COLOR_BASEHOVER */
		{ 40,  40,  40,  255 }, /* XUI_COLOR_BASEFOCUS */
		{ 43,  43,  43,  255 }, /* XUI_COLOR_SCROLLBASE */
		{ 30,  30,  30,  255 }  /* XUI_COLOR_SCROLLTHUMB */
	}
};

static struct region_t unclipped_region = {
	.x = 0,
	.y = 0,
	.w = INT_MAX,
	.h = INT_MAX,
	.area = -1,
};

void xui_draw_rect(struct xui_context_t * ctx, struct region_t * r, struct color_t * c)
{
	struct region_t region;

	if(region_intersect(&region, r, xui_get_clip(ctx)))
	{
		union xui_command_t * cmd = xui_push_command(ctx, XUI_COMMAND_TYPE_RECT, sizeof(struct xui_command_rect_t));
		region_clone(&cmd->rect.rect, &region);
		memcpy(&cmd->rect.color, c, sizeof(struct color_t));
	}
}

void xui_draw_box(struct xui_context_t * ctx, struct region_t * r, struct color_t * c)
{
	struct region_t region;

	region_init(&region, r->x + 1, r->y, r->w - 2, 1);
	xui_draw_rect(ctx, &region, c);
	region_init(&region, r->x + 1, r->y + r->h - 1, r->w - 2, 1);
	xui_draw_rect(ctx, &region, c);
	region_init(&region, r->x, r->y, 1, r->h);
	xui_draw_rect(ctx, &region, c);
	region_init(&region, r->x + r->w - 1, r->y, 1, r->h);
	xui_draw_rect(ctx, &region, c);
}

static void draw_frame(struct xui_context_t * ctx, struct region_t * r, int cid)
{
	struct color_t * col = &ctx->style.colors[cid];
	xui_draw_rect(ctx, r, col);
	if(cid == XUI_COLOR_SCROLLBASE || cid == XUI_COLOR_SCROLLTHUMB || cid == XUI_COLOR_TITLEBG)
		return;
	if(ctx->style.colors[XUI_COLOR_BORDER].a)
	{
		struct region_t region;
		region_expand(&region, r, 1);
		xui_draw_box(ctx, &region, &ctx->style.colors[XUI_COLOR_BORDER]);
	}
}

static int text_width(void * font, const char *text, int len)
{
	if(len == -1)
		len = strlen(text);
	return 8 * len;
}

static int text_height(void * font)
{
	return 16;
}

struct xui_context_t * xui_context_alloc(const char * fb, const char * input, struct xui_style_t * style)
{
	struct xui_context_t * ctx;

	ctx = malloc(sizeof(struct xui_context_t));
	if(!ctx)
		return NULL;

	memset(ctx, 0, sizeof(struct xui_context_t));
	ctx->w = window_alloc(fb, input, ctx);
	ctx->f = font_context_alloc();

	memcpy(&ctx->style, style ? style : &xui_style_default, sizeof(struct xui_style_t));
	region_init(&ctx->clip, 0, 0, window_get_width(ctx->w), window_get_height(ctx->w));

	ctx->draw_frame = draw_frame;
	ctx->text_width = text_width;
	ctx->text_height = text_height;
	return ctx;
}

void xui_context_free(struct xui_context_t * ctx)
{
	if(ctx)
	{
		window_free(ctx->w);
		font_context_free(ctx->f);
		free(ctx);
	}
}

static void xui_draw(struct window_t * w, void * o)
{
	struct xui_context_t * ctx = (struct xui_context_t *)o;
	struct surface_t * s = ctx->w->s;
	struct region_t * clip = &ctx->clip;
    union xui_command_t * cmd = NULL;

	while(xui_next_command(ctx, &cmd))
	{
		switch(cmd->base.type)
		{
		case XUI_COMMAND_TYPE_JUMP:
			break;
		case XUI_COMMAND_TYPE_CLIP:
			{
				struct region_t a, b;
				region_init(&a, cmd->clip.rect.x, cmd->clip.rect.y, cmd->clip.rect.w, cmd->clip.rect.h);
				region_init(&b, 0, 0, window_get_width(ctx->w), window_get_height(ctx->w));
				region_intersect(clip, &a, &b);
			}
			break;
		case XUI_COMMAND_TYPE_RECT:
			surface_shape_rectangle(s, clip, cmd->rect.rect.x, cmd->rect.rect.y, cmd->rect.rect.w, cmd->rect.rect.h, 0, 0, &cmd->rect.color);
			break;
		case XUI_COMMAND_TYPE_TEXT:
			font_draw(s, clip, cmd->text.x, cmd->text.y, cmd->text.str, &cmd->text.color);
			break;
		case XUI_COMMAND_TYPE_ICON:
			//r_draw_icon(cmd->icon.id, cmd->icon.rect, cmd->icon.color);
			break;
		default:
			break;
		}
	}
}

void xui_loop(struct xui_context_t * ctx, void (*func)(struct xui_context_t *))
{
	struct event_t e;

	while(1)
	{
		if(window_pump_event(ctx->w, &e))
		{
			switch(e.type)
			{
			case EVENT_TYPE_KEY_DOWN:
				xui_input_keydown(ctx, e.e.key_down.key);
				break;
			case EVENT_TYPE_KEY_UP:
				xui_input_keyup(ctx, e.e.key_up.key);
				break;
			case EVENT_TYPE_MOUSE_DOWN:
				xui_input_mousedown(ctx, e.e.mouse_down.x, e.e.mouse_down.y, e.e.mouse_down.button);
				break;
			case EVENT_TYPE_MOUSE_MOVE:
				xui_input_mousemove(ctx, e.e.mouse_move.x, e.e.mouse_move.y);
				break;
			case EVENT_TYPE_MOUSE_UP:
				xui_input_mouseup(ctx, e.e.mouse_up.x, e.e.mouse_up.y, e.e.mouse_up.button);
				break;
			case EVENT_TYPE_MOUSE_WHEEL:
				xui_input_scroll(ctx, e.e.mouse_wheel.dx * 30, e.e.mouse_wheel.dy * -30);
				break;
			default:
				break;
			}
		}
		if(func)
			func(ctx);
		if(window_is_active(ctx->w))
		{
			ctx->w->wm->refresh = 1;
			window_present(ctx->w, &ctx->style.bgcol, ctx, xui_draw);
		}
		task_yield();
	}
}

#define expect(x) do { \
		if(!(x)) \
		{ \
			fprintf(stderr, "Fatal error: %s:%d: assertion '%s' failed\r\n", __FILE__, __LINE__, #x); \
		} \
	} while(0)

#define xui_push(stk, val)	do { expect((stk).idx < (int)(sizeof((stk).items) / sizeof(*(stk).items))); (stk).items[(stk).idx] = (val); (stk).idx++; } while(0)
#define xui_pop(stk)		do { expect((stk).idx > 0); (stk).idx--; } while(0)

void xui_begin(struct xui_context_t * ctx)
{
	ctx->command_list.idx = 0;
	ctx->root_list.idx = 0;
	ctx->scroll_target = NULL;
	ctx->hover_root = ctx->next_hover_root;
	ctx->next_hover_root = NULL;
	ctx->mouse_delta_x = ctx->mouse_pos_x - ctx->last_mouse_pos_x;
	ctx->mouse_delta_y = ctx->mouse_pos_y - ctx->last_mouse_pos_y;
	ctx->frame++;
}

static int compare_zindex(const void * a, const void * b)
{
	return (*(struct xui_container_t **)a)->zindex - (*(struct xui_container_t **)b)->zindex;
}

void xui_end(struct xui_context_t * ctx)
{
	int i, n;

	expect(ctx->container_stack.idx == 0);
	expect(ctx->clip_stack.idx      == 0);
	expect(ctx->id_stack.idx        == 0);
	expect(ctx->layout_stack.idx    == 0);
	if(ctx->scroll_target)
	{
		ctx->scroll_target->scroll_abc.x += ctx->scroll_delta_x;
		ctx->scroll_target->scroll_abc.y += ctx->scroll_delta_y;
	}
	if(!ctx->updated_focus)
		ctx->focus = 0;
	ctx->updated_focus = 0;
	if(ctx->mouse_pressed && ctx->next_hover_root && (ctx->next_hover_root->zindex < ctx->last_zindex) && (ctx->next_hover_root->zindex >= 0))
		xui_set_front(ctx, ctx->next_hover_root);
	ctx->key_pressed = 0;
	ctx->input_text[0] = '\0';
	ctx->mouse_pressed = 0;
	ctx->scroll_delta_x = 0;
	ctx->scroll_delta_y = 0;
	ctx->last_mouse_pos_x = ctx->mouse_pos_x;
	ctx->last_mouse_pos_y = ctx->mouse_pos_y;
	n = ctx->root_list.idx;
	qsort(ctx->root_list.items, n, sizeof(struct xui_container_t *), compare_zindex);
	for(i = 0; i < n; i++)
	{
		struct xui_container_t * c = ctx->root_list.items[i];
		if(i == 0)
		{
			union xui_command_t * cmd = (union xui_command_t *)ctx->command_list.items;
			cmd->jump.addr = (char *)c->head + sizeof(struct xui_command_jump_t);
		}
		else
		{
			struct xui_container_t * prev = ctx->root_list.items[i - 1];
			prev->tail->jump.addr = (char *)c->head + sizeof(struct xui_command_jump_t);
		}
		if(i == n - 1)
			c->tail->jump.addr = ctx->command_list.items + ctx->command_list.idx;
	}
}

void xui_set_front(struct xui_context_t * ctx, struct xui_container_t * c)
{
	c->zindex = ++ctx->last_zindex;
}

void xui_set_focus(struct xui_context_t * ctx, unsigned int id)
{
	ctx->focus = id;
	ctx->updated_focus = 1;
}

static void xui_hash(unsigned int * hash, const void * data, int size)
{
	const unsigned char * p = data;
	while(size--)
		*hash = (*hash ^ *p++) * 16777619;
}

unsigned int xui_get_id(struct xui_context_t * ctx, const void * data, int size)
{
	int idx = ctx->id_stack.idx;
	unsigned int res = (idx > 0) ? ctx->id_stack.items[idx - 1] : 2166136261;
	xui_hash(&res, data, size);
	ctx->last_id = res;
	return res;
}

void xui_push_id(struct xui_context_t * ctx, const void * data, int size)
{
	xui_push(ctx->id_stack, xui_get_id(ctx, data, size));
}

void xui_pop_id(struct xui_context_t * ctx)
{
	xui_pop(ctx->id_stack);
}

struct region_t * xui_get_clip(struct xui_context_t * ctx)
{
	expect(ctx->clip_stack.idx > 0);
	return &ctx->clip_stack.items[ctx->clip_stack.idx - 1];
}

void xui_push_clip(struct xui_context_t * ctx, struct region_t * r)
{
	struct region_t region;
	if(!region_intersect(&region, r, xui_get_clip(ctx)))
		region_init(&region, 0, 0, 0, 0);
	xui_push(ctx->clip_stack, region);
}

void xui_pop_clip(struct xui_context_t * ctx)
{
	xui_pop(ctx->clip_stack);
}

int xui_check_clip(struct xui_context_t * ctx, struct region_t * r)
{
	struct region_t cr;
	region_clone(&cr, xui_get_clip(ctx));
	if((r->x > cr.x + cr.w) || (r->x + r->w < cr.x) || (r->y > cr.y + cr.h) || (r->y + r->h < cr.y))
		return MU_CLIP_ALL;
	if((r->x >= cr.x) && (r->x + r->w <= cr.x + cr.w) && (r->y >= cr.y) && (r->y + r->h <= cr.y + cr.h))
		return 0;
	return MU_CLIP_PART;
}

static void push_layout(struct xui_context_t * ctx, struct region_t * body, int scrollx, int scrolly)
{
	struct xui_layout_t layout;
	int width = 0;
	memset(&layout, 0, sizeof(layout));
	region_init(&layout.body, body->x - scrollx, body->y - scrolly, body->w, body->h);
	layout.max_width = INT_MIN;
	layout.max_height = INT_MIN;
	xui_push(ctx->layout_stack, layout);
	xui_layout_row(ctx, 1, &width, 0);
}

static struct xui_layout_t * get_layout(struct xui_context_t * ctx)
{
	return &ctx->layout_stack.items[ctx->layout_stack.idx - 1];
}

static void pop_container(struct xui_context_t * ctx)
{
	struct xui_container_t * c = xui_get_current_container(ctx);
	struct xui_layout_t * layout = get_layout(ctx);
	c->content_width = layout->max_width - layout->body.x;
	c->content_height = layout->max_height - layout->body.y;
	xui_pop(ctx->container_stack);
	xui_pop(ctx->layout_stack);
	xui_pop_id(ctx);
}

static struct xui_container_t * get_container(struct xui_context_t * ctx, unsigned int id, int opt)
{
	struct xui_container_t * c;
	int idx = xui_pool_get(ctx, ctx->container_pool, MU_CONTAINERPOOL_SIZE, id);
	if(idx >= 0)
	{
		if(ctx->containers[idx].open || (~opt & XUI_OPT_CLOSED))
			xui_pool_update(ctx, ctx->container_pool, idx);
		return &ctx->containers[idx];
	}
	if(opt & XUI_OPT_CLOSED)
		return NULL;
	idx = xui_pool_init(ctx, ctx->container_pool, MU_CONTAINERPOOL_SIZE, id);
	c = &ctx->containers[idx];
	memset(c, 0, sizeof(struct xui_container_t));
	c->open = 1;
	xui_set_front(ctx, c);
	return c;
}

struct xui_container_t * xui_get_container(struct xui_context_t * ctx, const char * name)
{
	unsigned int id = xui_get_id(ctx, name, strlen(name));
	return get_container(ctx, id, 0);
}

struct xui_container_t * xui_get_current_container(struct xui_context_t * ctx)
{
	expect(ctx->container_stack.idx > 0);
	return ctx->container_stack.items[ctx->container_stack.idx - 1];
}

int xui_pool_init(struct xui_context_t * ctx, struct xui_pool_item_t * items, int len, unsigned int id)
{
	int i, n = -1, f = ctx->frame;
	for(i = 0; i < len; i++)
	{
		if(items[i].last_update < f)
		{
			f = items[i].last_update;
			n = i;
		}
	}
	expect(n > -1);
	items[n].id = id;
	xui_pool_update(ctx, items, n);
	return n;
}

int xui_pool_get(struct xui_context_t * ctx, struct xui_pool_item_t * items, int len, unsigned int id)
{
	int i;
	for(i = 0; i < len; i++)
	{
		if(items[i].id == id)
			return i;
	}
	return -1;
}

void xui_pool_update(struct xui_context_t * ctx, struct xui_pool_item_t * items, int idx)
{
	items[idx].last_update = ctx->frame;
}

void xui_input_mousemove(struct xui_context_t * ctx, int x, int y)
{
	ctx->mouse_pos_x = x;
	ctx->mouse_pos_y = y;
}

void xui_input_mousedown(struct xui_context_t * ctx, int x, int y, int btn)
{
	xui_input_mousemove(ctx, x, y);
	ctx->mouse_down |= btn;
	ctx->mouse_pressed |= btn;
}

void xui_input_mouseup(struct xui_context_t * ctx, int x, int y, int btn)
{
	xui_input_mousemove(ctx, x, y);
	ctx->mouse_down &= ~btn;
}

void xui_input_scroll(struct xui_context_t * ctx, int x, int y)
{
	ctx->scroll_delta_x += x;
	ctx->scroll_delta_y += y;
}

void xui_input_keydown(struct xui_context_t * ctx, int key)
{
	ctx->key_pressed |= key;
	ctx->key_down |= key;
}

void xui_input_keyup(struct xui_context_t * ctx, int key)
{
	ctx->key_down &= ~key;
}

void xui_input_text(struct xui_context_t * ctx, const char * text)
{
	int len = strlen(ctx->input_text);
	int size = strlen(text) + 1;
	expect(len + size <= (int ) sizeof(ctx->input_text));
	memcpy(ctx->input_text + len, text, size);
}

union xui_command_t * xui_push_command(struct xui_context_t * ctx, enum xui_command_type_t type, int size)
{
	union xui_command_t * cmd = (union xui_command_t *)(ctx->command_list.items + ctx->command_list.idx);
	expect(ctx->command_list.idx + size < MU_COMMANDLIST_SIZE);
	cmd->base.type = type;
	cmd->base.size = size;
	ctx->command_list.idx += size;
	return cmd;
}

int xui_next_command(struct xui_context_t * ctx, union xui_command_t ** cmd)
{
	if(*cmd)
		*cmd = (union xui_command_t *)(((char *)*cmd) + (*cmd)->base.size);
	else
		*cmd = (union xui_command_t *)ctx->command_list.items;
	while((char *)(*cmd) != ctx->command_list.items + ctx->command_list.idx)
	{
		if((*cmd)->base.type != XUI_COMMAND_TYPE_JUMP)
			return 1;
		*cmd = (*cmd)->jump.addr;
	}
	return 0;
}

static union xui_command_t * push_jump(struct xui_context_t * ctx, union xui_command_t * addr)
{
	union xui_command_t * cmd = xui_push_command(ctx, XUI_COMMAND_TYPE_JUMP, sizeof(struct xui_command_jump_t));
	cmd->jump.addr = addr;
	return cmd;
}

void xui_set_clip(struct xui_context_t * ctx, struct region_t * r)
{
	union xui_command_t * cmd = xui_push_command(ctx, XUI_COMMAND_TYPE_CLIP, sizeof(struct xui_command_clip_t));
	region_clone(&cmd->clip.rect, r);
}

void xui_draw_text(struct xui_context_t * ctx, void * font, const char * str, int len, int x, int y, struct color_t * c)
{
	union xui_command_t * cmd;
	struct region_t rect;
	region_init(&rect, x, y, ctx->text_width(font, str, len), ctx->text_height(font));
	int clipped = xui_check_clip(ctx, &rect);
	if(clipped == MU_CLIP_ALL)
		return;
	if(clipped == MU_CLIP_PART)
		xui_set_clip(ctx, xui_get_clip(ctx));
	if(len < 0)
		len = strlen(str);
	cmd = xui_push_command(ctx, XUI_COMMAND_TYPE_TEXT, sizeof(struct xui_command_text_t) + len);
	memcpy(cmd->text.str, str, len);
	cmd->text.str[len] = '\0';
	cmd->text.x = x;
	cmd->text.y = y;
	memcpy(&cmd->text.color, c, sizeof(struct color_t));
	cmd->text.font = font;
	if(clipped)
		xui_set_clip(ctx, &unclipped_region);
}

void xui_draw_icon(struct xui_context_t * ctx, int id, struct region_t * r, struct color_t * c)
{
	union xui_command_t * cmd;
	int clipped = xui_check_clip(ctx, r);
	if(clipped == MU_CLIP_ALL)
		return;
	if(clipped == MU_CLIP_PART)
		xui_set_clip(ctx, xui_get_clip(ctx));
	cmd = xui_push_command(ctx, XUI_COMMAND_TYPE_ICON, sizeof(struct xui_command_icon_t));
	cmd->icon.id = id;
	region_clone(&cmd->icon.rect, r);
	memcpy(&cmd->icon.color, c, sizeof(struct color_t));
	if(clipped)
		xui_set_clip(ctx, &unclipped_region);
}

void xui_layout_width(struct xui_context_t * ctx, int width)
{
	get_layout(ctx)->size_width = width;
}

void xui_layout_height(struct xui_context_t * ctx, int height)
{
	get_layout(ctx)->size_height = height;
}

void xui_layout_row(struct xui_context_t * ctx, int items, const int * widths, int height)
{
	struct xui_layout_t * layout = get_layout(ctx);
	if(widths)
	{
		expect(items <= MU_MAX_WIDTHS);
		memcpy(layout->widths, widths, items * sizeof(widths[0]));
	}
	layout->items = items;
	layout->position_x = layout->indent;
	layout->position_y = layout->next_row;
	layout->size_height = height;
	layout->item_index = 0;
}

void xui_layout_begin_column(struct xui_context_t * ctx)
{
	push_layout(ctx, xui_layout_next(ctx), 0, 0);
}

void xui_layout_end_column(struct xui_context_t * ctx)
{
	struct xui_layout_t * a, * b;
	b = get_layout(ctx);
	xui_pop(ctx->layout_stack);
	a = get_layout(ctx);
	a->position_x = max(a->position_x, b->position_x + b->body.x - a->body.x);
	a->next_row = max(a->next_row, b->next_row + b->body.y - a->body.y);
	a->max_width = max(a->max_width, b->max_width);
	a->max_height = max(a->max_height, b->max_height);
}

void xui_layout_set_next(struct xui_context_t * ctx, struct region_t * r, int relative)
{
	struct xui_layout_t * layout = get_layout(ctx);
	region_clone(&layout->next, r);
	layout->next_type = relative ? 1 : 2;
}

struct region_t * xui_layout_next(struct xui_context_t * ctx)
{
	struct xui_layout_t * layout = get_layout(ctx);
	struct xui_style_t * style = &ctx->style;
	struct region_t r;

	if(layout->next_type)
	{
		int type = layout->next_type;
		layout->next_type = 0;
		region_clone(&r, &layout->next);
		if(type == 2)
		{
			region_clone(&ctx->last_rect, &r);
			return &ctx->last_rect;
		}
	}
	else
	{
		if(layout->item_index == layout->items)
			xui_layout_row(ctx, layout->items, NULL, layout->size_height);
		r.x = layout->position_x;
		r.y = layout->position_y;
		r.w = layout->items > 0 ? layout->widths[layout->item_index] : layout->size_width;
		r.h = layout->size_height;
		if(r.w == 0)
			r.w = style->width + style->padding * 2;
		if(r.h == 0)
			r.h = style->height + style->padding * 2;
		if(r.w < 0)
			r.w += layout->body.w - r.x + 1;
		if(r.h < 0)
			r.h += layout->body.h - r.y + 1;
		layout->item_index++;
	}

	layout->position_x += r.w + style->spacing;
	layout->next_row = max(layout->next_row, r.y + r.h + style->spacing);
	r.x += layout->body.x;
	r.y += layout->body.y;
	layout->max_width = max(layout->max_width, r.x + r.w);
	layout->max_height = max(layout->max_height, r.y + r.h);

	region_clone(&ctx->last_rect, &r);
	return &ctx->last_rect;
}

static int in_hover_root(struct xui_context_t * ctx)
{
	int i = ctx->container_stack.idx;
	while(i--)
	{
		if(ctx->container_stack.items[i] == ctx->hover_root)
			return 1;
		if(ctx->container_stack.items[i]->head)
			break;
	}
	return 0;
}

void xui_draw_control_frame(struct xui_context_t * ctx, unsigned int id, struct region_t * r, int cid, int opt)
{
	if(opt & XUI_OPT_NOFRAME)
		return;
	cid += (ctx->focus == id) ? 2 : (ctx->hover == id) ? 1 : 0;
	ctx->draw_frame(ctx, r, cid);
}

void xui_draw_control_text(struct xui_context_t * ctx, const char * str, struct region_t * r, int cid, int opt)
{
	void * font = ctx->style.font;
	int tw = ctx->text_width(font, str, -1);
	int x, y;
	xui_push_clip(ctx, r);
	y = r->y + (r->h - ctx->text_height(font)) / 2;
	if(opt & XUI_OPT_ALIGNCENTER)
		x = r->x + (r->w - tw) / 2;
	else if(opt & XUI_OPT_ALIGNRIGHT)
		x = r->x + r->w - tw - ctx->style.padding;
	else
		x = r->x + ctx->style.padding;
	xui_draw_text(ctx, font, str, -1, x, y, &ctx->style.colors[cid]);
	xui_pop_clip(ctx);
}

int xui_mouse_over(struct xui_context_t * ctx, struct region_t * r)
{
	return region_hit(r, ctx->mouse_pos_x, ctx->mouse_pos_y) && region_hit(xui_get_clip(ctx), ctx->mouse_pos_x, ctx->mouse_pos_y) && in_hover_root(ctx);
}

void xui_update_control(struct xui_context_t * ctx, unsigned int id, struct region_t * r, int opt)
{
	int mouseover = xui_mouse_over(ctx, r);

	if(ctx->focus == id)
		ctx->updated_focus = 1;
	if(opt & XUI_OPT_NOINTERACT)
		return;
	if(mouseover && !ctx->mouse_down)
		ctx->hover = id;
	if(ctx->focus == id)
	{
		if(ctx->mouse_pressed && !mouseover)
			xui_set_focus(ctx, 0);
		if(!ctx->mouse_down && (~opt & XUI_OPT_HOLDFOCUS))
			xui_set_focus(ctx, 0);
	}
	if(ctx->hover == id)
	{
		if(ctx->mouse_pressed)
			xui_set_focus(ctx, id);
		else if(!mouseover)
			ctx->hover = 0;
	}
}

void xui_text(struct xui_context_t * ctx, const char * text)
{
	const char *start, *end, *p = text;
	int width = -1;
	void * font = ctx->style.font;
	struct color_t * c = &ctx->style.colors[XUI_COLOR_TEXT];
	xui_layout_begin_column(ctx);
	xui_layout_row(ctx, 1, &width, ctx->text_height(font));
	do
	{
		struct region_t r;
		region_clone(&r, xui_layout_next(ctx));
		int w = 0;
		start = end = p;
		do
		{
			const char * word = p;
			while(*p && (*p != ' ') && (*p != '\n'))
				p++;
			w += ctx->text_width(font, word, p - word);
			if((w > r.w) && (end != start))
				break;
			w += ctx->text_width(font, p, 1);
			end = p++;
		} while(*end && (*end != '\n'));
		xui_draw_text(ctx, font, start, end - start, r.x, r.y, c);
		p = end + 1;
	} while(*end);
	xui_layout_end_column(ctx);
}

void xui_label(struct xui_context_t * ctx, const char * text)
{
	xui_draw_control_text(ctx, text, xui_layout_next(ctx), XUI_COLOR_TEXT, 0);
}

int xui_button_ex(struct xui_context_t * ctx, const char * label, int icon, int opt)
{
	int res = 0;
	unsigned int id = label ? xui_get_id(ctx, label, strlen(label)) : xui_get_id(ctx, &icon, sizeof(icon));
	struct region_t r;
	region_clone(&r, xui_layout_next(ctx));
	xui_update_control(ctx, id, &r, opt);
	if(ctx->mouse_pressed == MU_MOUSE_LEFT && ctx->focus == id)
		res |= XUI_RES_SUBMIT;
	xui_draw_control_frame(ctx, id, &r, XUI_COLOR_BUTTON, opt);
	if(label)
		xui_draw_control_text(ctx, label, &r, XUI_COLOR_TEXT, opt);
	if(icon)
		xui_draw_icon(ctx, icon, &r, &ctx->style.colors[XUI_COLOR_TEXT]);
	return res;
}

int xui_button(struct xui_context_t * ctx, const char * label)
{
	return xui_button_ex(ctx, label, 0, XUI_OPT_ALIGNCENTER);
}

int xui_checkbox(struct xui_context_t * ctx, const char * label, int * state)
{
	int res = 0;
	unsigned int id = xui_get_id(ctx, &state, sizeof(state));
	struct region_t r;
	region_clone(&r, xui_layout_next(ctx));
	struct region_t box;
	region_init(&box, r.x, r.y, r.h, r.h);
	xui_update_control(ctx, id, &r, 0);
	if(ctx->mouse_pressed == MU_MOUSE_LEFT && ctx->focus == id)
	{
		res |= XUI_RES_CHANGE;
		*state = !*state;
	}
	xui_draw_control_frame(ctx, id, &box, XUI_COLOR_BASE, 0);
	if(*state)
		xui_draw_icon(ctx, MU_ICON_CHECK, &box, &ctx->style.colors[XUI_COLOR_TEXT]);
	region_init(&r, r.x + box.w, r.y, r.w - box.w, r.h);
	xui_draw_control_text(ctx, label, &r, XUI_COLOR_TEXT, 0);
	return res;
}

int xui_textbox_raw(struct xui_context_t * ctx, char * buf, int bufsz, unsigned int id, struct region_t * r, int opt)
{
	int res = 0;
	xui_update_control(ctx, id, r, opt | XUI_OPT_HOLDFOCUS);

	if(ctx->focus == id)
	{
		int len = strlen(buf);
		int n = min(bufsz - len - 1, (int ) strlen(ctx->input_text));
		if(n > 0)
		{
			memcpy(buf + len, ctx->input_text, n);
			len += n;
			buf[len] = '\0';
			res |= XUI_RES_CHANGE;
		}
		if((ctx->key_pressed & MU_KEY_BACKSPACE) && len > 0)
		{
			while((buf[--len] & 0xc0) == 0x80 && len > 0)
				;
			buf[len] = '\0';
			res |= XUI_RES_CHANGE;
		}
		if(ctx->key_pressed & MU_KEY_RETURN)
		{
			xui_set_focus(ctx, 0);
			res |= XUI_RES_SUBMIT;
		}
	}
	xui_draw_control_frame(ctx, id, r, XUI_COLOR_BASE, opt);
	if(ctx->focus == id)
	{
		struct color_t * c = &ctx->style.colors[XUI_COLOR_TEXT];
		void * font = ctx->style.font;
		int textw = ctx->text_width(font, buf, -1);
		int texth = ctx->text_height(font);
		int ofx = r->w - ctx->style.padding - textw - 1;
		int textx = r->x + min(ofx, ctx->style.padding);
		int texty = r->y + (r->h - texth) / 2;
		xui_push_clip(ctx, r);
		xui_draw_text(ctx, font, buf, -1, textx, texty, c);
		struct region_t region;
		region_init(&region, textx + textw, texty, 1, texth);
		xui_draw_rect(ctx, &region, c);
		xui_pop_clip(ctx);
	}
	else
	{
		xui_draw_control_text(ctx, buf, r, XUI_COLOR_TEXT, opt);
	}
	return res;
}

static int number_textbox(struct xui_context_t * ctx, float * value, struct region_t * r, unsigned int id)
{
	if(ctx->mouse_pressed == MU_MOUSE_LEFT && (ctx->key_down & MU_KEY_SHIFT) && ctx->hover == id)
	{
		ctx->number_edit = id;
		sprintf(ctx->number_edit_buf, MU_REAL_FMT, *value);
	}
	if(ctx->number_edit == id)
	{
		int res = xui_textbox_raw(ctx, ctx->number_edit_buf, sizeof(ctx->number_edit_buf), id, r, 0);
		if((res & XUI_RES_SUBMIT) || ctx->focus != id)
		{
			*value = strtod(ctx->number_edit_buf, NULL);
			ctx->number_edit = 0;
		}
		else
		{
			return 1;
		}
	}
	return 0;
}

int xui_textbox_ex(struct xui_context_t * ctx, char * buf, int bufsz, int opt)
{
	unsigned int id = xui_get_id(ctx, &buf, sizeof(buf));
	struct region_t r;
	region_clone(&r, xui_layout_next(ctx));
	return xui_textbox_raw(ctx, buf, bufsz, id, &r, opt);
}

int xui_textbox(struct xui_context_t * ctx, char * buf, int bufsz)
{
	return xui_textbox_ex(ctx, buf, bufsz, 0);
}

int xui_slider_ex(struct xui_context_t * ctx, float * value, float low, float high, float step, const char * fmt, int opt)
{
	char buf[MU_MAX_FMT + 1];
	struct region_t thumb;
	int x, w, res = 0;
	float last = *value, v = last;
	unsigned int id = xui_get_id(ctx, &value, sizeof(value));
	struct region_t base;
	region_clone(&base, xui_layout_next(ctx));

	if(number_textbox(ctx, &v, &base, id))
		return res;

	xui_update_control(ctx, id, &base, opt);

	if((ctx->focus == id) && (ctx->mouse_down == MU_MOUSE_LEFT))
	{
		v = low + (ctx->mouse_pos_x - base.x) * (high - low) / base.w;
		if(step)
			v = (((v + step / 2) / step)) * step;
	}
	*value = v = clamp(v, low, high);
	if(last != v)
		res |= XUI_RES_CHANGE;

	xui_draw_control_frame(ctx, id, &base, XUI_COLOR_BASE, opt);
	w = ctx->style.thumb_size;
	x = (v - low) * (base.w - w) / (high - low);
	region_init(&thumb, base.x + x, base.y, w, base.h);
	xui_draw_control_frame(ctx, id, &thumb, XUI_COLOR_BUTTON, opt);
	sprintf(buf, fmt, v);
	xui_draw_control_text(ctx, buf, &base, XUI_COLOR_TEXT, opt);

	return res;
}

int xui_slider(struct xui_context_t * ctx, float * value, float low, float high)
{
	return xui_slider_ex(ctx, value, low, high, 0, "%.2f", XUI_OPT_ALIGNCENTER);
}

int xui_number_ex(struct xui_context_t * ctx, float * value, float step, const char * fmt, int opt)
{
	char buf[MU_MAX_FMT + 1];
	int res = 0;
	unsigned int id = xui_get_id(ctx, &value, sizeof(value));
	struct region_t base;
	region_clone(&base, xui_layout_next(ctx));
	float last = *value;

	if(number_textbox(ctx, value, &base, id))
		return res;
	xui_update_control(ctx, id, &base, opt);
	if(ctx->focus == id && ctx->mouse_down == MU_MOUSE_LEFT)
		*value += ctx->mouse_delta_x * step;
	if(*value != last)
		res |= XUI_RES_CHANGE;
	xui_draw_control_frame(ctx, id, &base, XUI_COLOR_BASE, opt);
	sprintf(buf, fmt, *value);
	xui_draw_control_text(ctx, buf, &base, XUI_COLOR_TEXT, opt);

	return res;
}

int xui_number(struct xui_context_t * ctx, float * value, float step)
{
	return xui_number_ex(ctx, value, step, "%.2f", XUI_OPT_ALIGNCENTER);
}

static int header(struct xui_context_t * ctx, const char * label, int istreenode, int opt)
{
	struct region_t r;
	int active, expanded;
	unsigned int id = xui_get_id(ctx, label, strlen(label));
	int idx = xui_pool_get(ctx, ctx->treenode_pool, MU_TREENODEPOOL_SIZE, id);
	int width = -1;
	xui_layout_row(ctx, 1, &width, 0);

	active = (idx >= 0);
	expanded = (opt & XUI_OPT_EXPANDED) ? !active : active;
	region_clone(&r, xui_layout_next(ctx));
	xui_update_control(ctx, id, &r, 0);

	active ^= (ctx->mouse_pressed == MU_MOUSE_LEFT && ctx->focus == id);

	if(idx >= 0)
	{
		if(active)
			xui_pool_update(ctx, ctx->treenode_pool, idx);
		else
			memset(&ctx->treenode_pool[idx], 0, sizeof(struct xui_pool_item_t));
	}
	else if(active)
	{
		xui_pool_init(ctx, ctx->treenode_pool, MU_TREENODEPOOL_SIZE, id);
	}
	if(istreenode)
	{
		if(ctx->hover == id)
			ctx->draw_frame(ctx, &r, XUI_COLOR_BUTTONHOVER);
	}
	else
	{
		xui_draw_control_frame(ctx, id, &r, XUI_COLOR_BUTTON, 0);
	}
	struct region_t region;
	region_init(&region, r.x, r.y, r.h, r.h);
	xui_draw_icon(ctx, expanded ? MU_ICON_EXPANDED : MU_ICON_COLLAPSED, &region, &ctx->style.colors[XUI_COLOR_TEXT]);
	r.x += r.h - ctx->style.padding;
	r.w -= r.h - ctx->style.padding;
	xui_draw_control_text(ctx, label, &r, XUI_COLOR_TEXT, 0);

	return expanded ? XUI_RES_ACTIVE : 0;
}

int xui_header_ex(struct xui_context_t * ctx, const char * label, int opt)
{
	return header(ctx, label, 0, opt);
}

int xui_header(struct xui_context_t * ctx, const char * label)
{
	return header(ctx, label, 0, 0);
}

int xui_begin_treenode_ex(struct xui_context_t * ctx, const char * label, int opt)
{
	int res = header(ctx, label, 1, opt);
	if(res & XUI_RES_ACTIVE)
	{
		get_layout(ctx)->indent += ctx->style.indent;
		xui_push(ctx->id_stack, ctx->last_id);
	}
	return res;
}

int xui_begin_treenode(struct xui_context_t * ctx, const char * label)
{
	return xui_begin_treenode_ex(ctx, label, 0);
}

void xui_end_treenode(struct xui_context_t * ctx)
{
	get_layout(ctx)->indent -= ctx->style.indent;
	xui_pop_id(ctx);
}

#define scrollbar(ctx, c, b, width, height, x, y, w, h)                              \
  do {                                                                      \
    /* only add scrollbar if content size is larger than body */            \
    int maxscroll = height - b->h;                                            \
                                                                            \
    if (maxscroll > 0 && b->h > 0) {                                        \
      struct region_t base, thumb;                                                  \
      unsigned int id = xui_get_id(ctx, "!scrollbar" #y, 11);                       \
                                                                            \
      /* get sizing / positioning */                                        \
      base = *b;                                                            \
      base.x = b->x + b->w;                                                 \
      base.w = ctx->style.scrollbar_size;                                  \
                                                                            \
      /* handle input */                                                    \
      xui_update_control(ctx, id, &base, 0);                                  \
      if (ctx->focus == id && ctx->mouse_down == MU_MOUSE_LEFT) {           \
        c->scroll_abc.y += ctx->mouse_delta_y * height / base.h;                \
      }                                                                     \
      /* clamp scroll to limits */                                          \
      c->scroll_abc.y = clamp(c->scroll_abc.y, 0, maxscroll);  	                \
                                                                            \
      /* draw base and thumb */                                             \
      ctx->draw_frame(ctx, &base, XUI_COLOR_SCROLLBASE);                      \
      thumb = base;                                                         \
      thumb.h = max(ctx->style.thumb_size, base.h * b->h / height);       \
      thumb.y += c->scroll_abc.y * (base.h - thumb.h) / maxscroll;            \
      ctx->draw_frame(ctx, &thumb, XUI_COLOR_SCROLLTHUMB);                    \
                                                                            \
      /* set this as the scroll_target (will get scrolled on mousewheel) */ \
      /* if the mouse is over it */                                         \
      if (xui_mouse_over(ctx, b)) { ctx->scroll_target = c; }             \
    } else {                                                                \
      c->scroll_abc.y = 0;                                                    \
    }                                                                       \
  } while (0)

static void scrollbars(struct xui_context_t * ctx, struct xui_container_t * c, struct region_t * body)
{
	int sz = ctx->style.scrollbar_size;
	int width = c->content_width;
	int height = c->content_height;
	width += ctx->style.padding * 2;
	height += ctx->style.padding * 2;
	xui_push_clip(ctx, body);
	if(height > c->body.h)
		body->w -= sz;
	if(width > c->body.w)
		body->h -= sz;
	scrollbar(ctx, c, body, width, height, x, y, w, h);
	scrollbar(ctx, c, body, width, height, y, x, h, w);
	xui_pop_clip(ctx);
}

static void push_container_body(struct xui_context_t * ctx, struct xui_container_t * c, struct region_t * body, int opt)
{
	struct region_t r;
	if(~opt & XUI_OPT_NOSCROLL)
		scrollbars(ctx, c, body);
	region_expand(&r, body, -ctx->style.padding);
	push_layout(ctx, &r, c->scroll_abc.x, c->scroll_abc.y);
	region_clone(&c->body, body);
}

static void begin_root_container(struct xui_context_t * ctx, struct xui_container_t * c)
{
	xui_push(ctx->container_stack, c);
	xui_push(ctx->root_list, c);
	c->head = push_jump(ctx, NULL);
	if(region_hit(&c->rect, ctx->mouse_pos_x, ctx->mouse_pos_y) && (!ctx->next_hover_root || c->zindex > ctx->next_hover_root->zindex))
		ctx->next_hover_root = c;
	xui_push(ctx->clip_stack, unclipped_region);
}

static void end_root_container(struct xui_context_t * ctx)
{
	struct xui_container_t * c = xui_get_current_container(ctx);
	c->tail = push_jump(ctx, NULL);
	c->head->jump.addr = ctx->command_list.items + ctx->command_list.idx;
	xui_pop_clip(ctx);
	pop_container(ctx);
}

int xui_begin_window_ex(struct xui_context_t * ctx, const char * title, struct region_t * r, int opt)
{
	unsigned int id = xui_get_id(ctx, title, strlen(title));
	struct xui_container_t * c = get_container(ctx, id, opt);
	struct region_t body, region;

	if(!c || !c->open)
		return 0;
	xui_push(ctx->id_stack, id);

	if(c->rect.w == 0)
		region_clone(&c->rect, r);
	begin_root_container(ctx, c);
	region_clone(&body, &c->rect);
	region_clone(&region, &c->rect);

	if(~opt & XUI_OPT_NOFRAME)
		ctx->draw_frame(ctx, &region, XUI_COLOR_WINDOW);

	if(~opt & XUI_OPT_NOTITLE)
	{
		struct region_t tr;
		region_clone(&tr, &region);
		tr.h = ctx->style.title_height;
		ctx->draw_frame(ctx, &tr, XUI_COLOR_TITLEBG);

		if(~opt & XUI_OPT_NOTITLE)
		{
			unsigned int id = xui_get_id(ctx, "!title", 6);
			xui_update_control(ctx, id, &tr, opt);
			xui_draw_control_text(ctx, title, &tr, XUI_COLOR_TITLETEXT, opt);
			if(id == ctx->focus && ctx->mouse_down == MU_MOUSE_LEFT)
			{
				c->rect.x += ctx->mouse_delta_x;
				c->rect.y += ctx->mouse_delta_y;
			}
			body.y += tr.h;
			body.h -= tr.h;
		}

		if(~opt & XUI_OPT_NOCLOSE)
		{
			unsigned int id = xui_get_id(ctx, "!close", 6);
			struct region_t r;
			region_init(&r, tr.x + tr.w - tr.h, tr.y, tr.h, tr.h);
			tr.w -= r.w;
			xui_draw_icon(ctx, MU_ICON_CLOSE, &r, &ctx->style.colors[XUI_COLOR_TITLETEXT]);
			xui_update_control(ctx, id, &r, opt);
			if((ctx->mouse_pressed == MU_MOUSE_LEFT) && (id == ctx->focus))
				c->open = 0;
		}
	}

	push_container_body(ctx, c, &body, opt);

	if(~opt & XUI_OPT_NORESIZE)
	{
		int sz = ctx->style.title_height;
		unsigned int id = xui_get_id(ctx, "!resize", 7);
		struct region_t r;
		region_init(&r, region.x + region.w - sz, region.y + region.h - sz, sz, sz);
		xui_update_control(ctx, id, &r, opt);
		if(id == ctx->focus && ctx->mouse_down == MU_MOUSE_LEFT)
		{
			c->rect.w = max(96, c->rect.w + ctx->mouse_delta_x);
			c->rect.h = max(64, c->rect.h + ctx->mouse_delta_y);
		}
	}

	if(opt & XUI_OPT_AUTOSIZE)
	{
		struct region_t r = get_layout(ctx)->body;
		c->rect.w = c->content_width + (c->rect.w - r.w);
		c->rect.h = c->content_height + (c->rect.h - r.h);
	}

	if((opt & XUI_OPT_POPUP) && ctx->mouse_pressed && ctx->hover_root != c)
		c->open = 0;

	xui_push_clip(ctx, &c->body);
	return XUI_RES_ACTIVE;
}

int xui_begin_window(struct xui_context_t * ctx, const char * title, struct region_t * r)
{
	return xui_begin_window_ex(ctx, title, r, 0);
}

void xui_end_window(struct xui_context_t * ctx)
{
	xui_pop_clip(ctx);
	end_root_container(ctx);
}

int xui_begin_popup(struct xui_context_t * ctx, const char * name)
{
	int opt = XUI_OPT_POPUP | XUI_OPT_AUTOSIZE | XUI_OPT_NORESIZE | XUI_OPT_NOSCROLL | XUI_OPT_NOTITLE | XUI_OPT_CLOSED;
	return xui_begin_window_ex(ctx, name, &(struct region_t){0, 0, 0, 0}, opt);
}

void xui_end_popup(struct xui_context_t * ctx)
{
	xui_end_window(ctx);
}

void xui_open_popup(struct xui_context_t * ctx, const char * name)
{
	struct xui_container_t * c = xui_get_container(ctx, name);
	ctx->hover_root = ctx->next_hover_root = c;
	region_init(&c->rect, ctx->mouse_pos_x, ctx->mouse_pos_y, 1, 1);
	c->open = 1;
	xui_set_front(ctx, c);
}

void xui_begin_panel_ex(struct xui_context_t * ctx, const char * name, int opt)
{
	struct xui_container_t * c;
	xui_push_id(ctx, name, strlen(name));
	c = get_container(ctx, ctx->last_id, opt);
	region_clone(&c->rect, xui_layout_next(ctx));
	if(~opt & XUI_OPT_NOFRAME)
		ctx->draw_frame(ctx, &c->rect, XUI_COLOR_PANEL);
	xui_push(ctx->container_stack, c);
	push_container_body(ctx, c, &c->rect, opt);
	xui_push_clip(ctx, &c->body);
}

void xui_begin_panel(struct xui_context_t * ctx, const char * name)
{
	xui_begin_panel_ex(ctx, name, 0);
}

void xui_end_panel(struct xui_context_t * ctx)
{
	xui_pop_clip(ctx);
	pop_container(ctx);
}