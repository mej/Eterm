/*
 * Copyright (C) 1999-1997, Michael Jennings
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies of the Software, its documentation and marketing & publicity
 * materials, and acknowledgment shall be given in the documentation, materials
 * and software packages that this Software was used.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

static const char cvs_ident[] = "$Id$";

#include "config.h"
#include "feature.h"

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <unistd.h>
#include <Imlib.h>

#include "../libmej/debug.h"
#include "../libmej/mem.h"
#include "../libmej/strings.h"
#include "e.h"
#include "icon.h"
#include "startup.h"
#include "menus.h"
#include "options.h"
#include "pixmap.h"
#include "screen.h"
#include "scrollbar.h"
#include "term.h"

#ifdef PIXMAP_SUPPORT
Pixmap desktop_pixmap = None;
Pixmap viewport_pixmap = None;
Window desktop_window = None;
unsigned char desktop_pixmap_is_mine = 0;
ImlibData *imlib_id = NULL;
image_t images[image_max] =
{
  {None, 0, NULL, NULL, NULL, NULL},
  {None, 0, NULL, NULL, NULL, NULL},
  {None, 0, NULL, NULL, NULL, NULL},
  {None, 0, NULL, NULL, NULL, NULL},
  {None, 0, NULL, NULL, NULL, NULL},
  {None, 0, NULL, NULL, NULL, NULL},
  {None, 0, NULL, NULL, NULL, NULL},
  {None, 0, NULL, NULL, NULL, NULL},
  {None, 0, NULL, NULL, NULL, NULL}
};

#endif

#ifdef PIXMAP_SUPPORT
static const char *get_iclass_name(unsigned char);

const char *
get_image_type(unsigned short type)
{

  switch (type) {
    case image_bg:
      return "image_bg";
      break;
    case image_up:
      return "image_up";
      break;
    case image_down:
      return "image_down";
      break;
    case image_left:
      return "image_left";
      break;
    case image_right:
      return "image_right";
      break;
    case image_sb:
      return "image_sb";
      break;
    case image_sa:
      return "image_sa";
      break;
    case image_menu:
      return "image_menu";
      break;
    case image_submenu:
      return "image_submenu";
      break;
    case image_max:
      return "image_max";
      break;
    default:
      return "";
      break;
  }
  return ("");
}

unsigned char
image_mode_any(unsigned char mode)
{
  unsigned char ismode = 0;

  FOREACH_IMAGE(if (image_mode_is(idx, mode)) ismode=1;);
  return ismode;
}

unsigned short
parse_pixmap_ops(char *str)
{

  unsigned short op = OP_NONE;
  char *token;

  REQUIRE_RVAL(str && *str, OP_NONE);
  D_PIXMAP(("parse_pixmap_ops(str [%s]) called.\n", str));

  for (; (token = (char *) strsep(&str, ":"));) {
    if (!BEG_STRCASECMP("tiled", token)) {
      op |= OP_TILE;
    } else if (!BEG_STRCASECMP("hscaled", token)) {
      op |= OP_HSCALE;
    } else if (!BEG_STRCASECMP("vscaled", token)) {
      op |= OP_VSCALE;
    } else if (!BEG_STRCASECMP("scaled", token)) {
      op |= OP_SCALE;
    } else if (!BEG_STRCASECMP("propscaled", token)) {
      op |= OP_PROPSCALE;
    }
  }
  return (op);
}

unsigned short
set_pixmap_scale(const char *geom, pixmap_t *pmap)
{

  static char str[GEOM_LEN + 1] =
  {'\0'};
  unsigned int w = 0, h = 0;
  int x = 0, y = 0;
  unsigned short op = OP_NONE;
  int flags;
  unsigned short changed = 0;
  char *p, *opstr;
  int n;

  if (geom == NULL)
    return 0;

  D_PIXMAP(("scale_pixmap(\"%s\")\n", geom));
  if (!strcmp(geom, "?")) {
    sprintf(str, "[%dx%d+%d+%d]", pmap->w, pmap->h, pmap->x, pmap->y);
    xterm_seq(XTerm_title, str);
    return 0;
  }
  if ((opstr = strchr(geom, ':')) != NULL) {
    *opstr++ = '\0';
    op |= parse_pixmap_ops(opstr);
  }
  if ((p = strchr(geom, ';')) == NULL)
    p = strchr(geom, '\0');
  n = (p - geom);
  if (n > GEOM_LEN - 1)
    return 0;

  strncpy(str, geom, n);
  str[n] = '\0';

  flags = XParseGeometry(str, &x, &y, &w, &h);

  if (!flags) {
    flags |= WidthValue;	/* default is tile */
    w = 0;
  }
  if (flags & WidthValue) {
    if (!(flags & XValue)) {
      x = 50;
    }
    if (!(flags & HeightValue))
      h = w;

    if (w && !h) {
      w = pmap->w * ((float) w / 100);
      h = pmap->h;
    } else if (h && !w) {
      w = pmap->w;
      h = pmap->h * ((float) h / 100);
    }
    /* If they want scaling, but didn't give a percentage, assume 100% */
    if (op & OP_PROPSCALE) {
      if (!w)
	w = 100;
      if (!h)
	h = 100;
    } else {
      if ((op & OP_HSCALE) && !w) {
	w = 100;
      }
      if ((op & OP_VSCALE) && !h) {
	h = 100;
      }
    }

    if (pmap->w != (int) w) {
      pmap->w = (int) w;
      changed++;
    }
    if (pmap->h != (int) h) {
      pmap->h = (int) h;
      changed++;
    }
  }
  if (!(flags & YValue)) {
    if (flags & XNegative)
      flags |= YNegative;
    y = x;
  }
  if (!(flags & WidthValue) && geom[0] != '=') {
    x += pmap->x;
    y += pmap->y;
  } else {
    if (flags & XNegative)
      x += 100;
    if (flags & YNegative)
      y += 100;
  }

  x = (x <= 0 ? 0 : (x >= 100 ? 100 : x));
  y = (y <= 0 ? 0 : (y >= 100 ? 100 : y));;
  if (pmap->x != x) {
    pmap->x = x;
    changed++;
  }
  if (pmap->y != y) {
    pmap->y = y;
    changed++;
  }
  if (pmap->op != op) {
    pmap->op = op;
    changed++;
  }
  D_PIXMAP(("scale_pixmap() returning %hu, *pmap == { op [%hu], w [%hd], h [%hd], x [%hd], y [%hd] }\n",
	    changed, pmap->op, pmap->w, pmap->h, pmap->x, pmap->y));
  return changed;
}

