/*
 * Copyright © 2009-2010 Siyan Panayotov <xsisqox@gmail.com>
 *
 * This file is part of Nowspide.
 *
 * Nowspide is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Nowspide is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Nowspide.  If not, see <http://www.gnu.org/licenses/>.
 */


#ifndef __NSP_WINDOW_H_
#define __NSP_WINDOW_H_ 1

#include <gtk/gtk.h>

#include "nsp-webview.h"
#include "nsp-typedefs.h"
#include "nsp-feed-list.h"
#include "nsp-feed-item-list.h"

typedef struct _NspWindow NspWindow;

struct _NspWindow
{
	GtkBuilder *builder;
	GtkWidget *window;
	NspFeedList *feed_list;
    GtkWidget *feed_menu;
	GtkWidget *feed_item_list;
    GtkWidget *feed_item_menu;
    NspWebview *webview;
	
	NspCallback *on_feeds_update;
	NspCallback *on_feeds_add;
	NspCallback *on_feed_item_delete;
	NspCallback *on_feed_item_toggle_read;
	NspCallback *on_feeds_search;
};

NspWindow * nsp_window_new();

void	nsp_window_free 		(NspWindow *window);
int 	nsp_window_init (NspWindow *window, GError **error);

#endif /* __NSP_WINDOW_H_ */
