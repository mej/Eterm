/*--------------------------------*-C-*---------------------------------*
 * File:	menubar.c
 *
 * Copyright 1996,97
 * mj olesen <olesen@me.QueensU.CA> Queen's Univ at Kingston
 *
 * You can do what you like with this source code provided you don't make
 * money from it and you include an unaltered copy of this message
 * (including the copyright).  As usual, the author accepts no
 * responsibility for anything, nor does he guarantee anything whatsoever.
 *
 *----------------------------------------------------------------------*/

static const char cvs_ident[] = "$Id$";

#include "main.h"
#include <string.h>

#ifdef HAVE_SYS_SELECT_H
# include <sys/select.h>
#endif
#ifdef TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else
# ifdef HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  include <time.h>
# endif
#endif

#include "menubar.h"
#include "command.h"
#include "debug.h"
#include "../libmej/debug.h"
#include "mem.h"
#include "misc.h"
#ifdef PIXMAP_MENUBAR
# include "pixmap.h"
# include "options.h"
#endif

#ifdef KANJI
# ifdef NO_XLOCALE
#  include <locale.h>
# else
#  include <X11/Xlocale.h>
# endif
static XFontSet fontset = 0;

#endif

int delay_menu_drawing;

extern XSetWindowAttributes Attributes;

#ifdef PIXMAP_MENUBAR
pixmap_t mbPixmap, mb_selPixmap;

# ifdef USE_IMLIB
imlib_t imlib_mb, imlib_ms;

# endif
#endif

#if (MENUBAR_MAX)
menuitem_t *menuitem_find(menu_t * menu, char *name);
void menuitem_free(menu_t * menu, menuitem_t * item);
int action_type(action_t * action, unsigned char *str);
int action_dispatch(action_t * action);
int menuarrow_find(char name);
void menuarrow_free(char name);
void menuarrow_add(char *string);
menuitem_t *menuitem_add(menu_t * menu, char *name, char *name2, char *action);
char *menu_find_base(menu_t ** menu, char *path);
menu_t *menu_delete(menu_t * menu);
menu_t *menu_add(menu_t * parent, char *path);
void drawbox_menubar(int x, int len, int state);
void drawtriangle(int x, int y, int state);
void drawbox_menuitem(int y, int state);
void print_menu_ancestors(menu_t * menu);
void print_menu_descendants(menu_t * menu);
void menu_show(void);
void menu_display(void (*update) (void));
void menu_hide_all(void);
void menu_hide(void);
void menu_clear(menu_t * menu);
void menubar_clear(void);
bar_t *menubar_find(const char *name);
int menubar_push(const char *name);
void menubar_remove(const char *name);
void action_decode(FILE * fp, action_t * act);
void menu_dump(FILE * fp, menu_t * menu);
void menubar_dump(FILE * fp);
void menubar_read(const char *filename);
void menubar_dispatch(char *str);
void draw_Arrows(int name, int state);
void menubar_expose(void);
int menubar_mapping(int map);
int menu_select(XButtonEvent * ev);
void menubar_select(XButtonEvent * ev);
void menubar_control(XButtonEvent * ev);

#define HSPACE		2
#define MENU_MARGIN	2
#define menu_height()	(TermWin.fheight + 2 * MENU_MARGIN)

#define MENU_DELAY_USEC	250000	/* 1/4 sec */

#define SEPARATOR_HALFHEIGHT	(SHADOW + 1)
#define SEPARATOR_HEIGHT	(2 * SEPARATOR_HALFHEIGHT)
#define isSeparator(name)	((name)[0] == '\0')

#define SEPARATOR_NAME		"-"
#define MENUITEM_BEG		'{'
#define MENUITEM_END		'}'
#define COMMENT_CHAR		'#'

#define DOT	"."
#define DOTS	".."

#define Menu_PixelWidth(menu)	(2 * SHADOW + Width2Pixel ((menu)->width + 3 * HSPACE))

static GC topShadowGC, botShadowGC, neutralGC, menubarGC;

struct menu_t;

static int menu_readonly = 1;	/* okay to alter menu? */
static int Arrows_x = 0;

static const struct {
  char name;			/* (l)eft, (u)p, (d)own, (r)ight */
  unsigned char *str;		/* str[0] = strlen (str+1) */
} Arrows[NARROWS] = {

  {
    'l', "\003\033[D"
  },
  {
    'u', "\003\033[A"
  },
  {
    'd', "\003\033[B"
  },
  {
    'r', "\003\033[C"
  }
};

#if (MENUBAR_MAX > 1)
static int Nbars = 0;
static bar_t *CurrentBar = NULL;

#else /* (MENUBAR_MAX > 1) */
static bar_t BarList;
static bar_t *CurrentBar = &BarList;

#endif /* (MENUBAR_MAX > 1) */

#ifdef PIXMAP_MENUBAR
menu_t *ActiveMenu = NULL;	/* currently active menu */

#else
static menu_t *ActiveMenu = NULL;	/* currently active menu */

#endif

#ifndef HAVE_MEMMOVE
/* Replacement for memmove() if it's not there -- mej */
inline void *
memmove(void *s1, const void *s2, size_t size)
{

  register char *tmp;
  register char *dest = (char *) s1;
  register const char *source = (const char *) s2;

  if ((tmp = (char *) malloc(size)) == NULL) {
    fprintf(stderr, "memmove(): allocation failure for %lu bytes\n",
	    size);
    exit(EXIT_FAILURE);
  }
  memcpy(tmp, source, size);
  memcpy(dest, tmp, size);
  free(tmp);
  return (dest);
}
#endif /* HAVE_MEMMOVE */

/*
 * find an item called NAME in MENU
 */
menuitem_t *
menuitem_find(menu_t * menu, char *name)
{

  menuitem_t *item;

  assert(name != NULL);
  assert(menu != NULL);

  D_MENUBAR(("menuitem_find(\"%s\", \"%s\")\n", menu->name, name));
/* find the last item in the menu, this is good for separators */
  for (item = menu->tail; item != NULL; item = item->prev) {
    if (item->entry.type == MenuSubMenu) {
      if (!strcmp(name, (item->entry.submenu.menu)->name))
	break;
    } else if ((isSeparator(name) && isSeparator(item->name)) ||
	       !strcmp(name, item->name))
      break;
  }
  return item;
}

/*
 * unlink ITEM from its MENU and free its memory
 */
void
menuitem_free(menu_t * menu, menuitem_t * item)
{

/* disconnect */
  menuitem_t *prev, *next;

  assert(menu != NULL);
  assert(item != NULL);

  D_MENUBAR(("menuitem_free(\"%s\", \"%s\")\n", menu->name, item->name));
  prev = item->prev;
  next = item->next;
  if (prev != NULL)
    prev->next = next;
  if (next != NULL)
    next->prev = prev;

/* new head, tail */
  if (menu->tail == item)
    menu->tail = prev;
  if (menu->head == item)
    menu->head = next;

  switch (item->entry.type) {
    case MenuAction:
    case MenuTerminalAction:
      FREE(item->entry.action.str);
      break;
    case MenuSubMenu:
      menu_delete(item->entry.submenu.menu);
      break;
  }
  if (item->name != NULL)
    FREE(item->name);
  if (item->name2 != NULL)
    FREE(item->name2);
  FREE(item);
}

/*
 * sort command vs. terminal actions and
 * remove the first character of STR if it's '\0'
 */
int
action_type(action_t * action, unsigned char *str)
{

  unsigned int len;

  len = parse_escaped_string(str);
  D_MENUBAR(("New string is %u bytes\n", len));

  ASSERT(action != NULL);

  if (!len)
    return -1;

/* sort command vs. terminal actions */
  action->type = MenuAction;
  if (str[0] == '\0') {
    /* the functional equivalent: memmove (str, str+1, len); */
    unsigned char *dst = (str);
    unsigned char *src = (str + 1);
    unsigned char *end = (str + len);

    while (src <= end)
      *dst++ = *src++;

    len--;			/* decrement length */
    if (str[0] != '\0')
      action->type = MenuTerminalAction;
  }
  action->str = str;
  action->len = len;

  return 0;
}

int
action_dispatch(action_t * action)
{

  assert(action != NULL);
  D_MENUBAR(("action_dispatch(\"%s\")\n", action->str));

  switch (action->type) {
    case MenuTerminalAction:
      cmd_write(action->str, action->len);
      break;

    case MenuAction:
      tt_write(action->str, action->len);
      break;

    default:
      return -1;
      break;
  }
  return 0;
}

/* return the arrow index corresponding to NAME */
int
menuarrow_find(char name)
{

  int i;

  D_MENUARROWS(("menuarrow_find(\'%c\')\n", name));

  for (i = 0; i < NARROWS; i++) {
    if (name == Arrows[i].name)
      return (i);
  }
  return (-1);
}