void
reset_simage(simage_t * simg, unsigned long mask)
{

  ASSERT(simg != NULL);

  D_PIXMAP(("reset_simage(%8p, 0x%08x)\n", simg, mask));

  if ((mask & RESET_IMLIB_IM) && simg->iml->im) {
    Imlib_destroy_image(imlib_id, simg->iml->im);
    simg->iml->im = NULL;
  }
  if ((mask & RESET_PMAP_PIXMAP) && simg->pmap->pixmap != None) {
    Imlib_free_pixmap(imlib_id, simg->pmap->pixmap);
    simg->pmap->pixmap = None;
  }
  if ((mask & RESET_PMAP_MASK) && simg->pmap->mask != None) {
    Imlib_free_pixmap(imlib_id, simg->pmap->mask);
    simg->pmap->mask = None;
  }
  if ((mask & RESET_IMLIB_BORDER) && simg->iml->border) {
    FREE(simg->iml->border);
    simg->iml->border = NULL;
  }
  if ((mask & RESET_IMLIB_MOD) && simg->iml->mod) {
    FREE(simg->iml->mod);
    simg->iml->mod = NULL;
  }
  if ((mask & RESET_IMLIB_RMOD) && simg->iml->rmod) {
    FREE(simg->iml->rmod);
    simg->iml->rmod = NULL;
  }
  if ((mask & RESET_IMLIB_GMOD) && simg->iml->gmod) {
    FREE(simg->iml->gmod);
    simg->iml->gmod = NULL;
  }
  if ((mask & RESET_IMLIB_BMOD) && simg->iml->bmod) {
    FREE(simg->iml->bmod);
    simg->iml->bmod = NULL;
  }
  if (mask & RESET_PMAP_GEOM) {
    simg->pmap->w = 0;
    simg->pmap->h = 0;
    simg->pmap->x = 50;
    simg->pmap->y = 50;
    simg->pmap->op = OP_NONE;
  }
}

static const char *
get_iclass_name(unsigned char which)
{
  switch (which) {
  case image_bg: return "ETERM_BG"; break;
  case image_up: return "ETERM_ARROW_UP"; break;
  case image_down: return "ETERM_ARROW_DOWN"; break;
  case image_left: return "ETERM_ARROW_LEFT"; break;
  case image_right: return "ETERM_ARROW_RIGHT"; break;
  case image_sb: return "ETERM_TROUGH"; break;
  case image_sa: return "ETERM_ANCHOR"; break;
  case image_menu: return "ETERM_MENU_ITEM"; break;
  case image_submenu: return "ETERM_MENU_SUBMENU"; break;
  default:
    ASSERT_NOTREACHED_RVAL(NULL);
    break;
  }
}

unsigned char
check_image_ipc(unsigned char reset)
{
  static unsigned char checked = 0;
  register unsigned short i;
  char buff[255], *reply;
  const char *iclass;

  if (reset) {
    checked = 0;
  }
  if (checked) {
    return ((checked == 1) ? 1 : 0);
  }
  for (i=0; i < image_max; i++) {
    if (!image_mode_is(i, MODE_AUTO)) {
      continue;
    }
    iclass = get_iclass_name(i);
    snprintf(buff, sizeof(buff), "imageclass %s query", iclass);
    reply = enl_send_and_wait(buff);
    if (strstr(reply, "not")) {
      print_error("ImageClass \"%s\" is not defined in Enlightenment.  Disallowing \"auto\" mode for this image.\n", iclass);
      image_mode_fallback(i);
    } else if (strstr(reply, "Error")) {
      print_error("Looks like this version of Enlightenment doesn't support the IPC commands I need.  Disallowing \"auto\" mode for all images.\n");
      FOREACH_IMAGE(if (image_mode_is(idx, MODE_AUTO)) {if (image_mode_is(idx, ALLOW_IMAGE)) {image_set_mode(idx, MODE_IMAGE);} else {image_set_mode(idx, MODE_SOLID);}} \
                    if (image_mode_is(idx, ALLOW_AUTO)) {image_disallow_mode(idx, ALLOW_AUTO);});
      FREE(reply);
      checked = 2;
      return 0;
    }
    FREE(reply);
  }
  checked = 1;
  return 1;
}

void
paste_simage(simage_t *simg, unsigned char which, Window win, unsigned short x, unsigned short y, unsigned short w, unsigned short h)
{

  ASSERT(simg != NULL);
  REQUIRE(win != None);

  D_PIXMAP(("paste_simage(%8p, %s, 0x%08x, %hd, %hd, %hd, %hd) called.\n", simg, get_image_type(which), (int) win, x, y, w, h));

  if ((images[which].mode & MODE_AUTO) && (images[which].mode & ALLOW_AUTO)) {
    char buff[255], *reply;
    const char *iclass, *state;
    Pixmap pmap, mask;

    check_image_ipc(0);
    if (image_mode_is(which, MODE_AUTO)) {
      iclass = get_iclass_name(which);
      if (simg == images[which].selected) {
        state = "hilited";
      } else if (simg == images[which].clicked) {
        state = "clicked";
      } else {
        state = "normal";
      }
      D_PIXMAP((" -> iclass == \"%s\", state == \"%s\"\n", NONULL(iclass), NONULL(state)));

      if (iclass) {
        snprintf(buff, sizeof(buff), "imageclass %s apply_copy 0x%x %s %hd %hd", iclass, (int) win, state, w, h);
        reply = enl_send_and_wait(buff);
        if (strstr(reply, "Error")) {
          print_error("Enlightenment didn't seem to like something about my syntax.  Disallowing \"auto\" mode for this image.\n");
          image_mode_fallback(which);
          FREE(reply);
        } else {
          GC gc;
          XGCValues gcvalues;

          gc = XCreateGC(Xdisplay, win, 0, &gcvalues);
          pmap = (Pixmap) strtoul(reply, (char **) NULL, 0);
          mask = (Pixmap) strtoul(PWord(2, reply), (char **) NULL, 0);
          FREE(reply);
          enl_ipc_sync();
          if (pmap) {
            if (mask) {
              shaped_window_apply_mask(pmap, mask);
            }
            XSetClipMask(Xdisplay, gc, mask);
            XSetClipOrigin(Xdisplay, gc, x, y);
            XCopyArea(Xdisplay, pmap, win, gc, 0, 0, w, h, x, y);
            snprintf(buff, sizeof(buff), "imageclass %s free_pixmap 0x%08x", iclass, (int) pmap);
            enl_ipc_send(buff);
            XFreeGC(Xdisplay, gc);
            return;
          } else {
            print_error("Enlightenment returned a null pixmap, which I can't use.  Disallowing \"auto\" mode for this image.\n");
            FREE(reply);
            image_mode_fallback(which);
          }
        }
      }
    }
  }
  if (image_mode_is(which, MODE_IMAGE) && image_mode_is(which, ALLOW_IMAGE)) {
    if (simg->iml->border) {
      Imlib_set_image_border(imlib_id, simg->iml->im, simg->iml->border);
    }
    if (simg->iml->mod) {
      Imlib_set_image_modifier(imlib_id, simg->iml->im, simg->iml->mod);
    }
    if (simg->iml->rmod) {
      Imlib_set_image_red_modifier(imlib_id, simg->iml->im, simg->iml->rmod);
    }
    if (simg->iml->gmod) {
      Imlib_set_image_green_modifier(imlib_id, simg->iml->im, simg->iml->gmod);
    }
    if (simg->iml->bmod) {
      Imlib_set_image_blue_modifier(imlib_id, simg->iml->im, simg->iml->bmod);
    }
    Imlib_paste_image(imlib_id, simg->iml->im, win, x, y, w, h);
  }
}

