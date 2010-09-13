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

static const char cvs_ident[] = "$Id$";

#include "config.h"
#include "feature.h"

#include <unistd.h>
#include <limits.h>

#include "startup.h"
#include "actions.h"
#include "command.h"
#include "e.h"
#include "events.h"
#include "menus.h"
#include "options.h"
#include "pixmap.h"
#include "screen.h"
#include "script.h"
#include "scrollbar.h"
#include "term.h"
#include "windows.h"
#ifdef ESCREEN
#  include "screamcfg.h"
#endif

static spif_bool_t action_handle_string(event_t *ev, spif_eterm_action_t action);
static spif_bool_t action_handle_echo(event_t *ev, spif_eterm_action_t action);
static spif_bool_t action_handle_script(event_t *ev, spif_eterm_action_t action);
static spif_bool_t action_handle_menu(event_t *ev, spif_eterm_action_t action);
static spif_bool_t action_check_button(spif_uchar_t button, int x_button);
static spif_bool_t action_check_keysym(KeySym keysym, KeySym x_keysym);
static spif_bool_t action_check_modifiers(spif_ushort_t mod, int x_mod);
static spif_bool_t action_matches_event(spif_eterm_action_t action, event_t *ev);

/* Global action list.  FIXME:  Should be member of terminal object. */
spif_vector_t actions = NULL;

/* *INDENT-OFF* */
static SPIF_CONST_TYPE(class) ea_class = {
    SPIF_DECL_CLASSNAME(eterm_action),
    (spif_func_t) spif_eterm_action_new,
    (spif_func_t) spif_eterm_action_init,
    (spif_func_t) spif_eterm_action_done,
    (spif_func_t) spif_eterm_action_del,
    (spif_func_t) spif_eterm_action_show,
    (spif_func_t) spif_eterm_action_comp,
    (spif_func_t) spif_eterm_action_dup,
    (spif_func_t) spif_eterm_action_type
};
SPIF_TYPE(class) SPIF_CLASS_VAR(eterm_action) = &ea_class;
/* *INDENT-ON* */

spif_eterm_action_t
spif_eterm_action_new(void)
{
    spif_eterm_action_t self;

    self = SPIF_ALLOC(eterm_action);
    if (!spif_eterm_action_init(self)) {
        SPIF_DEALLOC(self);
        self = SPIF_NULL_TYPE(eterm_action);
    }
    return self;
}

spif_eterm_action_t
spif_eterm_action_new_from_data(spif_eterm_action_type_t type, spif_ushort_t modifiers,
                                spif_uchar_t button, KeySym keysym, spif_ptr_t param)
{
    spif_eterm_action_t self;

    self = SPIF_ALLOC(eterm_action);
    if (!spif_eterm_action_init_from_data(self, type, modifiers, button, keysym, param)) {
        SPIF_DEALLOC(self);
        self = SPIF_NULL_TYPE(eterm_action);
    }
    return self;
}

spif_bool_t
spif_eterm_action_del(spif_eterm_action_t self)
{
    spif_eterm_action_done(self);
    SPIF_DEALLOC(self);
    return TRUE;
}

spif_bool_t
spif_eterm_action_init(spif_eterm_action_t self)
{
    spif_obj_init(SPIF_OBJ(self));
    spif_obj_set_class(SPIF_OBJ(self), SPIF_CLASS_VAR(eterm_action));
    self->type = ETERM_ACTION_NONE;
    self->modifiers = ETERM_MOD_NONE;
    self->button = ETERM_BUTTON_NONE;
    self->keysym = ETERM_KEYSYM_NONE;
    self->handler = SPIF_NULL_TYPE(eterm_action_handler);
    self->parameter = SPIF_NULL_TYPE(eterm_action_parameter);
    return TRUE;
}

