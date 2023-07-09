#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <xcb/xcb.h>
#include <xcb/xcb_aux.h>
#include <xcb/xcb_event.h>
#include <xcb/xcb_icccm.h>
#include <xcb/xproto.h>

static xcb_connection_t *conn;
static xcb_screen_t *scr;
static xcb_atom_t atom;

void init_atom() {
  // Get `_NET_WM_NAME` atom
  const char *atom_name = "_NET_WM_NAME";
  xcb_intern_atom_cookie_t atom_cookie =
      xcb_intern_atom(conn, true, strlen(atom_name), atom_name);
  xcb_intern_atom_reply_t *atom_reply =
      xcb_intern_atom_reply(conn, atom_cookie, NULL);
  if (!atom_reply) {
    fprintf(stderr, "_NET_WM_NAME atom not found, WTF?\n");
    return;
  }
  atom = atom_reply->atom;
  free(atom_reply);
}

void handle_wechat(xcb_window_t window) {
  // Get value of `_NET_WM_NAME`
  xcb_icccm_get_text_property_reply_t prop;
  if (!xcb_icccm_get_text_property_reply(
          conn, xcb_icccm_get_text_property(conn, window, atom), &prop, NULL)) {
    fprintf(stderr, "Can't get _NET_WM_NAME property\n");
    return;
  }
  
  if (prop.name_len) {
    printf("Normal window with name: %.*s\n", prop.name_len, prop.name);
    return;
  }
  // If `_NET_WM_NAME` is empty, check if this windows accept input
  xcb_icccm_wm_hints_t hints;
  if (!xcb_icccm_get_wm_hints_reply(conn, xcb_icccm_get_wm_hints(conn, window),
                                    &hints, NULL)) {
    fprintf(stderr, "Can't get WM_HINTS property\n");
    return;
  }
  if ((hints.flags & XCB_ICCCM_WM_HINT_INPUT) && hints.input) {
    printf("Normal dialog without name\n");
    return;
  }

  // Retrieve the window geometry
  xcb_get_geometry_cookie_t geom_cookie = xcb_get_geometry(
      conn, window);
  xcb_get_geometry_reply_t* geom_reply = xcb_get_geometry_reply(
      conn, geom_cookie, NULL);

  printf("size = (%d, %d)\n", geom_reply->width, geom_reply->height);
  bool is_menu_windows = (geom_reply->width < 400 &&
         geom_reply->height < 400) ||
         geom_reply->width < geom_reply->height;
  free(geom_reply);

  if (is_menu_windows) {
    printf("Size too small, skip.\n");
    return;
  }
  // only choose the shadow window
  printf("Black shadow window, unmap it!\n");
  xcb_unmap_window(conn, window);
  
  return;
}

bool is_wechat(xcb_window_t window) {
  xcb_get_property_cookie_t cookie = xcb_get_property(
      conn, 0, window, XCB_ATOM_WM_CLASS, XCB_ATOM_STRING, 0, 32);
  xcb_get_property_reply_t *reply = xcb_get_property_reply(conn, cookie, NULL);
  if (!reply) {
    return false;
  }
  int len = xcb_get_property_value_length(reply);
  if (!len) {
    free(reply);
    return false;
  }
  char *property = (char *)xcb_get_property_value(reply);
  printf("0x%0x8: WM_CLASS= %.*s\n", window, len, property);
  bool result = false;
  if (!strcmp(property, "wechat.exe")) {
    return true;
  }

  free(reply);
  return result;
}

int main(int argc, char **argv) {
  conn = xcb_connect(NULL, NULL);
  if (xcb_connection_has_error(conn)) {
    fprintf(stderr, "Failed to connect to the X server\n");
    exit(1);
  }
  scr = xcb_setup_roots_iterator(xcb_get_setup(conn)).data;
  if (!scr) {
    fprintf(stderr, "Failed to get X screen\n");
    exit(2);
  }

  uint32_t val[] = {XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY};
  xcb_change_window_attributes(conn, scr->root, XCB_CW_EVENT_MASK, val);

  init_atom();

  while (1) {
    xcb_aux_sync(conn);
    xcb_generic_event_t *e = xcb_wait_for_event(conn);
    if (XCB_EVENT_RESPONSE_TYPE(e) == XCB_MAP_NOTIFY) {
      xcb_map_notify_event_t *map = (xcb_map_notify_event_t *)e;
      if (is_wechat(map->window)) {
        handle_wechat(map->window);
      }
    }
    free(e);
  }

  if (conn) {
    xcb_disconnect(conn);
  }
  return 0;
}
