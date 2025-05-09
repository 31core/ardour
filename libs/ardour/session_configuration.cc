/*
 * Copyright (C) 2009-2012 David Robillard <d@drobilla.net>
 * Copyright (C) 2009-2016 Paul Davis <paul@linuxaudiosystems.com>
 * Copyright (C) 2009 Carl Hetherington <carl@carlh.net>
 * Copyright (C) 2014-2019 Robin Gareus <robin@gareus.org>
 * Copyright (C) 2016 Tim Mayberry <mojofunk@gmail.com>
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

#include <glib.h>
#include "pbd/gstdio_compat.h"
#include <glibmm/miscutils.h> /* for build_filename() */

#include "pbd/error.h"
#include "pbd/file_utils.h"
#include "pbd/pathexpand.h"

#include "temporal/types.h"
#include "temporal/types_convert.h"

#include "ardour/types.h"
#include "ardour/types_convert.h"
#include "ardour/filesystem_paths.h"
#include "ardour/session_configuration.h"
#include "ardour/utils.h"
#include "pbd/i18n.h"

using namespace ARDOUR;
using namespace PBD;

/* clang-format off */

SessionConfiguration::SessionConfiguration ()
	:
/* construct variables */
#undef  CONFIG_VARIABLE
#undef  CONFIG_VARIABLE_SPECIAL
#define CONFIG_VARIABLE(Type,var,name,value) var (name,value),
#define CONFIG_VARIABLE_SPECIAL(Type,var,name,value,mutator) var (name,value,mutator),
#include "ardour/session_configuration_vars.inc.h"
#undef  CONFIG_VARIABLE
#undef  CONFIG_VARIABLE_SPECIAL
	foo (0) // needed because above macros end in a comma
{
/* Uncomment the following to get a list of all config variables */

#if 0
#undef  CONFIG_VARIABLE
#undef  CONFIG_VARIABLE_SPECIAL
#define CONFIG_VARIABLE(Type,var,name,value) _my_variables.insert (std::make_pair ((name), &(var)));
#define CONFIG_VARIABLE_SPECIAL(Type,var,name,value,mutator) _my_variables.insert (std::make_pair ((name), &(var)));
#include "ardour/session_configuration_vars.inc.h"
#undef  CONFIG_VARIABLE
#undef  CONFIG_VARIABLE_SPECIAL

	for (auto const & s : _my_variables) {
		std::cerr << s.first << std::endl;
	}
#endif
}

XMLNode&
SessionConfiguration::get_state () const
{
	XMLNode* root;

	root = new XMLNode ("Ardour");
	root->add_child_nocopy (get_variables (X_("Config")));

	return *root;
}


XMLNode&
SessionConfiguration::get_variables (std::string const & node_name) const
{
	XMLNode* node;

	node = new XMLNode (node_name);

#undef  CONFIG_VARIABLE
#undef  CONFIG_VARIABLE_SPECIAL
#define CONFIG_VARIABLE(type,var,Name,value) \
	var.add_to_node (*node);
#define CONFIG_VARIABLE_SPECIAL(type,var,Name,value,mutator) \
	var.add_to_node (*node);
#include "ardour/session_configuration_vars.inc.h"
#undef  CONFIG_VARIABLE
#undef  CONFIG_VARIABLE_SPECIAL

	return *node;
}


int
SessionConfiguration::set_state (XMLNode const& root, int /*version*/)
{
	if (root.name() != "Ardour") {
		return -1;
	}

	for (XMLNodeConstIterator i = root.children().begin(); i != root.children().end(); ++i) {
		if ((*i)->name() == "Config") {
			set_variables (**i);
		}
	}

	return 0;
}

void
SessionConfiguration::set_variables (const XMLNode& node)
{
#undef  CONFIG_VARIABLE
#undef  CONFIG_VARIABLE_SPECIAL
#define CONFIG_VARIABLE(type,var,name,value) \
  if (var.set_from_node (node)) {            \
    ParameterChanged (name);                 \
  }

#define CONFIG_VARIABLE_SPECIAL(type,var,name,value,mutator) \
  if (var.set_from_node (node)) {                            \
    ParameterChanged (name);                                 \
  }

#include "ardour/session_configuration_vars.inc.h"
#undef  CONFIG_VARIABLE
#undef  CONFIG_VARIABLE_SPECIAL

}
void
SessionConfiguration::map_parameters (std::function<void (std::string)>& functor)
{
#undef  CONFIG_VARIABLE
#undef  CONFIG_VARIABLE_SPECIAL
#define CONFIG_VARIABLE(type,var,name,value)                 functor (name);
#define CONFIG_VARIABLE_SPECIAL(type,var,name,value,mutator) functor (name);
#include "ardour/session_configuration_vars.inc.h"
#undef  CONFIG_VARIABLE
#undef  CONFIG_VARIABLE_SPECIAL
}

/* clang-format on */

bool
SessionConfiguration::load_state ()
{
	std::string rcfile;
	GStatBuf statbuf;
	if (find_file (ardour_config_search_path(), "session.rc", rcfile)) {
		if (g_stat (rcfile.c_str(), &statbuf)) {
			return false;
		}
		if (statbuf.st_size == 0) {
			return false;
		}
		XMLTree tree;
		if (!tree.read (rcfile.c_str())) {
			error << string_compose(_("%1: cannot part default session options \"%2\""), PROGRAM_NAME, rcfile) << endmsg;
			return false;
		}

		XMLNode& root (*tree.root());
		if (root.name() != X_("SessionDefaults")) {
			warning << _("Invalid session default XML Root.") << endmsg;
			return false;
		}

		XMLNode* node;
		if (((node = find_named_node (root, X_("Config"))) != 0)) {
			set_variables(*node);
			info << _("Loaded custom session defaults.") << endmsg;
		} else {
			warning << _("Found no session defaults in XML file.") << endmsg;
			return false;
		}

		/* CUSTOM OVERRIDES */
		set_audio_search_path("");
		set_midi_search_path("");
		set_raid_path("");
	}
	return true;
}

bool
SessionConfiguration::save_state ()
{
	const std::string rcfile = Glib::build_filename (user_config_directory(), "session.rc");
	if (rcfile.empty()) {
		return false;
	}

	XMLTree tree;
	XMLNode* root = new XMLNode(X_("SessionDefaults"));
	root->add_child_nocopy (get_variables (X_("Config")));
	tree.set_root (root);

	if (!tree.write (rcfile.c_str())) {
		error << _("Could not save session options") << endmsg;
		return false;
	}

	return true;
}