spif_bool_t
spif_eterm_action_init_from_data(spif_eterm_action_t self, spif_eterm_action_type_t type,
                                 spif_ushort_t modifiers, spif_uchar_t button, KeySym keysym, spif_ptr_t param)
{
    spif_obj_init(SPIF_OBJ(self));
    spif_obj_set_class(SPIF_OBJ(self), SPIF_CLASS_VAR(eterm_action));
    self->type = type;
    self->modifiers = modifiers;
    self->button = button;
    self->keysym = keysym;

    switch (type) {
        case ETERM_ACTION_STRING:
            self->handler = SPIF_CAST(eterm_action_handler) action_handle_string;
            self->parameter = SPIF_CAST(obj) spif_str_new_from_ptr(param);
            /*parse_escaped_string(self->parameter.string); */
            break;
        case ETERM_ACTION_ECHO:
            self->handler = SPIF_CAST(eterm_action_handler) action_handle_echo;
            self->parameter = SPIF_CAST(obj) spif_str_new_from_ptr(param);
            /*parse_escaped_string(self->parameter.string); */
            break;
        case ETERM_ACTION_SCRIPT:
            self->handler = SPIF_CAST(eterm_action_handler) action_handle_script;
            self->parameter = SPIF_CAST(obj) spif_str_new_from_ptr(param);
            break;
        case ETERM_ACTION_MENU:
            self->handler = SPIF_CAST(eterm_action_handler) action_handle_menu;
            /*self->parameter.menu = (menu_t *) param; */
            break;
        default:
            break;
    }
    D_ACTIONS(("Added action.  modifiers == 0x%08x, button == %d, keysym == 0x%08x\n",
               self->modifiers, self->button, SPIF_CAST_C(unsigned) self->keysym));

    return TRUE;
}

spif_bool_t
spif_eterm_action_done(spif_eterm_action_t self)
{
    if (!SPIF_OBJ_ISNULL(self->parameter)) {
        if ((self->type == ETERM_ACTION_STRING)
            || (self->type == ETERM_ACTION_ECHO)
            || (self->type == ETERM_ACTION_SCRIPT)) {
            spif_str_del(SPIF_STR(self->parameter));
        }
    }
    self->type = ETERM_ACTION_NONE;
    self->modifiers = ETERM_MOD_NONE;
    self->button = ETERM_BUTTON_NONE;
    self->keysym = ETERM_KEYSYM_NONE;
    self->handler = SPIF_NULL_TYPE(eterm_action_handler);
    self->parameter = SPIF_NULL_TYPE(eterm_action_parameter);
    spif_obj_done(SPIF_OBJ(self));
    return TRUE;
}

spif_str_t
spif_eterm_action_show(spif_eterm_action_t self, spif_charptr_t name, spif_str_t buff, size_t indent)
{
    char tmp[4096];

    if (SPIF_ETERM_ACTION_ISNULL(self)) {
        SPIF_OBJ_SHOW_NULL(eterm_action, name, buff, indent, tmp);
        return buff;
    }

    memset(tmp, ' ', indent);
    snprintf(tmp + indent, sizeof(tmp) - indent, "(spif_eterm_action_t) %s:  {\n", name);
    if (SPIF_STR_ISNULL(buff)) {
        buff = spif_str_new_from_ptr(tmp);
    } else {
        spif_str_append_from_ptr(buff, tmp);
    }

    snprintf(tmp + indent, sizeof(tmp) - indent, "  (spif_eterm_action_type_t) type:  %d\n", self->type);
    spif_str_append_from_ptr(buff, tmp);
    snprintf(tmp + indent, sizeof(tmp) - indent, "  (spif_ushort_t) modifiers:  %c%c%c%c\n", SHOW_MODS(self->modifiers));
    spif_str_append_from_ptr(buff, tmp);
    snprintf(tmp + indent, sizeof(tmp) - indent, "  (spif_uchar_t) button:  %d\n", self->button);
    spif_str_append_from_ptr(buff, tmp);
    snprintf(tmp + indent, sizeof(tmp) - indent, "  (KeySym) keysym:  %04x\n", SPIF_CAST_C(unsigned) self->keysym);

    spif_str_append_from_ptr(buff, tmp);
    snprintf(tmp + indent, sizeof(tmp) - indent, "  (spif_eterm_action_handler_t) handler:  %10p\n", self->handler);
    spif_str_append_from_ptr(buff, tmp);
    snprintf(tmp + indent, sizeof(tmp) - indent, "  (spif_eterm_action_parameter_t) parameter:  %10p\n", self->parameter);
    spif_str_append_from_ptr(buff, tmp);

    snprintf(tmp + indent, sizeof(tmp) - indent, "}\n");
    spif_str_append_from_ptr(buff, tmp);
    return buff;
}