/* free the memory associated with arrow NAME of the current menubar */
void
menuarrow_free(char name)
{

  int i;

  D_MENUARROWS(("menuarrow_free(\'%c\')\n", name));

  if (name) {
    i = menuarrow_find(name);
    if (i >= 0) {
      action_t *act = &(CurrentBar->arrows[i]);

      switch (act->type) {
	case MenuAction:
	case MenuTerminalAction:
	  FREE(act->str);
	  act->str = NULL;
	  act->len = 0;
	  break;
      }
      act->type = MenuLabel;
    }
  } else {
    for (i = 0; i < NARROWS; i++)
      menuarrow_free(Arrows[i].name);
  }
}

void
menuarrow_add(char *string)
{

  int i;
  unsigned xtra_len;
  char *p;
  struct {
    char *str;
    int len;
  } beg = {
    NULL, 0
  }, end = {
    NULL, 0
  }, *cur, parse[NARROWS];

  D_MENUARROWS(("menuarrow_add(\"%s\")\n", string));

  memset(parse, 0, sizeof(parse));

  for (p = string; p != NULL && *p; string = p) {
    p = (string + 3);
    D_MENUARROWS(("parsing at %s\n", string));
    switch (string[1]) {
      case 'b':
	cur = &beg;
	break;
      case 'e':
	cur = &end;
	break;
      default:
	i = menuarrow_find(string[1]);
	if (i >= 0)
	  cur = &(parse[i]);
	else
	  continue;		/* not found */
	break;
    }

    string = p;
    cur->str = string;
    cur->len = 0;

    if (cur == &end) {
      p = strchr(string, '\0');
    } else {
      char *next = string;

      while (1) {
	p = strchr(next, '<');
	if (p != NULL) {
	  if (p[1] && p[2] == '>')
	    break;
	  /* parsed */
	} else {
	  if (beg.str == NULL)	/* no end needed */
	    p = strchr(next, '\0');
	  break;
	}
	next = (p + 1);
      }
    }

    if (p == NULL)
      return;
    cur->len = (p - string);
  }

#if DEBUG >= DEBUG_MENUARROWS
  if (debug_level >= DEBUG_MENUARROWS) {
    cur = &beg;
    DPRINTF1(("<b>(len %d) = %.*s\n", cur->len, cur->len, (cur->str ? cur->str : "")));
    for (i = 0; i < NARROWS; i++) {
      cur = &(parse[i]);
      DPRINTF1(("<%c>(len %d) = %.*s\n", Arrows[i].name, cur->len, cur->len, (cur->str ? cur->str : "")));
    }
    cur = &end;
    DPRINTF1(("<e>(len %d) = %.*s\n", cur->len, cur->len, (cur->str ? cur->str : "")));
  }
#endif

  xtra_len = (beg.len + end.len);
  for (i = 0; i < NARROWS; i++) {
    if (xtra_len || parse[i].len)
      menuarrow_free(Arrows[i].name);
  }

  for (i = 0; i < NARROWS; i++) {

    unsigned char *str;
    unsigned int len;

    if (!parse[i].len)
      continue;

    str = MALLOC(parse[i].len + xtra_len + 1);
    if (str == NULL)
      continue;

    len = 0;
    if (beg.len) {
      strncpy(str + len, beg.str, beg.len);
      len += beg.len;
    }
    strncpy(str + len, parse[i].str, parse[i].len);
    len += parse[i].len;

    if (end.len) {
      strncpy(str + len, end.str, end.len);
      len += end.len;
    }
    str[len] = '\0';

    D_MENUARROWS(("<%c>(len %d) = %s\n", Arrows[i].name, len, str));
    if (action_type(&(CurrentBar->arrows[i]), str) < 0)
      FREE(str);
  }
}

menuitem_t *
menuitem_add(menu_t * menu, char *name, char *name2, char *action)
{

  menuitem_t *item = NULL;
  unsigned int len;
  unsigned char found = 0;

  assert(name != NULL);
  assert(action != NULL);

  if (menu == NULL)
    return NULL;

  D_MENUBAR(("menuitem_add(\"%s\", \"%s\", \"%s\", \"%s\")\n", menu->name, name, (name2 ? name2 : "<nil>"), action));

  if (isSeparator(name)) {
    /* add separator, no action */
    name = "";
    action = "";
  } else {
    /*
     * add/replace existing menu item
     */
    item = menuitem_find(menu, name);
    if (item != NULL) {
      if (item->name2 != NULL && name2 != NULL) {
	FREE(item->name2);
	item->len2 = 0;
	item->name2 = NULL;
      }
      switch (item->entry.type) {
	case MenuAction:
	case MenuTerminalAction:
	  FREE(item->entry.action.str);
	  item->entry.action.str = NULL;
	  break;
      }
      found = 1;
    }
  }

  if (!found) {

    /* allocate a new itemect */
    if ((item = (menuitem_t *) MALLOC(sizeof(menuitem_t))) == NULL)
      return NULL;

    item->len2 = 0;
    item->name2 = NULL;

    len = strlen(name);
    item->name = MALLOC(len + 1);
    if (item->name != NULL) {
      strcpy(item->name, name);
      if (name[0] == '.' && name[1] != '.')
	len = 0;		/* hidden menu name */
    } else {
      FREE(item);
      return NULL;
    }
    item->len = len;

    /* add to tail of list */
    item->prev = menu->tail;
    item->next = NULL;

    if (menu->tail != NULL)
      (menu->tail)->next = item;
    menu->tail = item;
    /* fix head */
    if (menu->head == NULL)
      menu->head = item;
  }
  /*
   * add action
   */
  if (name2 != NULL && item->name2 == NULL) {
    len = strlen(name2);
    if (len == 0 || (item->name2 = MALLOC(len + 1)) == NULL) {
      len = 0;
      item->name2 = NULL;
    } else {
      strcpy(item->name2, name2);
    }
    item->len2 = len;
  }
  item->entry.type = MenuLabel;
  len = strlen(action);

  if (len == 0 && item->name2 != NULL) {
    action = item->name2;
    len = item->len2;
  }
  if (len) {
    unsigned char *str = MALLOC(len + 1);

    if (str == NULL) {
      menuitem_free(menu, item);
      return NULL;
    }
    strcpy(str, action);

    if (action_type(&(item->entry.action), str) < 0)
      FREE(str);
  }
  /* new item and a possible increase in width */
  if (menu->width < (item->len + item->len2))
    menu->width = (item->len + item->len2);

  return item;
}

/*
 * search for the base starting menu for NAME.
 * return a pointer to the portion of NAME that remains
 */
char *
menu_find_base(menu_t ** menu, char *path)
{

  menu_t *m = NULL;
  menuitem_t *item;

  assert(menu != NULL);
  assert(CurrentBar != NULL);

  D_MENUBAR(("menu_find_base(0x%08x, \"%s\")\n", menu, path));

  if (path[0] == '\0')
    return path;

  if (strchr(path, '/') != NULL) {
    register char *p = path;

    while ((p = strchr(p, '/')) != NULL) {
      p++;
      if (*p == '/')
	path = p;
    }
    if (path[0] == '/') {
      path++;
      *menu = NULL;
    }
    while ((p = strchr(path, '/')) != NULL) {
      p[0] = '\0';
      if (path[0] == '\0')
	return NULL;
      if (!strcmp(path, DOT)) {
	/* nothing to do */
      } else if (!strcmp(path, DOTS)) {
	if (*menu != NULL)
	  *menu = (*menu)->parent;
      } else {
	path = menu_find_base(menu, path);
	if (path[0] != '\0') {	/* not found */
	  p[0] = '/';		/* fix-up name again */
	  return path;
	}
      }

      path = (p + 1);
    }
  }
  if (!strcmp(path, DOTS)) {
    path += strlen(DOTS);
    if (*menu != NULL)
      *menu = (*menu)->parent;
    return path;
  }
  /* find this menu */
  if (*menu == NULL) {
    for (m = CurrentBar->tail; m != NULL; m = m->prev) {
      if (!strcmp(path, m->name))
	break;
    }
  } else {
    /* find this menu */
    for (item = (*menu)->tail; item != NULL; item = item->prev) {
      if (item->entry.type == MenuSubMenu &&
	  !strcmp(path, (item->entry.submenu.menu)->name)) {
	m = (item->entry.submenu.menu);
	break;
      }
    }
  }
  if (m != NULL) {
    *menu = m;
    path += strlen(path);
  }
  return path;
}

/*
 * delete this entire menu
 */
menu_t *
menu_delete(menu_t * menu)
{

  menu_t *parent = NULL, *prev, *next;
  menuitem_t *item;

  assert(CurrentBar != NULL);
  if (menu == NULL)
    return NULL;

  D_MENUBAR(("menu_delete(\"%s\")\n", menu->name));

/* delete the entire menu */
  parent = menu->parent;

/* unlink MENU */
  prev = menu->prev;
  next = menu->next;
  if (prev != NULL)
    prev->next = next;
  if (next != NULL)
    next->prev = prev;

/* fix the index */
  if (parent == NULL) {
    const int len = (menu->len + HSPACE);

    if (CurrentBar->tail == menu)
      CurrentBar->tail = prev;
    if (CurrentBar->head == menu)
      CurrentBar->head = next;

    for (next = menu->next; next != NULL; next = next->next)
      next->x -= len;
  } else {
    for (item = parent->tail; item != NULL; item = item->prev) {
      if (item->entry.type == MenuSubMenu &&
	  item->entry.submenu.menu == menu) {
	item->entry.submenu.menu = NULL;
	menuitem_free(menu->parent, item);
	break;
      }
    }
  }

  item = menu->tail;
  while (item != NULL) {
    menuitem_t *p = item->prev;

    menuitem_free(menu, item);
    item = p;
  }

  if (menu->name != NULL)
    FREE(menu->name);
  FREE(menu);

  return parent;
}

