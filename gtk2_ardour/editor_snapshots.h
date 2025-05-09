/*
 * Copyright (C) 2009-2010 Carl Hetherington <carl@carlh.net>
 * Copyright (C) 2009-2010 Paul Davis <paul@linuxaudiosystems.com>
 * Copyright (C) 2015-2019 Robin Gareus <robin@gareus.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#pragma once

#include "ardour/session_handle.h"

#include <ytkmm/liststore.h>
#include <ytkmm/scrolledwindow.h>
#include <ytkmm/treemodel.h>
#include <ytkmm/treeview.h>

class EditorSnapshots : public ARDOUR::SessionHandlePtr
{
public:
	EditorSnapshots ();

	void set_session (ARDOUR::Session *);

	Gtk::Widget& widget () {
		return _scroller;
	}

	void redisplay ();

private:

	Gtk::ScrolledWindow _scroller;

	struct Columns : public Gtk::TreeModel::ColumnRecord {
		Columns () {
			add (current_active);
			add (visible_name);
			add (real_name);
			add (time_formatted);
		}
		Gtk::TreeModelColumn<std::string> current_active;
		Gtk::TreeModelColumn<std::string> visible_name;
		Gtk::TreeModelColumn<std::string> real_name;
		Gtk::TreeModelColumn<std::string> time_formatted;
	};

	Columns _columns;
	Glib::RefPtr<Gtk::ListStore> _snapshot_model;
	Gtk::TreeView _snapshot_display;
	Gtk::Menu _menu;

	bool button_press (GdkEventButton *);
	void popup_context_menu (int, int32_t, std::string);
	void remove (std::string);
	void rename (std::string);
};