spif_cmp_t
spif_eterm_action_comp(spif_eterm_action_t self, spif_eterm_action_t other)
{
    spif_cmp_t c;

    SPIF_OBJ_COMP_CHECK_NULL(self, other);
    c = SPIF_CMP_FROM_INT(SPIF_CAST_C(int) (self->type) - SPIF_CAST_C(int) (other->type));

    if (!SPIF_CMP_IS_EQUAL(c)) {
        return c;
    }
    c = SPIF_CMP_FROM_INT(SPIF_CAST_C(int) (self->button) - SPIF_CAST_C(int) (other->button));

    if (!SPIF_CMP_IS_EQUAL(c)) {
        return c;
    }
    c = SPIF_CMP_FROM_INT(SPIF_CAST_C(int) (self->keysym) - SPIF_CAST_C(int) (other->keysym));

    if (!SPIF_CMP_IS_EQUAL(c)) {
        return c;
    }
    return SPIF_CMP_FROM_INT(SPIF_CAST_C(int) (self->modifiers) - SPIF_CAST_C(int) (other->modifiers));
}

spif_eterm_action_t
spif_eterm_action_dup(spif_eterm_action_t self)
{
    spif_eterm_action_t tmp;

    REQUIRE_RVAL(!SPIF_ETERM_ACTION_ISNULL(self), SPIF_NULL_TYPE(eterm_action));
    tmp = spif_eterm_action_new();
    tmp->type = self->type;
    tmp->modifiers = self->modifiers;
    tmp->button = self->button;
    tmp->keysym = self->keysym;
    tmp->handler = self->handler;
    tmp->parameter = self->parameter;
    return tmp;
}

spif_classname_t
spif_eterm_action_type(spif_eterm_action_t self)
{
    return SPIF_OBJ_CLASSNAME(self);
}

SPIF_DEFINE_PROPERTY_FUNC_NONOBJ(eterm_action, eterm_action_type, type);
SPIF_DEFINE_PROPERTY_FUNC_NONOBJ(eterm_action, ushort, modifiers);
SPIF_DEFINE_PROPERTY_FUNC_NONOBJ(eterm_action, uchar, button);
SPIF_DEFINE_PROPERTY_FUNC_C(eterm_action, KeySym, keysym);
SPIF_DEFINE_PROPERTY_FUNC_NONOBJ(eterm_action, eterm_action_handler, handler);
SPIF_DEFINE_PROPERTY_FUNC_NONOBJ(eterm_action, eterm_action_parameter, parameter);










static spif_bool_t
action_handle_string(event_t *ev, spif_eterm_action_t action)
{
    REQUIRE_RVAL(!SPIF_PTR_ISNULL(ev), FALSE);
    REQUIRE_RVAL(!SPIF_STR_ISNULL(action->parameter), FALSE);
    cmd_write(SPIF_STR_STR(action->parameter), spif_str_get_len(SPIF_STR(action->parameter)));
    return 1;
}