void
redraw_image(unsigned char which) {

  switch (which) {
  case image_bg:
    render_simage(images[image_bg].current, TermWin.vt, TermWin_TotalWidth(), TermWin_TotalHeight(), image_bg, 0);
    scr_touch();
    break;
  case image_up:
    scrollbar_draw_uparrow(IMAGE_STATE_CURRENT, MODE_MASK);
    break;
  case image_down:
    scrollbar_draw_downarrow(IMAGE_STATE_CURRENT, MODE_MASK);
    break;
  case image_sb:
    scrollbar_draw_trough(IMAGE_STATE_CURRENT, MODE_MASK);
    break;
  case image_sa:
    scrollbar_draw_anchor(IMAGE_STATE_CURRENT, MODE_MASK);
    break;
  default:
    D_PIXMAP(("redraw_image():  Bad value %u\n", which));
    break;
  }
}

void
render_simage(simage_t * simg, Window win, unsigned short width, unsigned short height, unsigned char which, renderop_t renderop)
{

  XGCValues gcvalue;
  GC gc;
  short xsize, ysize;
  short xpos = 0, ypos = 0;
  Pixmap pixmap = None;
  unsigned short rendered = 0;
  unsigned short xscaled = 0, yscaled = 0;
# ifdef PIXMAP_OFFSET
  static unsigned int last_x = 0, last_y = 0;
  int x, y;
  int px, py;
  unsigned int pw, ph, pb, pd;
  Window w, dummy;
  Screen *scr;
# endif				/* PIXMAP_OFFSET */

  scr = ScreenOfDisplay(Xdisplay, Xscreen);
  if (!scr)
    return;

  ASSERT(simg != NULL);
  ASSERT(simg->iml != NULL);
  ASSERT(simg->pmap != NULL);

  D_PIXMAP(("render_simage():  Rendering simg->iml->im %8p (%s) at %hux%hu onto window 0x%08x\n", simg->iml->im, get_image_type(which), width, height, win));
  D_PIXMAP(("render_simage():  Image mode is 0x%02x\n", images[which].mode));

  if ((which == image_bg) && image_mode_is(image_bg, MODE_VIEWPORT)) {
    width = scr->width;
    height = scr->height;
  }
  if (!(width) || !(height))
    return;

  gcvalue.foreground = gcvalue.background = PixColors[bgColor];
  gc = XCreateGC(Xdisplay, win, GCForeground | GCBackground, &gcvalue);
  pixmap = simg->pmap->pixmap;	/* Save this for later */

  if ((images[which].mode & MODE_AUTO) && (images[which].mode & ALLOW_AUTO)) {
    char buff[255];
    const char *iclass, *state;

    check_image_ipc(0);
    if (image_mode_is(which, MODE_AUTO)) {
      iclass = get_iclass_name(which);
      if (images[which].current == images[which].selected) {
        state = "hilited";
      } else if (images[which].current == images[which].clicked) {
        state = "clicked";
      } else {
        state = "normal";
      }
      if (iclass) {
        snprintf(buff, sizeof(buff), "imageclass %s apply 0x%x %s", iclass, (int) win, state);
        enl_ipc_send(buff);
        return;
      }
    }
  }

# ifdef PIXMAP_OFFSET
  if (image_mode_is(which, MODE_TRANS) && image_mode_is(which, ALLOW_TRANS)) {
    if (desktop_window == None) {
      get_desktop_window();
    }
    if (desktop_window == None) {
      print_error("Unable to locate desktop window.  If you are running Enlightenment, please\n"
		  "restart.  If not, please set your background image with Esetroot, then try again.");
      FOREACH_IMAGE(if (image_mode_is(idx, MODE_TRANS)) {image_set_mode(idx, MODE_IMAGE); image_allow_mode(idx, ALLOW_IMAGE);});
      render_simage(simg, win, width, height, which, renderop);
      return;
    }
    if (desktop_pixmap == None) {
      get_desktop_pixmap();
      last_x = last_y = -1;
    }
    if (desktop_pixmap != None) {
      XTranslateCoordinates(Xdisplay, win, desktop_window, 0, 0, &x, &y, &w);
      if (simg->pmap->pixmap != None) {
	XFreePixmap(Xdisplay, simg->pmap->pixmap);
      }
      simg->pmap->pixmap = XCreatePixmap(Xdisplay, win, width, height, Xdepth);
      D_PIXMAP(("desktop_pixmap == 0x%08x, simg->pmap->pixmap == 0x%08x\n", desktop_pixmap, simg->pmap->pixmap));
      if (simg->pmap->pixmap != None) {
	XGetGeometry(Xdisplay, desktop_pixmap, &w, &px, &py, &pw, &ph, &pb, &pd);
        if ((pw <= 0) || (ph <= 0)) {
          print_error("Value of desktop pixmap property is invalid.  Please restart your "
                      "window manager or use Esetroot to set a new one.");
          desktop_pixmap = None;
          D_PIXMAP(("Setting background of window 0x%08x to the background color\n", win));
          XSetWindowBackground(Xdisplay, win, PixColors[bgColor]);
	} else {
          if (pw < (unsigned int) scr->width || ph < (unsigned int) scr->height) {
            XFreeGC(Xdisplay, gc);
            gc = XCreateGC(Xdisplay, desktop_pixmap, 0, &gcvalue);
            XSetTile(Xdisplay, gc, desktop_pixmap);
            XSetTSOrigin(Xdisplay, gc, pw - (x % pw), ph - (y % ph));
            XSetFillStyle(Xdisplay, gc, FillTiled);
            XFillRectangle(Xdisplay, simg->pmap->pixmap, gc, 0, 0, scr->width, scr->height);
          } else {
            XCopyArea(Xdisplay, desktop_pixmap, simg->pmap->pixmap, gc, x, y, width, height, 0, 0);
          }
          if (which != image_bg && need_colormod(simg->iml)) {
            colormod_trans(simg->pmap->pixmap, simg->iml, gc, width, height);
          }
          if (simg->iml->bevel != NULL) {
            Imlib_bevel_pixmap(imlib_id, simg->pmap->pixmap, width, height, simg->iml->bevel->edges, simg->iml->bevel->up);
          }
          D_PIXMAP(("Setting background of window 0x%08x to 0x%08x\n", win, simg->pmap->pixmap));
          XSetWindowBackgroundPixmap(Xdisplay, win, simg->pmap->pixmap);
        }
      }
    } else {
      D_PIXMAP(("Setting background of window 0x%08x to the background color\n", win));
      XSetWindowBackground(Xdisplay, win, PixColors[bgColor]);
    }
  } else if (image_mode_is(which, MODE_VIEWPORT) && image_mode_is(which, ALLOW_VIEWPORT)) {
    D_PIXMAP(("Viewport mode enabled.  viewport_pixmap == 0x%08x and simg->pmap->pixmap == 0x%08x\n", viewport_pixmap, simg->pmap->pixmap));
    if (viewport_pixmap == None) {
      imlib_t *tmp_iml = images[image_bg].current->iml;

      xsize = tmp_iml->im->rgb_width;
      ysize = tmp_iml->im->rgb_height;
      if (tmp_iml->border) {
	Imlib_set_image_border(imlib_id, tmp_iml->im, tmp_iml->border);
      }
      if (tmp_iml->mod) {
	Imlib_set_image_modifier(imlib_id, tmp_iml->im, tmp_iml->mod);
      }
      if (tmp_iml->rmod) {
	Imlib_set_image_red_modifier(imlib_id, tmp_iml->im, tmp_iml->rmod);
      }
      if (tmp_iml->gmod) {
	Imlib_set_image_green_modifier(imlib_id, tmp_iml->im, tmp_iml->gmod);
      }
      if (tmp_iml->bmod) {
	Imlib_set_image_blue_modifier(imlib_id, tmp_iml->im, tmp_iml->bmod);
      }
      if ((images[image_bg].current->pmap->w > 0) || (images[image_bg].current->pmap->op & OP_SCALE)) {
	D_PIXMAP(("Scaling image to %dx%d\n", scr->width, scr->height));
	Imlib_render(imlib_id, tmp_iml->im, scr->width, scr->height);
      } else {
	D_PIXMAP(("Tiling image at %dx%d\n", xsize, ysize));
	Imlib_render(imlib_id, tmp_iml->im, xsize, ysize);
      }
      viewport_pixmap = Imlib_copy_image(imlib_id, tmp_iml->im);
      D_PIXMAP(("Created viewport_pixmap == 0x%08x\n", viewport_pixmap));
    } else {
      XGetGeometry(Xdisplay, viewport_pixmap, &dummy, &px, &py, &pw, &ph, &pb, &pd);
      xsize = (short) pw;
      ysize = (short) ph;
    }
    if (simg->pmap->pixmap != None) {
      XGetGeometry(Xdisplay, simg->pmap->pixmap, &dummy, &px, &py, &pw, &ph, &pb, &pd);
      if (pw != width || ph != height) {
	Imlib_free_pixmap(imlib_id, simg->pmap->pixmap);
	simg->pmap->pixmap = None;
      }
    }
    if (simg->pmap->pixmap == None) {
      simg->pmap->pixmap = XCreatePixmap(Xdisplay, win, width, height, Xdepth);
      D_PIXMAP(("Created simg->pmap->pixmap == 0x%08x\n", simg->pmap->pixmap));
    }
    XTranslateCoordinates(Xdisplay, win, Xroot, 0, 0, &x, &y, &dummy);
    D_PIXMAP(("Translated coords are %d, %d\n", x, y));
    if ((images[image_bg].current->pmap->w > 0) || (images[image_bg].current->pmap->op & OP_SCALE)) {
      XCopyArea(Xdisplay, viewport_pixmap, simg->pmap->pixmap, gc, x, y, width, height, 0, 0);
    } else {
      XFreeGC(Xdisplay, gc);
      gc = XCreateGC(Xdisplay, viewport_pixmap, 0, &gcvalue);
      XSetTile(Xdisplay, gc, viewport_pixmap);
      XSetTSOrigin(Xdisplay, gc, xsize - (x % xsize), ysize - (y % ysize));
      XSetFillStyle(Xdisplay, gc, FillTiled);
      XFillRectangle(Xdisplay, simg->pmap->pixmap, gc, 0, 0, width, height);
    }
    D_PIXMAP(("Setting background of window 0x%08x to 0x%08x\n", win, simg->pmap->pixmap));
    XSetWindowBackgroundPixmap(Xdisplay, win, simg->pmap->pixmap);
  } else
# endif
  if (image_mode_is(which, MODE_IMAGE) && image_mode_is(which, ALLOW_IMAGE)) {
    if (simg->iml->im) {
      int w = simg->pmap->w;
      int h = simg->pmap->h;
      int x = simg->pmap->x;
      int y = simg->pmap->y;

      xsize = simg->iml->im->rgb_width;
      ysize = simg->iml->im->rgb_height;
      D_PIXMAP(("render_simage():  w == %d, h == %d, x == %d, y == %d, xsize == %d, ysize == %d\n", w, h, x, y, xsize, ysize));

      if ((simg->pmap->op & OP_PROPSCALE)) {
	double x_ratio, y_ratio;

	x_ratio = ((double) width) / ((double) xsize);
	y_ratio = ((double) height) / ((double) ysize);
	if (x_ratio > 1) {
	  /* Window is larger than image.  Smaller ratio determines whether which image dimension is closer in value
	     to the corresponding window dimension, which is the scale factor we want to use */
	  if (x_ratio > y_ratio) {
	    x_ratio = y_ratio;
	  }
	} else {
	  if (x_ratio > y_ratio) {
	    x_ratio = y_ratio;
	  }
	}
	xscaled = (unsigned short) ((xsize * x_ratio) * ((float) w / 100.0));
	yscaled = (unsigned short) ((ysize * x_ratio) * ((float) h / 100.0));
      } else {
	if (w > 0) {
	  xscaled = width * ((float) w / 100.0);
	} else {
	  xscaled = xsize;
	}
	if (h > 0) {
	  yscaled = height * ((float) h / 100.0);
	} else {
	  yscaled = ysize;
	}
      }

      xpos = (short) ((width - xscaled) * ((float) x / 100.0));
      ypos = (short) ((height - yscaled) * ((float) y / 100.0));
      D_PIXMAP(("render_simage():  Calculated scaled size as %hux%hu with origin at (%hd, %hd)\n", xscaled, yscaled, xpos, ypos));

      if (simg->iml->last_w != xscaled || simg->iml->last_h != yscaled || 1) {

	simg->iml->last_w = xscaled;
	simg->iml->last_h = yscaled;

	if (simg->iml->border) {
	  D_PIXMAP(("render_simage():  Setting image border:  { left [%d], right [%d], top [%d], bottom [%d] }\n",
		    simg->iml->border->left, simg->iml->border->right, simg->iml->border->top, simg->iml->border->bottom));
	  Imlib_set_image_border(imlib_id, simg->iml->im, simg->iml->border);
	}
	if (simg->iml->mod) {
	  D_PIXMAP(("render_simage():  Setting image modifier:  { gamma [0x%08x], brightness [0x%08x], contrast [0x%08x] }\n",
		    simg->iml->mod->gamma, simg->iml->mod->brightness, simg->iml->mod->contrast));
	  Imlib_set_image_modifier(imlib_id, simg->iml->im, simg->iml->mod);
	}
	if (simg->iml->rmod) {
	  D_PIXMAP(("render_simage():  Setting image red modifier:  { gamma [0x%08x], brightness [0x%08x], contrast [0x%08x] }\n",
		    simg->iml->rmod->gamma, simg->iml->rmod->brightness, simg->iml->rmod->contrast));
	  Imlib_set_image_red_modifier(imlib_id, simg->iml->im, simg->iml->rmod);
	}
	if (simg->iml->gmod) {
	  D_PIXMAP(("render_simage():  Setting image green modifier:  { gamma [0x%08x], brightness [0x%08x], contrast [0x%08x] }\n",
		    simg->iml->gmod->gamma, simg->iml->gmod->brightness, simg->iml->gmod->contrast));
	  Imlib_set_image_green_modifier(imlib_id, simg->iml->im, simg->iml->gmod);
	}
	if (simg->iml->bmod) {
	  D_PIXMAP(("render_simage():  Setting image blue modifier:  { gamma [0x%08x], brightness [0x%08x], contrast [0x%08x] }\n",
		    simg->iml->bmod->gamma, simg->iml->bmod->brightness, simg->iml->bmod->contrast));
	  Imlib_set_image_blue_modifier(imlib_id, simg->iml->im, simg->iml->bmod);
	}
	D_PIXMAP(("render_simage():  Rendering image simg->iml->im [%8p] to %hdx%hd pixmap\n", simg->iml->im, xscaled, yscaled));
	Imlib_render(imlib_id, simg->iml->im, xscaled, yscaled);
	rendered = 1;
      }
      simg->pmap->pixmap = Imlib_copy_image(imlib_id, simg->iml->im);
      simg->pmap->mask = Imlib_copy_mask(imlib_id, simg->iml->im);
      if (simg->pmap->mask != None) {
	shaped_window_apply_mask(win, simg->pmap->mask);
      }
    } else {
      XSetWindowBackground(Xdisplay, win, PixColors[bgColor]);
      reset_simage(simg, RESET_ALL);
    }
    if (simg->pmap->pixmap != None) {
      if (pixmap != None) {
	Imlib_free_pixmap(imlib_id, pixmap);
      }
      if (xscaled != width || yscaled != height || xpos != 0 || ypos != 0) {
	unsigned char single;

	single = ((xscaled < width || yscaled < height) && !(simg->pmap->op & OP_TILE)) ? 1 : 0;

	pixmap = simg->pmap->pixmap;
	simg->pmap->pixmap = XCreatePixmap(Xdisplay, win, width, height, Xdepth);
	if (single)
	  XFillRectangle(Xdisplay, simg->pmap->pixmap, gc, 0, 0, width, height);
	XSetTile(Xdisplay, gc, pixmap);
	XSetTSOrigin(Xdisplay, gc, xpos, ypos);
	XSetFillStyle(Xdisplay, gc, FillTiled);
	if (single) {
	  XCopyArea(Xdisplay, pixmap, simg->pmap->pixmap, gc, 0, 0, xscaled, yscaled, xpos, ypos);
	} else {
	  XFillRectangle(Xdisplay, simg->pmap->pixmap, gc, 0, 0, width, height);
	}
	Imlib_free_pixmap(imlib_id, pixmap);
      }
      if (simg->iml->bevel != NULL) {
	Imlib_bevel_pixmap(imlib_id, simg->pmap->pixmap, width, height, simg->iml->bevel->edges, simg->iml->bevel->up);
      }
      D_PIXMAP(("Setting background of window 0x%08x to 0x%08x\n", win, simg->pmap->pixmap));
      XSetWindowBackgroundPixmap(Xdisplay, win, simg->pmap->pixmap);
    }
  } else {
    unsigned short cidx;

    switch (which) {
    case image_up:
    case image_down:
    case image_left:
    case image_right:
    case image_sb:
    case image_sa:
      cidx = (TermWin.focus ? scrollColor : unfocusedScrollColor);
      break;
    case image_menu:
    case image_submenu:
      cidx = (TermWin.focus ? menuColor : unfocusedMenuColor);
      break;
    default:
      cidx = bgColor;
      break;
    }
    
    XSetWindowBackground(Xdisplay, win, PixColors[cidx]);
    image_set_mode(which, MODE_SOLID);
  }
  XClearWindow(Xdisplay, win);
  XFreeGC(Xdisplay, gc);
  XSync(Xdisplay, False);
}