menu_t *
menu_add(menu_t * parent, char *path)
{

  menu_t *menu;

  assert(CurrentBar != NULL);

  D_MENUBAR(("menu_add(\"%s\", \"%s\")\n", (parent ? parent->name : "<nil>"), path));

  if (strchr(path, '/') != NULL) {
    register char *p;

    if (path[0] == '/') {
      /* shouldn't happen */
      path++;
      parent = NULL;
    }
    while ((p = strchr(path, '/')) != NULL) {
      p[0] = '\0';
      if (path[0] == '\0')
	return NULL;

      parent = menu_add(parent, path);
      path = (p + 1);
    }
  }
  if (!strcmp(path, DOTS))
    return (parent != NULL ? parent->parent : parent);

  if (!strcmp(path, DOT) || path[0] == '\0')
    return parent;

/* allocate a new menu */
  if ((menu = (menu_t *) MALLOC(sizeof(menu_t))) == NULL)
    return parent;

  menu->width = 0;
  menu->parent = parent;
  menu->len = strlen(path);
  menu->name = MALLOC((menu->len + 1));
  if (menu->name == NULL) {
    FREE(menu);
    return parent;
  }
  strcpy(menu->name, path);

/* initialize head/tail */
  menu->head = menu->tail = NULL;
  menu->prev = menu->next = NULL;

  menu->win = None;
  menu->x = menu->y = menu->w = menu->h = 0;
  menu->item = NULL;

/* add to tail of list */
  if (parent == NULL) {
    menu->prev = CurrentBar->tail;
    if (CurrentBar->tail != NULL)
      CurrentBar->tail->next = menu;
    CurrentBar->tail = menu;
    if (CurrentBar->head == NULL)
      CurrentBar->head = menu;	/* fix head */
    if (menu->prev)
      menu->x = (menu->prev->x + menu->prev->len + HSPACE);
  } else {
    menuitem_t *item;

    item = menuitem_add(parent, path, "", "");
    if (item == NULL) {
      FREE(menu);
      return parent;
    }
    assert(item->entry.type == MenuLabel);
    item->entry.type = MenuSubMenu;
    item->entry.submenu.menu = menu;
  }

  return menu;
}

void
drawbox_menubar(int x, int len, int state)
{

  GC top = None, bot = None;

  x = Width2Pixel(x);
  len = Width2Pixel(len + HSPACE);
  if (x >= TermWin.width)
    return;
  else if (x + len >= TermWin.width)
    len = (TermWin_TotalWidth() - x);

#ifdef MENUBAR_SHADOW_IN
  state = -state;
#endif
  switch (state) {
    case +1:
      top = topShadowGC;
      bot = botShadowGC;
      break;			/* SHADOW_OUT */
    case -1:
      top = botShadowGC;
      bot = topShadowGC;
      break;			/* SHADOW_IN */
    case 0:
      top = bot = neutralGC;
      break;			/* neutral */
  }

  if (!(menubar_is_pixmapped()))
    Draw_Shadow(menuBar.win, top, bot,
		x, 0, len, menuBar_TotalHeight());
}

void
drawtriangle(int x, int y, int state)
{

  GC top = None, bot = None;
  int w;

#ifdef MENUBAR_SHADOW_IN
  state = -state;
#endif
  switch (state) {
    case +1:
      top = topShadowGC;
      bot = botShadowGC;
      break;			/* SHADOW_OUT */
    case -1:
      top = botShadowGC;
      bot = topShadowGC;
      break;			/* SHADOW_IN */
    case 0:
      top = bot = neutralGC;
      break;			/* neutral */
  }

  w = menu_height() / 2;

  x -= (SHADOW + MENU_MARGIN) + (3 * w / 2);
  y += (SHADOW + MENU_MARGIN) + (w / 2);

  Draw_Triangle(ActiveMenu->win, top, bot, x, y, w, 'r');
}

void
drawbox_menuitem(int y, int state)
{

  GC top = None, bot = None;

#ifdef MENU_SHADOW_IN
  state = -state;
#endif
  switch (state) {
    case +1:
      top = topShadowGC;
      bot = botShadowGC;
      break;			/* SHADOW_OUT */
    case -1:
      top = botShadowGC;
      bot = topShadowGC;
      break;			/* SHADOW_IN */
    case 0:
      top = bot = neutralGC;
      break;			/* neutral */
  }

  if (!(menubar_is_pixmapped()))
    Draw_Shadow(ActiveMenu->win, top, bot,
		SHADOW + 0,
		SHADOW + y,
		ActiveMenu->w - 2 * (SHADOW),
		menu_height() + 2 * MENU_MARGIN);
  XFlush(Xdisplay);
}

#if DEBUG >= DEBUG_MENU_LAYOUT
void
print_menu_ancestors(menu_t * menu)
{

  if (menu == NULL) {
    D_MENU_LAYOUT(("Top Level menu\n"));
    return;
  }
  D_MENU_LAYOUT(("menu %s ", menu->name));
  if (menu->parent != NULL) {
    menuitem_t *item;

    for (item = menu->parent->head; item != NULL; item = item->next) {
      if (item->entry.type == MenuSubMenu &&
	  item->entry.submenu.menu == menu) {
	break;
      }
    }
    if (item == NULL) {
      fprintf(stderr, "is an orphan!\n");
      return;
    }
  }
  fprintf(stderr, "\n");
  print_menu_ancestors(menu->parent);
}

void
print_menu_descendants(menu_t * menu)
{

  menuitem_t *item;
  menu_t *parent;
  int i, level = 0;

  parent = menu;
  do {
    level++;
    parent = parent->parent;
  }
  while (parent != NULL);

  for (i = 0; i < level; i++)
    fprintf(stderr, ">");
  fprintf(stderr, "%s\n", menu->name);

  for (item = menu->head; item != NULL; item = item->next) {
    if (item->entry.type == MenuSubMenu) {
      if (item->entry.submenu.menu == NULL)
	fprintf(stderr, "> %s == NULL\n", item->name);
      else
	print_menu_descendants(item->entry.submenu.menu);
    } else {
      for (i = 0; i < level; i++)
	fprintf(stderr, "+");
      if (item->entry.type == MenuLabel)
	fprintf(stderr, "label: ");
      fprintf(stderr, "%s\n", item->name);
    }
  }

  for (i = 0; i < level; i++)
    fprintf(stderr, "<");
  fprintf(stderr, "\n");
}
#endif