static spif_bool_t
action_handle_echo(event_t *ev, spif_eterm_action_t action)
{
    REQUIRE_RVAL(!SPIF_PTR_ISNULL(ev), FALSE);
    REQUIRE_RVAL(!SPIF_STR_ISNULL(action->parameter), FALSE);
#ifdef ESCREEN
    if (TermWin.screen && TermWin.screen->backend) {
#  ifdef NS_HAVE_SCREEN
        /* translate escapes */
        ns_parse_screen_interactive(TermWin.screen, SPIF_STR_STR(action->parameter));
#  endif
    } else
#endif
        tt_write(SPIF_STR_STR(action->parameter), spif_str_get_len(SPIF_STR(action->parameter)));
    return 1;
}

static spif_bool_t
action_handle_script(event_t *ev, spif_eterm_action_t action)
{
    REQUIRE_RVAL(!SPIF_PTR_ISNULL(ev), FALSE);
    REQUIRE_RVAL(!SPIF_STR_ISNULL(action->parameter), FALSE);
    script_parse(SPIF_STR_STR(action->parameter));
    return 1;
}

static spif_bool_t
action_handle_menu(event_t *ev, spif_eterm_action_t action)
{
    REQUIRE_RVAL(!SPIF_PTR_ISNULL(ev), FALSE);
    REQUIRE_RVAL(!SPIF_PTR_ISNULL(action->parameter), FALSE);
    /*menu_invoke(ev->xbutton.x, ev->xbutton.y, TermWin.parent, action->parameter.menu, ev->xbutton.time); */
    return 1;
}

static spif_bool_t
action_check_button(spif_uchar_t button, int x_button)
{
    /* The event we're looking at is a button press.  Make sure the
       current action is also, and that it matches.  Continue if not. */
    D_ACTIONS(("Checking button %d vs x_button %d\n", button, x_button));
    if (button == ETERM_BUTTON_NONE) {
        /* It was a button press, and this action is not a button action. */
        return FALSE;
    }
    if ((button != ETERM_BUTTON_ANY) && (button != x_button)) {
        /* It's a specific button, and the two don't match. */
        return FALSE;
    }
    D_ACTIONS(("Button match confirmed.\n"));
    return TRUE;
}

static spif_bool_t
action_check_keysym(KeySym keysym, KeySym x_keysym)
{
    /* The event we're looking at is a key press.  Make sure the
       current action is also, and that it matches.  Continue if not. */
    D_ACTIONS(("Checking keysym 0x%08x vs x_keysym 0x%08x\n", keysym, x_keysym));
    if (keysym == None) {
        return FALSE;
    } else if (keysym != x_keysym) {
        return FALSE;
    }
    D_ACTIONS(("Keysym match confirmed.\n"));
    return TRUE;
}