const char *
search_path(const char *pathlist, const char *file, const char *ext)
{
  static char name[PATH_MAX];
  char *p;
  const char *path;
  int maxpath, len;
  struct stat fst;

  if (!pathlist || !file) {	/* If either one is NULL, there really isn't much point in going on.... */
    return ((const char *) NULL);
  }
  if (!ext) {
    ext = "";
  }
  getcwd(name, PATH_MAX);
  D_OPTIONS(("search_path(\"%s\", \"%s\", \"%s\") called from \"%s\".\n", pathlist, file, ext, name));
  D_OPTIONS(("search_path():  Checking for file \"%s\"\n", file));
  if (!access(file, R_OK)) {
    if (stat(file, &fst)) {
      D_OPTIONS(("Unable to stat %s -- %s\n", file, strerror(errno)));
    } else {
      D_OPTIONS(("Stat returned mode 0x%08o, S_ISDIR() == %d\n", fst.st_mode, S_ISDIR(fst.st_mode)));
    }
    if (!S_ISDIR(fst.st_mode))
      return file;
  }
  if ((p = strchr(file, '@')) == NULL)
    p = strchr(file, '\0');
  len = (p - file);

  /* check about adding a trailing extension */
  if (ext != NULL) {

    char *dot;

    dot = strrchr(p, '.');
    path = strrchr(p, '/');
    if (dot != NULL || (path != NULL && dot <= path))
      ext = NULL;
  }
  /* leave room for an extra '/' and trailing '\0' */
  maxpath = sizeof(name) - (len + (ext ? strlen(ext) : 0) + 2);
  if (maxpath <= 0)
    return NULL;

  /* check if we can find it now */
  strncpy(name, file, len);
  name[len] = '\0';

  D_OPTIONS(("search_path():  Checking for file \"%s\"\n", name));
  if (!access(name, R_OK)) {
    stat(name, &fst);
    if (!S_ISDIR(fst.st_mode))
      return name;
  }
  if (ext) {
    strcat(name, ext);
    D_OPTIONS(("search_path():  Checking for file \"%s\"\n", name));
    if (!access(name, R_OK)) {
      stat(name, &fst);
      if (!S_ISDIR(fst.st_mode))
	return name;
    }
  }
  for (path = pathlist; path != NULL && *path != '\0'; path = p) {

    int n;

    /* colon delimited */
    if ((p = strchr(path, ':')) == NULL)
      p = strchr(path, '\0');

    n = (p - path);
    if (*p != '\0')
      p++;

    if (n > 0 && n <= maxpath) {

      strncpy(name, path, n);
      if (name[n - 1] != '/')
	name[n++] = '/';
      name[n] = '\0';
      strncat(name, file, len);

      D_OPTIONS(("search_path():  Checking for file \"%s\"\n", name));
      if (!access(name, R_OK)) {
	stat(name, &fst);
	if (!S_ISDIR(fst.st_mode))
	  return name;
      }
      if (ext) {
	strcat(name, ext);
	D_OPTIONS(("search_path():  Checking for file \"%s\"\n", name));
	if (!access(name, R_OK)) {
	  stat(name, &fst);
	  if (!S_ISDIR(fst.st_mode))
	    return name;
	}
      }
    }
  }
  D_OPTIONS(("search_path():  File \"%s\" not found in path.\n", file));
  return ((const char *) NULL);
}