/* pop up/down the current menu and redraw the menuBar button */
void
menu_show(void)
{

  int x, y, newx, newy, xright;
  menuitem_t *item;
  XSetWindowAttributes attr = Attributes;
  Window true_parent;

  if (ActiveMenu == NULL)
    return;

  attr.override_redirect = TRUE;
  x = ActiveMenu->x;
  if (ActiveMenu->parent == NULL) {
    register int h;

    drawbox_menubar(x, ActiveMenu->len, -1);
    x = Width2Pixel(x);

    ActiveMenu->y = 1;
    ActiveMenu->w = Menu_PixelWidth(ActiveMenu);

    /* find the height */
    for (h = 0, item = ActiveMenu->head; item != NULL; item = item->next) {
      if (isSeparator(item->name))
	h += SEPARATOR_HEIGHT;
      else
	h += menu_height();
    }
    ActiveMenu->h = h + 2 * (SHADOW + MENU_MARGIN);
  }
  if (ActiveMenu->win == None) {
    XTranslateCoordinates(Xdisplay, TermWin.vt, Xroot, 0, 0, &newx, &newy, &true_parent);
    if (x < newx) {
      x += newx;
    }
    if (x + ActiveMenu->w >= (ScreenOfDisplay(Xdisplay, Xscreen))->width) {
      register int dx = ((ActiveMenu->w + x) - (ScreenOfDisplay(Xdisplay, Xscreen))->width);

      x -= dx;
      ActiveMenu->x -= dx;
    }
    y = ActiveMenu->y + newy;
    if (y + ActiveMenu->h >= (ScreenOfDisplay(Xdisplay, Xscreen))->height) {
      register int dy = ((ActiveMenu->h + y) - (ScreenOfDisplay(Xdisplay, Xscreen))->height);

      y -= dy;
      ActiveMenu->y -= dy;
    }
    ActiveMenu->win = XCreateWindow(Xdisplay, Xroot,
				    x,
				    y,
				    ActiveMenu->w,
				    ActiveMenu->h,
				    0,
				    Xdepth, InputOutput,
				    DefaultVisual(Xdisplay, Xscreen),
				    CWBackPixel | CWBorderPixel
				    | CWColormap | CWOverrideRedirect
				    | CWSaveUnder | CWBackingStore,
				    &attr
	);

    XMapWindow(Xdisplay, ActiveMenu->win);
  }
  if (!(menubar_is_pixmapped()))
    Draw_Shadow(ActiveMenu->win,
		topShadowGC, botShadowGC,
		0, 0,
		ActiveMenu->w, ActiveMenu->h);

  /* determine the correct right-alignment */
  for (xright = 0, item = ActiveMenu->head; item != NULL; item = item->next) {
    if (item->len2 > xright) {
      xright = item->len2;
    }
  }
  D_MENU_LAYOUT(("xright == %d\n", xright));

  for (y = 0, item = ActiveMenu->head; item != NULL; item = item->next) {

    const int xoff = (SHADOW + Width2Pixel(HSPACE) / 2);
    const int yoff = (SHADOW + MENU_MARGIN);
    register int h;
    GC gc = menubarGC;

    if (isSeparator(item->name)) {
      if (!(menubar_is_pixmapped()))
	Draw_Shadow(ActiveMenu->win,
		    topShadowGC, botShadowGC,
		    xoff,
		    yoff + y + SEPARATOR_HALFHEIGHT,
		    ActiveMenu->w - (2 * xoff),
		    0);
      h = SEPARATOR_HEIGHT;
    } else {
      char *name = item->name;
      int len = item->len;

      if (item->entry.type == MenuLabel) {
	gc = botShadowGC;
      } else if (item->entry.type == MenuSubMenu) {
	register int x1, y1;
	menuitem_t *it;
	menu_t *menu = item->entry.submenu.menu;

	drawtriangle(ActiveMenu->w, y, +1);

	name = menu->name;
	len = menu->len;

	y1 = ActiveMenu->y + y;

	/* place sub-menu at midpoint of parent menu */
	menu->w = Menu_PixelWidth(menu);
	x1 = ActiveMenu->w / 2;

	/* right-flush menu if it's too small */
	if (x1 > menu->w)
	  x1 += (x1 - menu->w);
	x1 += x;

	/* find the height of this submenu */
	for (h = 0, it = menu->head; it != NULL; it = it->next) {
	  if (isSeparator(it->name))
	    h += SEPARATOR_HEIGHT;
	  else
	    h += menu_height();
	}
	menu->h = h + 2 * (SHADOW + MENU_MARGIN);

	menu->x = x1;
	menu->y = y1;
      } else if (item->name2 && !strcmp(name, item->name2))
	name = NULL;

      if (len && name) {
	D_MENU_LAYOUT(("len == %d, name == %s\n", len, name));
#ifdef KANJI
	if (fontset)
	  XmbDrawString(Xdisplay,
			ActiveMenu->win, fontset, gc,
			xoff,
			yoff + y + menu_height() - (MENU_MARGIN + TermWin.font->descent),
			name, len);
	else
#endif
	  XDrawString(Xdisplay,
		      ActiveMenu->win, gc,
		      xoff,
		      yoff + y + menu_height() - (MENU_MARGIN + TermWin.font->descent),
		      name, len);
      }
      len = item->len2;
      name = item->name2;
      if (len && name) {
	D_MENU_LAYOUT(("len2 == %d, name2 == %s\n", len, name));
#ifdef KANJI
	if (fontset)
	  XmbDrawString(Xdisplay,
			ActiveMenu->win, fontset, gc,
			ActiveMenu->w - (xoff + Width2Pixel(xright)),
			yoff + y + menu_height() - (MENU_MARGIN + TermWin.font->descent),
			name, len);
	else
#endif
	  XDrawString(Xdisplay,
		      ActiveMenu->win, gc,
		      ActiveMenu->w - (xoff + Width2Pixel(xright)),
		      yoff + y + menu_height() - (MENU_MARGIN + TermWin.font->descent),
		      name, len);
      }
      h = menu_height();
    }
    y += h;
  }
}
void
menu_display(void (*update) (void))
{

  D_MENUBAR(("menu_display(0x%08x)\n", update));

  if (ActiveMenu == NULL)
    return;

  if (ActiveMenu->win != None) {
    XDestroyWindow(Xdisplay, ActiveMenu->win);
    ActiveMenu->win = None;
  }
  ActiveMenu->item = NULL;

  if (ActiveMenu->parent == NULL)
    drawbox_menubar(ActiveMenu->x, ActiveMenu->len, +1);
  ActiveMenu = ActiveMenu->parent;
  update();
}

void
menu_hide_all(void)
{

  D_MENUBAR(("menu_hide_all()\n"));
  menu_display(menu_hide_all);
} void
menu_hide(void)
{

  D_MENUBAR(("menu_hide()\n"));
  menu_display(menu_show);
} void

menu_clear(menu_t * menu)
{

  D_MENUBAR(("menu_clear(\"%s\")\n", (menu ? menu->name : "<nil>")));

  if (menu != NULL) {
    menuitem_t *item = menu->tail;

    while (item != NULL) {
      menuitem_free(menu, item);
      /* it didn't get freed ... why? */
      if (item == menu->tail)
	return;
      item = menu->tail;
    }
    menu->width = 0;
  }
}

void
menubar_clear(void)
{

  if (CurrentBar != NULL) {
    menu_t *menu = CurrentBar->tail;

    while (menu != NULL) {
      menu_t *prev = menu->prev;

      menu_delete(menu);
      menu = prev;
    }
    CurrentBar->head = CurrentBar->tail = ActiveMenu = NULL;

    if (CurrentBar->title) {
      FREE(CurrentBar->title);
      CurrentBar->title = NULL;
    }
    menuarrow_free(0);		/* remove all arrow functions */
  }
  ActiveMenu = NULL;
}

#if (MENUBAR_MAX > 1)
/* find if menu already exists */
bar_t *
menubar_find(const char *name)
{

  bar_t *bar = CurrentBar;

  D_MENUBAR_STACKING(("looking for [menu:%s]...\n", name ? name : "(nil)"));
  if (bar == NULL || name == NULL)
    return NULL;

  if (strlen(name) && strcmp(name, "*")) {
    do {
      if (!strcmp(bar->name, name)) {
	D_MENUBAR_STACKING(("Found!\n"));
	return bar;
      }
      bar = bar->next;
    }
    while (bar != CurrentBar);
    bar = NULL;
  }
  D_MENUBAR_STACKING(("%s found!\n", (bar ? "" : " NOT")));
  return bar;
}

int
menubar_push(const char *name)
{

  int ret = 1;
  bar_t *bar;

  if (CurrentBar == NULL) {
    /* allocate first one */
    bar = (bar_t *) MALLOC(sizeof(bar_t));

    if (bar == NULL)
      return 0;

    memset(bar, 0, sizeof(bar_t));
    /* circular linked-list */
    bar->next = bar->prev = bar;
    bar->head = bar->tail = NULL;
    bar->title = NULL;
    CurrentBar = bar;
    Nbars++;

    menubar_clear();
  } else {
    /* find if menu already exists */
    bar = menubar_find(name);
    if (bar != NULL) {
      /* found it, use it */
      CurrentBar = bar;
    } else {
      /* create if needed, or reuse the existing empty menubar */
      if (CurrentBar->head != NULL) {
	/* need to malloc another one */
	if (Nbars < MENUBAR_MAX)
	  bar = (bar_t *) MALLOC(sizeof(bar_t));
	else
	  bar = NULL;

	/* malloc failed or too many menubars, reuse another */
	if (bar == NULL) {
	  bar = CurrentBar->next;
	  ret = -1;
	} else {
	  bar->head = bar->tail = NULL;
	  bar->title = NULL;

	  bar->next = CurrentBar->next;
	  CurrentBar->next = bar;
	  bar->prev = CurrentBar;
	  bar->next->prev = bar;

	  Nbars++;
	}
	CurrentBar = bar;

      }
      menubar_clear();
    }
  }

/* give menubar this name */
  strncpy(CurrentBar->name, name, MAXNAME);
  CurrentBar->name[MAXNAME - 1] = '\0';

  return ret;
}

/* switch to a menu called NAME and remove it */
void
menubar_remove(const char *name)
{

  bar_t *bar;

  if ((bar = menubar_find(name)) == NULL)
    return;
  CurrentBar = bar;

  do {
    menubar_clear();
    /*
     * pop a menubar, clean it up first
     */
    if (CurrentBar != NULL) {
      bar_t *prev = CurrentBar->prev;
      bar_t *next = CurrentBar->next;

      if (prev == next && prev == CurrentBar) {		/* only 1 left */
	prev = NULL;
	Nbars = 0;		/* safety */
      } else {
	next->prev = prev;
	prev->next = next;
	Nbars--;
      }

      FREE(CurrentBar);
      CurrentBar = prev;
    }
  }
  while (CurrentBar && !strcmp(name, "*"));
}

