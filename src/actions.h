/*
 * Copyright (C) 1997-2009, Michael Jennings
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

#ifndef _ACTIONS_H_
#define _ACTIONS_H_

#include <X11/Xfuncproto.h>
#include <X11/Intrinsic.h>	/* Xlib, Xutil, Xresource, Xfuncproto */
#include <libast.h>

#include "events.h"
#include "menus.h"

/************ Macros and Definitions ************/
typedef enum {
    ETERM_ACTION_NONE = 0,
    ETERM_ACTION_STRING,
    ETERM_ACTION_ECHO,
    ETERM_ACTION_SCRIPT,
    ETERM_ACTION_MENU
} SPIF_TYPE(eterm_action_type);
typedef SPIF_TYPE(obj) SPIF_TYPE(eterm_action_parameter);
typedef spif_func_t SPIF_TYPE(eterm_action_handler);

#define ETERM_KEYSYM_NONE        (0UL)

#define ETERM_MOD_NONE           (0UL)
#define ETERM_MOD_CTRL           (1UL << 0)
#define ETERM_MOD_SHIFT          (1UL << 1)
#define ETERM_MOD_LOCK           (1UL << 2)
#define ETERM_MOD_META           (1UL << 3)
#define ETERM_MOD_ALT            (1UL << 4)
#define ETERM_MOD_MOD1           (1UL << 5)
#define ETERM_MOD_MOD2           (1UL << 6)
#define ETERM_MOD_MOD3           (1UL << 7)
#define ETERM_MOD_MOD4           (1UL << 8)
#define ETERM_MOD_MOD5           (1UL << 9)
#define ETERM_MOD_ANY            (1UL << 10)

#define ETERM_BUTTON_NONE        (0)
#define ETERM_BUTTON_ANY         (0xff)

#define LOGICAL_XOR(a, b)  !(((a) && (b)) || (!(a) && !(b)))

#define SHOW_MODS(m)       ((m & ETERM_MOD_CTRL) ? 'C' : 'c'), ((m & ETERM_MOD_SHIFT) ? 'S' : 's'), ((m & ETERM_MOD_META) ? 'M' : 'm'), ((m & ETERM_MOD_ALT) ? 'A' : 'a')
#define SHOW_X_MODS(m)     ((m & ControlMask) ? 'C' : 'c'), ((m & ShiftMask) ? 'S' : 's'), ((m & MetaMask) ? 'M' : 'm'), ((m & AltMask) ? 'A' : 'a')
#define MOD_FMT            "%c%c%c%c"






/* Cast an arbitrary object pointer to a eterm_action. */
#define SPIF_ETERM_ACTION(obj)                (SPIF_CAST(eterm_action) (obj))

/* Check to see if a pointer references a eterm_actioning object. */
#define SPIF_OBJ_IS_ETERM_ACTION(obj)         (SPIF_OBJ_IS_TYPE(obj, eterm_action))

/* Check for NULL eterm_action object */
#define SPIF_ETERM_ACTION_ISNULL(s)           SPIF_OBJ_ISNULL(SPIF_OBJ(s))

SPIF_DECL_OBJ(eterm_action) {
    SPIF_DECL_PARENT_TYPE(obj);
    SPIF_DECL_PROPERTY(eterm_action_type, type);
    SPIF_DECL_PROPERTY(ushort, modifiers);
    SPIF_DECL_PROPERTY(uchar, button);
    SPIF_DECL_PROPERTY_C(KeySym, keysym);
    SPIF_DECL_PROPERTY(eterm_action_handler, handler);
    SPIF_DECL_PROPERTY(obj, parameter);
};

extern spif_vector_t actions;
extern spif_class_t SPIF_CLASS_VAR(eterm_action);
extern spif_eterm_action_t spif_eterm_action_new(void);
extern spif_eterm_action_t spif_eterm_action_new_from_data(spif_eterm_action_type_t, spif_ushort_t,
                                                           spif_uchar_t, KeySym, spif_ptr_t);
extern spif_bool_t spif_eterm_action_del(spif_eterm_action_t);
extern spif_bool_t spif_eterm_action_init(spif_eterm_action_t);
extern spif_bool_t spif_eterm_action_init_from_data(spif_eterm_action_t, spif_eterm_action_type_t,
                                                    spif_ushort_t, spif_uchar_t, KeySym, spif_ptr_t);
extern spif_bool_t spif_eterm_action_done(spif_eterm_action_t);
extern spif_eterm_action_t spif_eterm_action_dup(spif_eterm_action_t);
extern spif_cmp_t spif_eterm_action_comp(spif_eterm_action_t, spif_eterm_action_t);
extern spif_str_t spif_eterm_action_show(spif_eterm_action_t, spif_charptr_t, spif_str_t, size_t);
extern spif_classname_t spif_eterm_action_type(spif_eterm_action_t);
SPIF_DECL_PROPERTY_FUNC(eterm_action, eterm_action_type, type);
SPIF_DECL_PROPERTY_FUNC(eterm_action, ushort, modifiers);
SPIF_DECL_PROPERTY_FUNC(eterm_action, uchar, button);
SPIF_DECL_PROPERTY_FUNC_C(eterm_action, KeySym, keysym);
SPIF_DECL_PROPERTY_FUNC(eterm_action, eterm_action_handler, handler);
SPIF_DECL_PROPERTY_FUNC(eterm_action, eterm_action_parameter, parameter);

extern spif_bool_t eterm_action_dispatch(event_t *ev);

#endif	/* _ACTIONS_H_ */