unsigned short
load_image(const char *file, short type)
{
  const char *f;
  imlib_t img;
  char *geom;

  ASSERT_RVAL(file != NULL, 0);
  ASSERT_RVAL(type >= 0 && type < image_max, 0);

  D_PIXMAP(("load_image(%s, %d)\n", file, type));

  if (*file != '\0') {

    if ((geom = strchr(file, '@')) != NULL) {
      *geom++ = 0;
    } else if ((geom = strchr(file, ';')) != NULL) {
      *geom++ = 0;
    }
    if (geom != NULL) {
      set_pixmap_scale(geom, images[type].current->pmap);
    }
    if ((f = search_path(rs_path, file, PIXMAP_EXT)) == NULL) {
      f = search_path(getenv(PATH_ENV), file, PIXMAP_EXT);
    }
    if (f != NULL) {
      img.im = Imlib_load_image(imlib_id, (char *) f);
      if (img.im == NULL) {
	print_error("Unable to load image file \"%s\"", file);
	return 0;
      } else {
	reset_simage(images[type].current, (RESET_IMLIB_IM | RESET_PMAP_PIXMAP | RESET_PMAP_MASK));
	images[type].current->iml->im = img.im;
      }
      D_PIXMAP(("load_image() exiting.  images[%s].current->iml->im == %8p\n", get_image_type(type), images[type].current->iml->im));
      return 1;
    }
  }
  reset_simage(images[type].current, RESET_ALL);
  return 0;
}