void
action_decode(FILE * fp, action_t * act)
{

  unsigned char *str;
  short len;

  if (act == NULL || (len = act->len) == 0 || (str = act->str) == NULL)
    return;

  if (act->type == MenuTerminalAction) {
    fprintf(fp, "^@");
    /* can strip trailing ^G from XTerm sequence */
    if (str[0] == 033 && str[1] == ']' && str[len - 1] == 007)
      len--;
  } else if (str[0] == 033) {
    switch (str[1]) {
      case '[':
      case ']':
	break;

      case 'x':
	/* can strip trailing '\r' from M-x sequence */
	if (str[len - 1] == '\r')
	  len--;
	/* drop */

      default:
	fprintf(fp, "M-");	/* meta prefix */
	str++;
	len--;
	break;
    }
  }
/*
 * control character form is preferred, since backslash-escaping
 * can be really ugly looking when the backslashes themselves also
 * have to be escaped to avoid Shell (or whatever scripting
 * language) interpretation
 */
  while (len > 0) {
    unsigned char ch = *str++;

    switch (ch) {
      case 033:
	fprintf(fp, "\\e");
	break;			/* escape */
      case '\r':
	fprintf(fp, "\\r");
	break;			/* carriage-return */
      case '\\':
	fprintf(fp, "\\\\");
	break;			/* backslash */
      case '^':
	fprintf(fp, "\\^");
	break;			/* caret */
      case 127:
	fprintf(fp, "^?");
      default:
	if (ch <= 31)
	  fprintf(fp, "^%c", ('@' + ch));
	else if (ch > 127)
	  fprintf(fp, "\\%o", ch);
	else
	  fprintf(fp, "%c", ch);
	break;
    }
    len--;
  }
  fprintf(fp, "\n");
}

void
menu_dump(FILE * fp, menu_t * menu)
{

  menuitem_t *item;

/* create a new menu and clear it */
  fprintf(fp, (menu->parent ? "./%s/*\n" : "/%s/*\n"), menu->name);

  for (item = menu->head; item != NULL; item = item->next) {
    switch (item->entry.type) {
      case MenuSubMenu:
	if (item->entry.submenu.menu == NULL)
	  fprintf(fp, "> %s == NULL\n", item->name);
	else
	  menu_dump(fp, item->entry.submenu.menu);
	break;

      case MenuLabel:
	fprintf(fp, "{%s}\n",
		(strlen(item->name) ? item->name : "-"));
	break;

      case MenuTerminalAction:
      case MenuAction:
	fprintf(fp, "{%s}", item->name);
	if (item->name2 != NULL && strlen(item->name2))
	  fprintf(fp, "{%s}", item->name2);
	fprintf(fp, "\t");
	action_decode(fp, &(item->entry.action));
	break;
    }
  }
  fprintf(fp, (menu->parent ? "../\n" : "/\n\n"));
}

void
menubar_dump(FILE * fp)
{

  bar_t *bar = CurrentBar;
  time_t t;

  if (bar == NULL || fp == NULL)
    return;
  time(&t);

  fprintf(fp,
	  "# " APL_NAME " (%s)  Pid: %u\n# Date: %s\n\n",
	  rs_name, (unsigned int) getpid(), ctime(&t));

/* dump in reverse order */
  bar = CurrentBar->prev;
  do {
    menu_t *menu;
    int i;

    fprintf(fp, "[menu:%s]\n", bar->name);

    if (bar->title != NULL)
      fprintf(fp, "[title:%s]\n", bar->title);

    for (i = 0; i < NARROWS; i++) {
      switch (bar->arrows[i].type) {
	case MenuTerminalAction:
	case MenuAction:
	  fprintf(fp, "<%c>", Arrows[i].name);
	  action_decode(fp, &(bar->arrows[i]));
	  break;
      }
    }
    fprintf(fp, "\n");

    for (menu = bar->head; menu != NULL; menu = menu->next)
      menu_dump(fp, menu);

    fprintf(fp, "\n[done:%s]\n\n", bar->name);
    bar = bar->prev;
  }
  while (bar != CurrentBar->prev);
}
#endif /* (MENUBAR_MAX > 1) */

/*
 * read in menubar commands from FILENAME
 * ignore all input before the tag line [menu] or [menu:???]
 *
 * Note that since File_find () is used, FILENAME can be semi-colon
 * delimited such that the second part can refer to a tag
 * so that a large `database' of menus can be collected together
 *
 * FILENAME = "file"
 * FILENAME = "file;"
 *      read `file' starting with first [menu] or [menu:???] line
 *
 * FILENAME = "file;tag"
 *      read `file' starting with [menu:tag]
 */
void
menubar_read(const char *filename)
{

/* read in a menu from a file */
  FILE *fp;
  char buffer[256];
  char *p, *tag = NULL;
  const char *file;

  if (!filename || !strlen(filename))
    return;

  file = find_file(filename, ".menu");
  if (file == NULL || (fp = fopen(file, "rb")) == NULL) {
    return;
  }
#if (MENUBAR_MAX > 1)
  /* semi-colon delimited */
  if ((tag = strchr(filename, ';')) != NULL) {
    tag++;
    if (*tag == '\0') {
      tag = NULL;
    }
  }
#endif /* (MENUBAR_MAX > 1) */

  D_MENUBAR(("looking for [menu:%s]\n", tag ? tag : "(nil)"));

  while ((p = fgets(buffer, sizeof(buffer), fp)) != NULL) {

    int n;

    D_MENUBAR(("Got \"%s\"\n", p));

    if ((n = str_leading_match(p, "[menu")) != 0) {
      if (tag) {
	/* looking for [menu:tag] */
	if (p[n] == ':' && p[n + 1] != ']') {
	  n++;
	  n += str_leading_match(p + n, tag);
	  if (p[n] == ']') {
	    D_MENUBAR(("[menu:%s]\n", tag));
	    break;
	  }
	}
      } else if (p[n] == ':' || p[n] == ']')
	break;
    }
  }

/* found [menu], [menu:???] tag */
  while (p != NULL) {

    int n;

    D_MENUBAR(("read line = %s\n", p));

    /* looking for [done:tag] or [done:] */
    if ((n = str_leading_match(p, "[done")) != 0) {
      if (p[n] == ']') {
	menu_readonly = 1;
	break;
      } else if (p[n] == ':') {
	n++;
	if (p[n] == ']') {
	  menu_readonly = 1;
	  break;
	} else if (tag) {
	  n += str_leading_match(p + n, tag);
	  if (p[n] == ']') {
	    D_MENUBAR(("[done:%s]\n", tag));
	    menu_readonly = 1;
	    break;
	  }
	} else {
	  /* what? ... skip this line */
	  p[0] = COMMENT_CHAR;
	}
      }
    }
    /*
     * remove leading/trailing space
     * and strip-off leading/trailing quotes
     * skip blank or comment lines
     */
    p = str_trim(p);
    if (p != NULL && *p && *p != COMMENT_CHAR) {
      menu_readonly = 0;	/* if case we read another file */
      menubar_dispatch(p);
    }
    /* get another line */
    p = fgets(buffer, sizeof(buffer), fp);
  }

  fclose(fp);
}

/*
 * user interface for building/deleting and otherwise managing menus
 */