static spif_bool_t
action_check_modifiers(spif_ushort_t mod, int x_mod)
{
    spif_uint32_t m = (AltMask | MetaMask | NumLockMask);

    /* When we do have to check the modifiers, we do so in this order to eliminate the
       most popular choices first.  If any test fails, we return FALSE. */
    D_ACTIONS(("Checking modifier set 0x%08x (%c%c%c%c) vs. X modifier set 0x%08x (%c%c%c%c)\n",
               mod, SHOW_MODS(mod), x_mod, SHOW_X_MODS(x_mod)));
    if (mod != ETERM_MOD_ANY) {
        /* LOGICAL_XOR() returns true if either the first parameter or the second parameter
           is true, but not both...just like XOR.  If the mask we're looking for is set in
           mod but not in x_mod, or set in x_mod but not in mod, we don't have a match. */
        if (LOGICAL_XOR((mod & ETERM_MOD_CTRL), (x_mod & ControlMask))) {
            return FALSE;
        }
        if (LOGICAL_XOR((mod & ETERM_MOD_SHIFT), (x_mod & ShiftMask))) {
            return FALSE;
        }
        if (MetaMask != AltMask) {
            if (LOGICAL_XOR((mod & ETERM_MOD_ALT), (x_mod & AltMask))) {
                return FALSE;
            }
            if (LOGICAL_XOR((mod & ETERM_MOD_META), (x_mod & MetaMask))) {
                return FALSE;
            }
        } else {
            if (LOGICAL_XOR((mod & (ETERM_MOD_META | ETERM_MOD_ALT)), (x_mod & (MetaMask | AltMask)))) {
                return FALSE;
            }
        }
        if (LOGICAL_XOR((mod & ETERM_MOD_LOCK), (x_mod & LockMask))) {
            return FALSE;
        }
        /* These tests can't use LOGICAL_XOR because the second test has an additional
           restriction that the Mod?Mask cannot be set in m; i.e., we want to ignore
           any Mod?Mask assigned to Alt, Meta, or the NumLock On state. */
        if (((mod & ETERM_MOD_MOD1) && !(x_mod & Mod1Mask)) || (!(mod & ETERM_MOD_MOD1) && (x_mod & Mod1Mask) && !(Mod1Mask & m))) {
            return FALSE;
        }
        if (((mod & ETERM_MOD_MOD2) && !(x_mod & Mod2Mask)) || (!(mod & ETERM_MOD_MOD2) && (x_mod & Mod2Mask) && !(Mod2Mask & m))) {
            return FALSE;
        }
        if (((mod & ETERM_MOD_MOD3) && !(x_mod & Mod3Mask)) || (!(mod & ETERM_MOD_MOD3) && (x_mod & Mod3Mask) && !(Mod3Mask & m))) {
            return FALSE;
        }
        if (((mod & ETERM_MOD_MOD4) && !(x_mod & Mod4Mask)) || (!(mod & ETERM_MOD_MOD4) && (x_mod & Mod4Mask) && !(Mod4Mask & m))) {
            return FALSE;
        }
        if (((mod & ETERM_MOD_MOD5) && !(x_mod & Mod5Mask)) || (!(mod & ETERM_MOD_MOD5) && (x_mod & Mod5Mask) && !(Mod5Mask & m))) {
            return FALSE;
        }
    }
    D_ACTIONS(("Modifier match confirmed.\n"));
    return TRUE;
}

spif_bool_t
eterm_action_dispatch(event_t *ev)
{
    spif_eterm_action_t action;
    spif_iterator_t iter;

    ASSERT_RVAL(ev != NULL, FALSE);
    ASSERT_RVAL(ev->xany.type == ButtonPress || ev->xany.type == KeyPress, FALSE);
    D_ACTIONS(("Event %8p:  Button %d, Keycode %d, Key State 0x%08x (modifiers %c%c%c%c)\n",
               ev, ev->xbutton.button, SPIF_CAST_C(int) ev->xkey.keycode, ev->xkey.state, SHOW_X_MODS(ev->xkey.state)));
    D_ACTIONS(("Searching %d actions to find match.\n", SPIF_VECTOR_COUNT(actions)));

    for (iter = SPIF_VECTOR_ITERATOR(actions); SPIF_ITERATOR_HAS_NEXT(iter);) {
        action = SPIF_CAST(eterm_action) SPIF_ITERATOR_NEXT(iter);
        if (action_matches_event(action, ev)) {
            D_ACTIONS(("Spawning handler for action object %10p.\n", action));
            return SPIF_CAST(bool) ((SPIF_CAST(eterm_action_handler) (action->handler)) (ev, action));
        }
    }
    return FALSE;
}

static spif_bool_t
action_matches_event(spif_eterm_action_t action, event_t *ev)
{
    action_t *action;

    if (!action_list || !(action = action_find_match(mod, button, keysym))) {
        action = (action_t *) MALLOC(sizeof(action_t));
        action->next = action_list;
        action_list = action;
    } else {
        ASSERT_NOTREACHED_RVAL(FALSE);
    }
    if (action_check_modifiers(action->modifiers, ev->xkey.state)) {
        D_ACTIONS(("Match found.\n"));
        return TRUE;
    }
    return FALSE;
}