# ifdef PIXMAP_OFFSET

#  define MOD_IS_SET(mod) ((mod) && ((mod)->brightness != 0xff || (mod)->contrast != 0xff || (mod)->gamma != 0xff))

unsigned char
need_colormod(register imlib_t *iml)
{
  if (MOD_IS_SET(iml->mod) || MOD_IS_SET(iml->rmod) || MOD_IS_SET(iml->gmod) || MOD_IS_SET(iml->bmod)) {
    return 1;
  } else {
    return 0;
  }
}

void
colormod_trans(Pixmap p, imlib_t *iml, GC gc, unsigned short w, unsigned short h)
{

  XImage *ximg;
  register unsigned long v, i;
  unsigned long x, y;
  int r, g, b;
  unsigned short rm, gm, bm, shade;
  ImlibColor ctab[256];
  int real_depth = 0;
  register int br, bg, bb;
  register unsigned int mr, mg, mb;

  D_PIXMAP(("colormod_trans(p == 0x%08x, gc, w == %hu, h == %hu) called.\n", p, w, h));

  if (iml->mod) {
    shade = iml->mod->brightness;
  } else {
    shade = 256;
  }
  if (iml->rmod) {
    rm = (iml->rmod->brightness * shade) >> 8;
  } else {
    rm = shade;
  }
  if (iml->gmod) {
    gm = (iml->gmod->brightness * shade) >> 8;
  } else {
    gm = shade;
  }
  if (iml->bmod) {
    bm = (iml->bmod->brightness * shade) >> 8;
  } else {
    bm = shade;
  }

  if (rm == 256 && gm == 256 && bm == 256) {
    return;			/* Nothing to do */
  }
  if (Xdepth <= 8) {

    XColor cols[256];

    for (i = 0; i < (unsigned long) (1 << Xdepth); i++) {
      cols[i].pixel = i;
      cols[i].flags = DoRed | DoGreen | DoBlue;
    }
    XQueryColors(Xdisplay, cmap, cols, 1 << Xdepth);
    for (i = 0; i < (unsigned long) (1 << Xdepth); i++) {
      ctab[i].r = cols[i].red >> 8;
      ctab[i].g = cols[i].green >> 8;
      ctab[i].b = cols[i].blue >> 8;
      ctab[i].pixel = cols[i].pixel;
    }
  } else if (Xdepth == 16) {

    XWindowAttributes xattr;

    XGetWindowAttributes(Xdisplay, desktop_window, &xattr);
    if ((xattr.visual->red_mask == 0x7c00) && (xattr.visual->green_mask == 0x3e0) && (xattr.visual->blue_mask == 0x1f)) {
      real_depth = 15;
    }
  }
  if (!real_depth) {
    real_depth = Xdepth;
  }
  ximg = XGetImage(Xdisplay, p, 0, 0, w, h, -1, ZPixmap);
  if (ximg == NULL) {
    print_warning("colormod_trans:  XGetImage(Xdisplay, 0x%08x, 0, 0, %d, %d, -1, ZPixmap) returned NULL.",
		  p, w, h);
    return;
  }
  D_PIXMAP(("XGetImage(Xdisplay, 0x%08x, 0, 0, %d, %d, -1, ZPixmap) returned %8p.", p, w, h, ximg));
  if (Xdepth <= 8) {
    D_PIXMAP(("Rendering low-depth image, depth == %d\n", (int) Xdepth));
    for (y = 0; y < h; y++) {
      for (x = 0; x < w; x++) {
	v = XGetPixel(ximg, x, y);
	r = (ctab[v & 0xff].r * rm) >> 8;
	g = (ctab[v & 0xff].g * gm) >> 8;
	b = (ctab[v & 0xff].b * bm) >> 8;
	v = Imlib_best_color_match(imlib_id, &r, &g, &b);
	XPutPixel(ximg, x, y, v);
      }
    }
  } else {
    D_PIXMAP(("Rendering high-depth image, depth == %d\n", real_depth));
    /* Determine bitshift and bitmask values */
    switch (real_depth) {
      case 15:
	br = 7;
	bg = 2;
	bb = 3;
	mr = mg = mb = 0xf8;
	break;
      case 16:
	br = 8;
	bg = bb = 3;
	mr = mb = 0xf8;
	mg = 0xfc;
	break;
      case 24:
      case 32:
	br = 16;
	bg = 8;
	bb = 0;
	mr = mg = mb = 0xff;
	break;
      default:
	print_warning("colormod_trans:  Bit depth of %d is unsupported for tinting/shading.", real_depth);
	return;
    }

    for (y = 0; y < h; y++) {
      for (x = 0; x < w; x++) {
	v = XGetPixel(ximg, x, y);
	r = ((((v >> br) & mr) * rm) >> 8) & 0xff;
	g = ((((v >> bg) & mg) * gm) >> 8) & 0xff;
	b = ((((v << bb) & mb) * bm) >> 8) & 0xff;
	v = ((r & mr) << br) | ((g & mg) << bg) | ((b & mb) >> bb);
	XPutPixel(ximg, x, y, v);
      }
    }
  }
  XPutImage(Xdisplay, p, gc, ximg, 0, 0, 0, 0, w, h);
  XDestroyImage(ximg);
}