void
menubar_dispatch(char *str)
{

  static menu_t *BuildMenu = NULL;	/* the menu currently being built */
  int n, cmd;
  char *path, *name, *name2;

  D_MENUBAR(("menubar_dispatch(%s) called\n", str));

  if (menubar_visible() && ActiveMenu != NULL)
    menubar_expose();
  else
    ActiveMenu = NULL;

  cmd = *str;
  switch (cmd) {
    case '.':
    case '/':			/* absolute & relative path */
    case MENUITEM_BEG:		/* menuitem */
      /* add `+' prefix for these cases */
      cmd = '+';
      break;

    case '+':
    case '-':
      str++;			/* skip cmd character */
      break;

    case '<':
#if (MENUBAR_MAX > 1)
      if (CurrentBar == NULL)
	break;
#endif /* (MENUBAR_MAX > 1) */
      if (str[1] && str[2] == '>')	/* arrow commands */
	menuarrow_add(str);
      break;

    case '=':
      D_MENUBAR(("Setting title\n"));
      str++;
      if (CurrentBar != NULL && !menu_readonly) {
	if (*str) {
	  name = REALLOC(CurrentBar->title, strlen(str) + 1);
	  if (name != NULL) {
	    strcpy(name, str);
	    CurrentBar->title = name;
	  }
	  menubar_expose();
	} else {
	  FREE(CurrentBar->title);
	  CurrentBar->title = NULL;
	}
      }
      break;

    case '[':			/* extended command */
      while (str[0] == '[') {
	char *next = (++str);	/* skip leading '[' */

	if (str[0] == ':') {	/* [:command:] */
	  do {
	    next++;
	    if ((next = strchr(next, ':')) == NULL)
	      return;		/* parse error */
	  } while (next[1] != ']');

	  /* remove and skip ':]' */
	  *next = '\0';
	  next += 2;
	} else {
	  if ((next = strchr(next, ']')) == NULL)
	    return;		/* parse error */
	  /* remove and skip ']' */
	  *next = '\0';
	  next++;
	}

	if (str[0] == ':') {
	  int saved;

	  /* try and dispatch it, regardless of read/write status */
	  D_MENUBAR(("Ignoring read-only status to parse command %s\n", str + 1));
	  saved = menu_readonly;
	  menu_readonly = 0;
	  menubar_dispatch(str + 1);
	  menu_readonly = saved;
	}
	/* these ones don't require menu stacking */
	else if (!strcmp(str, "clear")) {
	  D_MENUBAR(("Extended command \"clear\"\n"));
	  menubar_clear();
	} else if (!strcmp(str, "done") || str_leading_match(str, "done:")) {
	  D_MENUBAR(("Extended command \"done\"\n"));
	  menu_readonly = 1;
	} else if (!strcmp(str, "show")) {
	  D_MENUBAR(("Extended command \"show\"\n"));
	  map_menuBar(1);
	  menu_readonly = 1;
	} else if (!strcmp(str, "hide")) {
	  D_MENUBAR(("Extended command \"hide\"\n"));
	  map_menuBar(0);
	  menu_readonly = 1;
	} else if ((n = str_leading_match(str, "read:")) != 0) {
	  /* read in a menu from a file */
	  D_MENUBAR(("Extended command \"read\"\n"));
	  str += n;
	  menubar_read(str);
	} else if ((n = str_leading_match(str, "echo:")) != 0) {
	  D_MENUBAR(("Extended command \"echo\"\n"));
	  str += n;
	  tt_write(str, strlen(str));
	  tt_write("\r", 1);
	} else if ((n = str_leading_match(str, "apptitle:")) != 0) {
	  D_MENUBAR(("Extended command \"apptitle\"\n"));
	  str += n;
	  xterm_seq(XTerm_title, str);
	} else if ((n = str_leading_match(str, "title:")) != 0) {
	  D_MENUBAR(("Extended command \"title\"\n"));
	  str += n;
	  if (CurrentBar != NULL && !menu_readonly) {
	    if (*str) {
	      name = REALLOC(CurrentBar->title, strlen(str) + 1);
	      if (name != NULL) {
		strcpy(name, str);
		CurrentBar->title = name;
	      }
	      menubar_expose();
	    } else {
	      FREE(CurrentBar->title);
	      CurrentBar->title = NULL;
	    }
	  }
	} else if ((n = str_leading_match(str, "pixmap:")) != 0) {
	  D_MENUBAR(("Extended command \"pixmap\"\n"));
	  str += n;
	  xterm_seq(XTerm_Pixmap, str);
	}
#if (MENUBAR_MAX > 1)
	else if ((n = str_leading_match(str, "rm")) != 0) {
	  D_MENUBAR(("Extended command \"rm\"\n"));
	  str += n;
	  switch (str[0]) {
	    case ':':
	      str++;
	      menubar_remove(str);
	      break;

	    case '\0':
	      menubar_remove(str);
	      break;

	    case '*':
	      menubar_remove(str);
	      break;
	  }
	  menu_readonly = 1;
	} else if ((n = str_leading_match(str, "menu")) != 0) {
	  D_MENUBAR(("Extended command \"menu\"\n"));
	  str += n;
	  switch (str[0]) {
	    case ':':
	      str++;
	      /* add/access menuBar */
	      if (*str != '\0' && *str != '*')
		menubar_push(str);
	      break;
	    default:
	      if (CurrentBar == NULL) {
		menubar_push("default");
	      }
	  }


	  if (CurrentBar != NULL)
	    menu_readonly = 0;	/* allow menu build commands */
	} else if (!strcmp(str, "dump")) {
	  /* dump current menubars to a file */
	  FILE *fp;

	  /* enough space to hold the results */
	  char buffer[32];

	  D_MENUBAR(("Extended command \"dump\"\n"));
	  sprintf(buffer, "/tmp/" APL_NAME "-%u",
		  (unsigned int) getpid());

	  if ((fp = fopen(buffer, "wb")) != NULL) {
	    xterm_seq(XTerm_title, buffer);
	    menubar_dump(fp);
	    fclose(fp);
	  }
	} else if (!strcmp(str, "next")) {
	  if (CurrentBar) {
	    CurrentBar = CurrentBar->next;
	    menu_readonly = 1;
	  }
	} else if (!strcmp(str, "prev")) {
	  if (CurrentBar) {
	    CurrentBar = CurrentBar->prev;
	    menu_readonly = 1;
	  }
	} else if (!strcmp(str, "swap")) {
	  /* swap the top 2 menus */
	  if (CurrentBar) {
	    bar_t *prev = CurrentBar->prev;
	    bar_t *next = CurrentBar->next;

	    prev->next = next;
	    next->prev = prev;

	    CurrentBar->next = prev;
	    CurrentBar->prev = prev->prev;

	    prev->prev->next = CurrentBar;
	    prev->prev = CurrentBar;

	    CurrentBar = prev;
	    menu_readonly = 1;
	  }
	}
#endif /* (MENUBAR_MAX > 1) */
	str = next;

	BuildMenu = ActiveMenu = NULL;
	menubar_expose();
	D_MENUBAR_STACKING(("menus are read%s\n", menu_readonly ? "only" : "/write"));
      }
      return;
      break;
  }

#if (MENUBAR_MAX > 1)
  if (CurrentBar == NULL)
    return;
  if (menu_readonly) {
    D_MENUBAR_STACKING(("menus are read%s\n", menu_readonly ? "only" : "/write"));
    return;
  }
#endif /* (MENUBAR_MAX > 1) */

  switch (cmd) {
    case '+':
    case '-':
      path = name = str;

      name2 = NULL;
      /* parse STR, allow spaces inside (name)  */
      if (path[0] != '\0') {
	name = strchr(path, MENUITEM_BEG);
	str = strchr(path, MENUITEM_END);
	if (name != NULL || str != NULL) {
	  if (name == NULL || str == NULL || str <= (name + 1)
	      || (name > path && name[-1] != '/')) {
	    print_error("menu error <%s>\n", path);
	    break;
	  }
	  if (str[1] == MENUITEM_BEG) {
	    name2 = (str + 2);
	    str = strchr(name2, MENUITEM_END);

	    if (str == NULL) {
	      print_error("menu error <%s>\n", path);
	      break;
	    }
	    name2[-2] = '\0';	/* remove prev MENUITEM_END */
	  }
	  if (name > path && name[-1] == '/')
	    name[-1] = '\0';

	  *name++ = '\0';	/* delimit */
	  *str++ = '\0';	/* delimit */

	  while (isspace(*str))
	    str++;		/* skip space */
	}
	D_MENUBAR(("`%c' path = <%s>, name = <%s>, name2 = <%s>, action = <%s>\n", cmd,
		   (path ? path : "(nil)"), (name ? name : "(nil)"), (name2 ? name2 : "(nil)"),
		   (str ? str : "(nil)")));
      }
      /* process the different commands */
      switch (cmd) {
	case '+':		/* add/replace existing menu or menuitem */
	  if (path[0] != '\0') {
	    int len;

	    path = menu_find_base(&BuildMenu, path);
	    len = strlen(path);

	    /* don't allow menus called `*' */
	    if (path[0] == '*') {
	      menu_clear(BuildMenu);
	      break;
	    } else if (len >= 2 && !strcmp((path + len - 2), "/*")) {
	      path[len - 2] = '\0';
	    }
	    if (path[0] != '\0')
	      BuildMenu = menu_add(BuildMenu, path);
	  }
	  if (name != NULL && name[0] != '\0') {
	    if (!strcmp(name, SEPARATOR_NAME))
	      name = "";
	    menuitem_add(BuildMenu, name, name2, str);
	  }
	  break;

	case '-':		/* delete menu entry */
	  if (!strcmp(path, "/*") && (name == NULL || name[0] == '\0')) {
	    menubar_clear();
	    BuildMenu = NULL;
	    menubar_expose();
	    break;
	  } else if (path[0] != '\0') {
	    int len;
	    menu_t *menu = BuildMenu;

	    path = menu_find_base(&menu, path);
	    len = strlen(path);

	    /* submenu called `*' clears all menu items */
	    if (path[0] == '*') {
	      menu_clear(menu);
	      break;		/* done */
	    } else if (len >= 2 && !strcmp(&path[len - 2], "/*")) {
	      /* done */
	      break;
	    } else if (path[0] != '\0') {
	      BuildMenu = NULL;
	      break;
	    } else {
	      BuildMenu = menu;
	    }
	  }
	  if (BuildMenu != NULL) {
	    if (name == NULL || name[0] == '\0') {
	      BuildMenu = menu_delete(BuildMenu);
	    } else {
	      menuitem_t *item;

	      if (!strcmp(name, SEPARATOR_NAME))
		name = "";
	      item = menuitem_find(BuildMenu, name);

	      if (item != NULL && item->entry.type != MenuSubMenu) {
		menuitem_free(BuildMenu, item);

		/* fix up the width */
		BuildMenu->width = 0;
		for (item = BuildMenu->head;
		     item != NULL;
		     item = item->next) {
		  if (BuildMenu->width < (item->len + item->len2))
		    BuildMenu->width = (item->len + item->len2);
		}
	      }
	    }
	    menubar_expose();
	  }
	  break;
      }
      break;
  }
}

void
draw_Arrows(int name, int state)
{

  GC top = None, bot = None;

  int i;

#ifdef MENU_SHADOW_IN
  state = -state;
#endif
  switch (state) {
    case +1:
      top = topShadowGC;
      bot = botShadowGC;
      break;			/* SHADOW_OUT */
    case -1:
      top = botShadowGC;
      bot = topShadowGC;
      break;			/* SHADOW_IN */
    case 0:
      top = bot = neutralGC;
      break;			/* neutral */
  }
  if (!Arrows_x)
    return;

  for (i = 0; i < NARROWS; i++) {
    const int w = Width2Pixel(1);
    const int y = (menuBar_TotalHeight() - w) / 2;
    int x = Arrows_x + (5 * Width2Pixel(i)) / 4;

    if (!name || name == Arrows[i].name)
      Draw_Triangle(menuBar.win, top, bot, x, y, w,
		    Arrows[i].name);
  }
  XFlush(Xdisplay);
}

void
menubar_expose(void)
{

  menu_t *menu;
  int x;

# ifdef CHANGE_SCROLLCOLOR_ON_FOCUS
  static int focus = -1;

# endif
# ifdef KANJI
  static int fsTry = 0;

# endif

  if (delay_menu_drawing || !menubar_visible())
    return;

#ifdef KANJI
  if (!fontset && !fsTry) {
    char *fontname = malloc(strlen(rs_font[0]) + strlen(rs_kfont[0]) + 2);
    int i, mc;
    char **ml, *ds;

    fsTry = 1;
    if (fontname) {
      setlocale(LC_ALL, "");
      strcpy(fontname, rs_font[0]);
      strcat(fontname, ",");
      strcat(fontname, rs_kfont[0]);
      fontset = XCreateFontSet(Xdisplay, fontname, &ml, &mc, &ds);
      free(fontname);
      if (mc) {
	XFreeStringList(ml);
	fontset = 0;
	return;
      }
    }
  }
#endif /* KANJI */

  if (menubarGC == None) {
    /* Create the graphics context */
    XGCValues gcvalue;

    gcvalue.font = TermWin.font->fid;

    gcvalue.foreground = (Xdepth <= 2 ?
			  PixColors[fgColor] :
			  PixColors[menuTextColor]);
    menubarGC = XCreateGC(Xdisplay, menuBar.win,
			  GCForeground | GCFont,
			  &gcvalue);

#ifdef KEEP_SCROLLCOLOR
    gcvalue.foreground = PixColors[scrollColor];
#endif
    neutralGC = XCreateGC(Xdisplay, menuBar.win,
			  GCForeground,
			  &gcvalue);

#ifdef KEEP_SCROLLCOLOR
    gcvalue.foreground = PixColors[bottomShadowColor];
#endif
    botShadowGC = XCreateGC(Xdisplay, menuBar.win,
			    GCForeground | GCFont,
			    &gcvalue);

#ifdef KEEP_SCROLLCOLOR
    gcvalue.foreground = PixColors[topShadowColor];
#endif
    topShadowGC = XCreateGC(Xdisplay, menuBar.win,
			    GCForeground,
			    &gcvalue);
  }
# ifdef CHANGE_SCROLLCOLOR_ON_FOCUS
  /* Update colors on focus change */
  if (focus != TermWin.focus) {
    XGCValues gcvalue;

    focus = TermWin.focus;

    gcvalue.foreground = PixColors[fgColor];

# ifdef KEEP_SCROLLCOLOR
    if (Xdepth > 2)
      gcvalue.foreground = PixColors[focus ? scrollColor : unfocusedScrollColor];
# endif

    XChangeGC(Xdisplay, neutralGC, GCForeground,
	      &gcvalue);

    gcvalue.background = gcvalue.foreground;
    XChangeGC(Xdisplay, menubarGC, GCBackground,
	      &gcvalue);
    XChangeGC(Xdisplay, neutralGC, GCForeground,
	      &gcvalue);

    XSetWindowBackground(Xdisplay, menuBar.win, gcvalue.foreground);

    gcvalue.foreground = PixColors[bgColor];

# ifdef KEEP_SCROLLCOLOR
    gcvalue.foreground = PixColors[focus ? topShadowColor : unfocusedTopShadowColor];
# endif
    XChangeGC(Xdisplay, topShadowGC,
	      GCForeground,
	      &gcvalue);

# ifdef KEEP_SCROLLCOLOR
    gcvalue.foreground = PixColors[focus ? bottomShadowColor : unfocusedBottomShadowColor];
# endif
    XChangeGC(Xdisplay, botShadowGC,
	      GCForeground,
	      &gcvalue);

  }
#endif

  /* make sure the font is correct */
  XSetFont(Xdisplay, menubarGC, TermWin.font->fid);
  XSetFont(Xdisplay, botShadowGC, TermWin.font->fid);
  XClearWindow(Xdisplay, menuBar.win);

  menu_hide_all();

  x = 0;
  if (CurrentBar != NULL) {
    for (menu = CurrentBar->head; menu != NULL; menu = menu->next) {
      int len = menu->len;

      x = (menu->x + menu->len + HSPACE);

#if DEBUG >= DEBUG_MENU_LAYOUT
      if (debug_level >= DEBUG_MENU_LAYOUT) {
	print_menu_descendants(menu);
      }
#endif

      if (x >= TermWin.ncol)
	len = (TermWin.ncol - (menu->x + HSPACE));

      drawbox_menubar(menu->x, len, +1);

#ifdef KANJI
      if (fontset)
	XmbDrawString(Xdisplay, menuBar.win, fontset, menubarGC, (Width2Pixel(menu->x) + Width2Pixel(HSPACE) / 2),
		      menuBar_height() - (TermWin.font->descent) + 1, menu->name, len);
      else
#endif
	XDrawString(Xdisplay, menuBar.win, menubarGC, (Width2Pixel(menu->x) + Width2Pixel(HSPACE) / 2),
		    menuBar_height() - (TermWin.font->descent) + 1, menu->name, len);
      if (x >= TermWin.ncol)
	break;
    }
  }
  drawbox_menubar(x, TermWin.ncol, 1);

  /* add the menuBar title, if it exists and there's plenty of room */
  Arrows_x = 0;
  if (x < TermWin.ncol) {
    char *str, title[256];
    int len, ncol = TermWin.ncol;

    if (x < (ncol - (NARROWS + 1))) {
      ncol -= (NARROWS + 1);
      Arrows_x = menuBar_TotalWidth() - (2 * SHADOW + ((5 * Width2Pixel(1)) / 4 * NARROWS) + HSPACE);
    }
    draw_Arrows(0, -1);

    str = (CurrentBar && CurrentBar->title ? CurrentBar->title : "%n");
    for (len = 0; str[0] && len < sizeof(title) - 1; str++) {
      const char *s = NULL;

      switch (str[0]) {
	case '%':
	  str++;
	  switch (str[0]) {
	    case 'n':
	      s = rs_name;
	      break;		/* resource name */
	    case 'v':
	      s = VERSION;
	      break;		/* version number */
	    case '%':
	      s = "%";
	      break;		/* literal '%' */
	  }
	  if (s != NULL)
	    while (*s && len < sizeof(title) - 1)
	      title[len++] = *s++;
	  break;

	default:
	  title[len++] = str[0];
	  break;
      }
    }
    title[len] = '\0';

    ncol = Pixel2Width(Arrows_x - Width2Pixel(x) - Width2Pixel(len) - Width2Pixel(4));
#ifdef KANJI
    if (fontset) {
      if (len > 0 && ncol >= 0)
	XmbDrawString(Xdisplay, menuBar.win, fontset, menubarGC,
		      Width2Pixel(x) + ((Arrows_x - Width2Pixel(x)) / 2 - (Width2Pixel(len) / 2)),
		      menuBar_height() - (TermWin.font->descent) + 1, title, len);
    } else
#endif
    if (len > 0 && ncol >= 0)
      XDrawString(Xdisplay, menuBar.win, menubarGC,
		  Width2Pixel(x) + ((Arrows_x - Width2Pixel(x + len + 1)) / 2),
		  menuBar_height() - (TermWin.font->descent) + 1, title, len);
  }
}

int
menubar_mapping(int map)
{

  int change = 0;

  if (map && !menubar_visible()) {
    menuBar.state = 1;
    XMapWindow(Xdisplay, menuBar.win);
    change = 1;
  } else if (!map && menubar_visible()) {
    menubar_expose();
    menuBar.state = 0;
    XUnmapWindow(Xdisplay, menuBar.win);
    change = 1;
  } else
    menubar_expose();

  return change;
}