Window
get_desktop_window(void)
{

  Atom prop, type, prop2;
  int format;
  unsigned long length, after;
  unsigned char *data;
  unsigned int nchildren;
  Window w, root, *children, parent;

  D_PIXMAP(("get_desktop_window() called.  Current desktop window is 0x%08x\n", (unsigned int) desktop_window));
  if ((prop = XInternAtom(Xdisplay, "_XROOTPMAP_ID", True)) == None) {
    D_PIXMAP(("get_desktop_window():  No _XROOTPMAP_ID found.\n"));
  }
  if ((prop2 = XInternAtom(Xdisplay, "_XROOTCOLOR_PIXEL", True)) == None) {
    D_PIXMAP(("get_desktop_window():  No _XROOTCOLOR_PIXEL found.\n"));
  }
  if (prop == None && prop2 == None) {
    return None;
  }
  if ((desktop_window != None) && (desktop_window != Xroot)) {
    XSelectInput(Xdisplay, desktop_window, None);
  }

  for (w = TermWin.parent; w; w = parent) {

    D_PIXMAP(("  Current window ID is:  0x%08x\n", w));

    if ((XQueryTree(Xdisplay, w, &root, &parent, &children, &nchildren)) == False) {
      D_PIXMAP(("    Egad!  XQueryTree() returned false!\n"));
      return None;
    }
    D_PIXMAP(("    Window is 0x%08x with %d children, root is 0x%08x, parent is 0x%08x\n",
	      w, nchildren, root, parent));
    if (nchildren) {
      XFree(children);
    }

    if (prop != None) {
      XGetWindowProperty(Xdisplay, w, prop, 0L, 1L, False, AnyPropertyType, &type, &format, &length, &after, &data);
    } else if (prop2 != None) {
      XGetWindowProperty(Xdisplay, w, prop2, 0L, 1L, False, AnyPropertyType, &type, &format, &length, &after, &data);
    } else {
      continue;
    }
    if (type != None) {
      D_PIXMAP(("get_desktop_window():  Found desktop as window 0x%08x\n", w));
      if (w != Xroot) {
        XSelectInput(Xdisplay, w, PropertyChangeMask);
      }
      if (desktop_window == w) {
        D_PIXMAP(("  Desktop window has not changed.\n"));
        return ((Window) 1);
      } else {
        D_PIXMAP(("  Desktop window has changed  Updating desktop_window.\n"));
        return (desktop_window = w);
      }
    }
  }

  D_PIXMAP(("get_desktop_window():  No suitable parent found.\n"));
  return (desktop_window = None);

}

Pixmap
get_desktop_pixmap(void)
{

  Pixmap p;
  Atom prop, type, prop2;
  int format;
  static Pixmap color_pixmap = None;
  unsigned long length, after;
  unsigned char *data;

  D_PIXMAP(("get_desktop_pixmap() called.  Current desktop pixmap is 0x%08x\n", (unsigned int) desktop_pixmap));
  if (desktop_window == None) {
    D_PIXMAP(("get_desktop_pixmap():  No desktop window.  Aborting.\n"));
    free_desktop_pixmap();
    return (desktop_pixmap = None);
  }

  prop = XInternAtom(Xdisplay, "_XROOTPMAP_ID", True);
  prop2 = XInternAtom(Xdisplay, "_XROOTCOLOR_PIXEL", True);

  if (prop == None && prop2 == None) {
    free_desktop_pixmap();
    return (desktop_pixmap = None);
  }
  if (color_pixmap != None) {
    D_PIXMAP(("get_desktop_pixmap():  Removing old solid color pixmap 0x%08x.\n", color_pixmap));
    XFreePixmap(Xdisplay, color_pixmap);
    color_pixmap = None;
  }
  if (prop != None) {
    XGetWindowProperty(Xdisplay, desktop_window, prop, 0L, 1L, False, AnyPropertyType, &type, &format, &length, &after, &data);
    if (type == XA_PIXMAP) {
      p = *((Pixmap *) data);
      if (p != None) {
        D_PIXMAP(("  Found pixmap 0x%08x\n", p));
        if (desktop_pixmap == p) {
          D_PIXMAP(("get_desktop_pixmap():  Desktop pixmap is unchanged.\n"));
          return ((Pixmap) 1);
        } else {
          D_PIXMAP(("get_desktop_pixmap():  Desktop pixmap has changed.  Updating desktop_pixmap\n"));
          free_desktop_pixmap();
          if (need_colormod(images[image_bg].current->iml)) {
            int px, py;
            unsigned int pw, ph, pb, pd;
            Window w;
            GC gc;
            XGCValues gcvalue;
            Screen *scr = ScreenOfDisplay(Xdisplay, Xscreen);

            gcvalue.foreground = gcvalue.background = PixColors[bgColor];
            gc = XCreateGC(Xdisplay, TermWin.vt, GCForeground | GCBackground, &gcvalue);
            XGetGeometry(Xdisplay, p, &w, &px, &py, &pw, &ph, &pb, &pd);
            D_PIXMAP(("get_desktop_pixmap():  XGetGeometry() returned w = 0x%08x, pw == %u, ph == %u\n", w, pw, ph));
            if (pw < (unsigned int) scr->width || ph < (unsigned int) scr->height) {
              desktop_pixmap = XCreatePixmap(Xdisplay, TermWin.parent, pw, ph, Xdepth);
              XCopyArea(Xdisplay, p, desktop_pixmap, gc, 0, 0, pw, ph, 0, 0);
              colormod_trans(desktop_pixmap, images[image_bg].current->iml, gc, pw, ph);
            } else {
              desktop_pixmap = XCreatePixmap(Xdisplay, TermWin.vt, scr->width, scr->height, Xdepth);
              XCopyArea(Xdisplay, p, desktop_pixmap, gc, 0, 0, scr->width, scr->height, 0, 0);
              colormod_trans(desktop_pixmap, images[image_bg].current->iml, gc, scr->width, scr->height);
            }
            XFreeGC(Xdisplay, gc);
            desktop_pixmap_is_mine = 1;
            return (desktop_pixmap);
          } else {
            desktop_pixmap_is_mine = 0;
            return (desktop_pixmap = p);
          }
        }
      }
    }
  }
  if (prop2 != None) {
    XGetWindowProperty(Xdisplay, desktop_window, prop2, 0L, 1L, False, AnyPropertyType, &type, &format, &length, &after, &data);
    if (type == XA_CARDINAL) {

      XGCValues gcvalue;
      GC gc;
      Pixel pix;

      free_desktop_pixmap();
      pix = *((Pixel *) data);
      D_PIXMAP(("  Found solid color 0x%08x\n", pix));
      gcvalue.foreground = pix;
      gcvalue.background = pix;
      gc = XCreateGC(Xdisplay, TermWin.vt, GCForeground | GCBackground, &gcvalue);

      color_pixmap = XCreatePixmap(Xdisplay, TermWin.vt, 16, 16, Xdepth);
      XFillRectangle(Xdisplay, color_pixmap, gc, 0, 0, 16, 16);
      D_PIXMAP(("get_desktop_pixmap():  Created solid color pixmap 0x%08x for desktop_pixmap.\n", color_pixmap));
      XFreeGC(Xdisplay, gc);
      return (desktop_pixmap = color_pixmap);
    }
  }
  D_PIXMAP(("get_desktop_pixmap():  No suitable attribute found.\n"));
  free_desktop_pixmap();
  return (desktop_pixmap = None);

}