int
menu_select(XButtonEvent * ev)
{

  menuitem_t *thisitem, *item = NULL;
  int this_y = 0, y = 0;

  Window unused_root, unused_child;
  int unused_root_x, unused_root_y;
  unsigned int unused_mask;

  if (ActiveMenu == NULL)
    return 0;

  D_MENUBAR(("menu_select()\n"));
  XQueryPointer(Xdisplay, ActiveMenu->win,
		&unused_root, &unused_child,
		&unused_root_x, &unused_root_y,
		&(ev->x), &(ev->y),
		&unused_mask);

  if (ActiveMenu->parent != NULL && (ev->x < 0 || ev->y < 0 || (ev->y > menu_height() && ev->x < 0))) {
    menu_hide();
    return 1;
  }

  /* determine the menu item corresponding to the Y index */
  if (ev->x >= 0 && ev->x <= (ActiveMenu->w - SHADOW)) {
    for (y = 0, item = ActiveMenu->head; item != NULL; item = item->next) {
      int h = menu_height();

      if (isSeparator(item->name)) {
	h = SEPARATOR_HEIGHT;
      } else if (ev->y >= y && ev->y < (y + h)) {
	break;
      }
      y += h;
    }
  }
  if (item == NULL && ev->type == ButtonRelease) {
    menu_hide_all();
    return 0;
  }
  thisitem = item;
  this_y = y;

/* erase the last item */
  if (ActiveMenu->item != NULL) {
    if (ActiveMenu->item != thisitem) {
      for (y = 0, item = ActiveMenu->head; item != NULL; item = item->next) {

	int h = menu_height();

	if (isSeparator(item->name)) {
	  h = SEPARATOR_HEIGHT;
	} else if (item == ActiveMenu->item) {
	  /* erase old menuitem */
	  drawbox_menuitem(y, 0);	/* No Shadow */
	  if (item->entry.type == MenuSubMenu)
	    drawtriangle(ActiveMenu->w, y, +1);
	  break;
	}
	y += h;
      }
    } else {
      if (ev->type == ButtonRelease) {
	switch (item->entry.type) {
	case MenuLabel:
	case MenuSubMenu:
	  menu_hide_all();
	  break;

	case MenuAction:
	case MenuTerminalAction:
	  drawbox_menuitem(this_y, -1);

#if MENU_DELAY_USEC > 0
	  /* use select for timing */
	  {
	    struct itimerval tv;

	    tv.it_value.tv_sec = 0;
	    tv.it_value.tv_usec = MENU_DELAY_USEC;

	    select(0, NULL, NULL, NULL, &tv.it_value);
	  }
#endif
	  menu_hide_all();  /* remove menu before sending keys to the application */
	  D_MENUBAR(("%s: %s\n", item->name, item->entry.action.str));
	  action_dispatch(&(item->entry.action));
	  break;
	}
	return 0;
      } else if (item->entry.type != MenuSubMenu) {
	return 0;
      }
    }
  }
  ActiveMenu->item = thisitem;
  y = this_y;
  if (thisitem != NULL) {
    item = ActiveMenu->item;
    if (item->entry.type != MenuLabel)
      drawbox_menuitem(y, +1);
    if (item->entry.type == MenuSubMenu) {
      drawtriangle(ActiveMenu->w, y, -1);
      if ((ev->x > ActiveMenu->w / 2) && (ev->y > 0) && (Menu_PixelWidth(item->entry.submenu.menu) + ev->x >= ActiveMenu->w)) {
	ActiveMenu = item->entry.submenu.menu;
	menu_show();
	return 1;
      }
    }
  }
  return 0;
}

void
menubar_select(XButtonEvent * ev)
{

  menu_t *menu = NULL;
  static int last_mouse_x = 0, last_mouse_y = 0, last_win_x = 0, last_win_y = 0;
  int mouse_x, mouse_y, win_x, win_y, dx, dy, unused;
  Window unused_window;

/* determine the pulldown menu corresponding to the X index */
  D_MENUBAR(("menubar_select():\n"));
  if (ev->y >= 0 && ev->y <= menuBar_height() && CurrentBar != NULL) {
    for (menu = CurrentBar->head; menu != NULL; menu = menu->next) {
      int x = Width2Pixel(menu->x);
      int w = Width2Pixel(menu->len + HSPACE);

      if ((ev->x >= x && ev->x < x + w))
	break;
    }
  }
  switch (ev->type) {
    case ButtonRelease:
      D_MENUBAR(("  menubar_select(ButtonRelease)\n"));
      menu_hide_all();
      break;

    case ButtonPress:
      D_MENUBAR(("  menubar_select(ButtonPress)\n"));
      if (menu == NULL && Arrows_x && ev->x >= Arrows_x) {
	int i;

	for (i = 0; i < NARROWS; i++) {
	  if ((ev->x >= (Arrows_x + (Width2Pixel(4 * i + i)) / 4)) && (ev->x < (Arrows_x + (Width2Pixel(4 * i + i + 4)) / 4))) {

	    draw_Arrows(Arrows[i].name, +1);

#if MENU_DELAY_USEC > 0
	    /*
	     * use select for timing
	     */
	    {
	      struct itimerval tv;

	      tv.it_value.tv_sec = 0;
	      tv.it_value.tv_usec = MENU_DELAY_USEC;

	      select(0, NULL, NULL, NULL, &tv.it_value);
	    }
#endif

	    draw_Arrows(Arrows[i].name, -1);
#if DEBUG >= DEBUG_MENUARROWS
	    if (debug_level >= DEBUG_MENUARROWS) {
	      fprintf(stderr, "'%c': ", Arrows[i].name);

	      if (CurrentBar == NULL ||
		  (CurrentBar->arrows[i].type != MenuAction &&
		   CurrentBar->arrows[i].type != MenuTerminalAction)) {
		if (Arrows[i].str != NULL && Arrows[i].str[0])
		  fprintf(stderr, "(default) \\033%s\n",
			  &(Arrows[i].str[2]));
	      } else {
		fprintf(stderr, "%s\n", CurrentBar->arrows[i].str);
	      }
	    } else {
#endif
	      if (CurrentBar == NULL ||
		  action_dispatch(&(CurrentBar->arrows[i]))) {
		if (Arrows[i].str != NULL &&
		    Arrows[i].str[0] != 0)
		  tt_write((Arrows[i].str + 1),
			   Arrows[i].str[0]);
	      }
#if DEBUG >= DEBUG_MENUARROWS
	    }
#endif /* DEBUG_MENUARROWS */
	    return;
	  }
	}
      } else if (menu == NULL && ActiveMenu == NULL && Options & Opt_menubar_move) {
	XTranslateCoordinates(Xdisplay, TermWin.parent, Xroot,
			      0, 0, &last_win_x, &last_win_y, &unused_window);
	XQueryPointer(Xdisplay, TermWin.parent, &unused_window, &unused_window, &unused, &unused,
		      &last_mouse_x, &last_mouse_y, &unused);
	D_MENUBAR(("Initial data:  last_mouse == %d,%d  last_win == %d,%d\n",
		   last_mouse_x, last_mouse_y, last_win_x, last_win_y));
	break;
      }
      /*drop */
    case MotionNotify:
      if (menu == NULL && ActiveMenu == NULL && Options & Opt_menubar_move) {
	XQueryPointer(Xdisplay, TermWin.parent, &unused_window, &unused_window, &unused, &unused,
		      &mouse_x, &mouse_y, &unused);
	if (mouse_x != last_mouse_x || mouse_y != last_mouse_y) {
	  dx = mouse_x - last_mouse_x;
	  dy = mouse_y - last_mouse_y;
	  D_MENUBAR((" -> last_mouse == %d,%d  mouse == %d,%d  rel == %d,%d  move %d,%d to %d,%d\n",
		     last_mouse_x, last_mouse_y, mouse_x, mouse_y, dx, dy, last_win_x, last_win_y, last_win_x + dx, last_win_y + dy));
	  XMoveWindow(Xdisplay, TermWin.parent, last_win_x + dx, last_win_y + dy);
	  last_win_x += dx;
	  last_win_y += dy;
	}
	break;
      }
      /* drop */
    default:
      /*
       * press menubar or move to a new entry
       */
      D_MENUBAR(("  menubar_select(default)\n"));
      if (menu != NULL && menu != ActiveMenu) {
	menu_hide_all();	/* pop down old menu */
	ActiveMenu = menu;
	menu_show();		/* pop up new menu */
      }
      break;
  }
}

/*
 * general dispatch routine,
 * it would be nice to have `sticky' menus
 */
void
menubar_control(XButtonEvent * ev)
{

  switch (ev->type) {
    case ButtonPress:
      D_MENUBAR(("menubar_control(ButtonPress)\n"));
      if (ev->button == Button1)
	menubar_select(ev);
      break;

    case ButtonRelease:
      D_MENUBAR(("menubar_control(ButtonRelease)\n"));
      if (ev->button == Button1)
	menu_select(ev);
      break;

    case MotionNotify:
      D_MENUBAR(("menubar_control(MotionNotify)\n"));
      while (XCheckTypedWindowEvent(Xdisplay, TermWin.parent, MotionNotify, (XEvent *) ev));

      if (ActiveMenu)
	while (menu_select(ev));
      else
	ev->y = -1;
      if (ev->y < 0) {

	Window unused_root, unused_child;
	int unused_root_x, unused_root_y;
	unsigned int unused_mask;

	XQueryPointer(Xdisplay, menuBar.win, &unused_root, &unused_child, &unused_root_x, &unused_root_y,
		      &(ev->x), &(ev->y), &unused_mask);
	menubar_select(ev);
      }
      break;
  }
}
#endif /* MENUBAR_MAX */