void
free_desktop_pixmap(void)
{

  if (desktop_pixmap_is_mine && desktop_pixmap != None) {
    XFreePixmap(Xdisplay, desktop_pixmap);
    desktop_pixmap_is_mine = 0;
  }
  desktop_pixmap = None;
}

# endif				/* PIXMAP_OFFSET */

void
shaped_window_apply_mask(Drawable d, Pixmap mask)
{

  static signed char have_shape = -1;

  REQUIRE(d != None && mask != None);

  D_PIXMAP(("shaped_window_apply_mask(d [0x%08x], mask [0x%08x]) called.\n", d, mask));

# ifdef HAVE_X_SHAPE_EXT
  if (have_shape == -1) {	/* Don't know yet. */
    int unused;

    D_PIXMAP(("shaped_window_apply_mask():  Looking for shape extension.\n"));
    if (XQueryExtension(Xdisplay, "SHAPE", &unused, &unused, &unused)) {
      have_shape = 1;
    } else {
      have_shape = 0;
    }
  }
  if (have_shape == 1) {
    D_PIXMAP(("shaped_window_apply_mask():  Shape extension available, applying mask.\n"));
    XShapeCombineMask(Xdisplay, d, ShapeBounding, 0, 0, mask, ShapeSet);
  } else if (have_shape == 0) {
    D_PIXMAP(("shaped_window_apply_mask():  Shape extension not available.\n"));
    return;
  }
# else
  D_PIXMAP(("shaped_window_apply_mask():  Shape support disabled.\n"));
# endif
}

void
set_icon_pixmap(char *filename, XWMHints * pwm_hints)
{

  const char *icon_path;
  ImlibImage *temp_im;
  XWMHints *wm_hints;

  if (pwm_hints) {
    wm_hints = pwm_hints;
  } else {
    wm_hints = XGetWMHints(Xdisplay, TermWin.parent);
  }

  if (filename && *filename) {
    if ((icon_path = search_path(rs_path, filename, NULL)) == NULL)
      icon_path = search_path(getenv(PATH_ENV), filename, NULL);

    if (icon_path != NULL) {
      XIconSize *icon_sizes;
      int count, i, w = 8, h = 8;  /* At least 8x8 */

      temp_im = Imlib_load_image(imlib_id, (char *) icon_path);
      /* If we're going to render the image anyway, might as well be nice and give it to the WM in a size it likes. */
      if (XGetIconSizes(Xdisplay, Xroot, &icon_sizes, &count)) {
	for (i = 0; i < count; i++) {
	  D_PIXMAP(("Got icon sizes:  Width %d to %d +/- %d, Height %d to %d +/- %d\n", icon_sizes[i].min_width, icon_sizes[i].max_width,
		    icon_sizes[i].width_inc, icon_sizes[i].min_height, icon_sizes[i].max_height, icon_sizes[i].height_inc));
          if (icon_sizes[i].max_width > 64 || icon_sizes[i].max_height > 64) {
            continue;
          }
          /* Find the largest supported size <= 64 */
	  w = MAX(icon_sizes[i].max_width, w);
	  h = MAX(icon_sizes[i].max_height, h);
	}
	fflush(stdout);
	XFree(icon_sizes);
      } else {
        w = h = 48;
      }
      MIN_IT(w, 64);
      MIN_IT(h, 64);
      Imlib_render(imlib_id, temp_im, w, h);
      wm_hints->icon_pixmap = Imlib_copy_image(imlib_id, temp_im);
      wm_hints->icon_mask = Imlib_copy_mask(imlib_id, temp_im);
      if (check_for_enlightenment()) {
        wm_hints->flags |= IconPixmapHint | IconMaskHint;
      } else {
        wm_hints->icon_window = XCreateSimpleWindow(Xdisplay, TermWin.parent, 0, 0, w, h, 0, 0L, 0L);
        shaped_window_apply_mask(wm_hints->icon_window, wm_hints->icon_mask);
        XSetWindowBackgroundPixmap(Xdisplay, wm_hints->icon_window, wm_hints->icon_pixmap);
        wm_hints->flags |= IconWindowHint;
      }
      Imlib_destroy_image(imlib_id, temp_im);
    }
  } else {
    /* Use the default.  It's 48x48, so if the WM doesn't like it, tough cookies.  Pixmap -> ImlibImage -> Render -> Pixmap would be
       too expensive, IMHO. */
    Imlib_data_to_pixmap(imlib_id, Eterm_xpm, &wm_hints->icon_pixmap, &wm_hints->icon_mask);
    if (check_for_enlightenment()) {
      wm_hints->flags |= IconPixmapHint | IconMaskHint;
    } else {
      wm_hints->icon_window = XCreateSimpleWindow(Xdisplay, TermWin.parent, 0, 0, 48, 48, 0, 0L, 0L);
      shaped_window_apply_mask(wm_hints->icon_window, wm_hints->icon_mask);
      XSetWindowBackgroundPixmap(Xdisplay, wm_hints->icon_window, wm_hints->icon_pixmap);
      wm_hints->flags |= IconWindowHint;
    }
  }
  wm_hints->icon_x = wm_hints->icon_y = 0;
  wm_hints->flags |= IconPositionHint;

  /* Only set the hints ourselves if we were passed a NULL pointer for pwm_hints */
  if (!pwm_hints) {
    XSetWMHints(Xdisplay, TermWin.parent, wm_hints);
    XFree(wm_hints);
  }
}

# ifdef USE_EFFECTS
int
fade_in(ImlibImage *img, int frames)
{

  static int i = 0;
  register int f = frames;
  ImlibColorModifier mod;
  double gamma, brightness, contrast;

  Imlib_get_image_modifier(imlib_id, img, &mod);

  if (i < f) {
    i++;
    gamma = (double) mod.gamma / i;
    brightness = (double) mod.brightness / i;
    contrast = (double) mod.contrast / i;
    Imlib_set_image_modifier(imlib_id, img, &mod);
  } else if (i == f) {
    i = 0;
  }
  /* how many frames to go */
  return (f - i);
}
# endif				/* USE_EFFECTS */

#endif /* PIXMAP_SUPPORT */
