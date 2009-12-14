/*
 * Copyright (C) 2003-2009 The Music Player Daemon Project
 * http://www.musicpd.org
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

#include "filter_config.h"
#include "config.h"
#include "conf.h"
#include "filter/chain_filter_plugin.h"
#include "filter_plugin.h"
#include "filter_internal.h"
#include "filter_registry.h"

#include <string.h>

/**
 * Find the "filter" configuration block for the specified name.
 *
 * @param filter_template_name the name of the filter template
 * @return the configuration block, or NULL if none was configured
 */
static const struct config_param *
filter_plugin_config(const char *filter_template_name)
{
	const struct config_param *param = NULL;

	while ((param = config_get_next_param(CONF_AUDIO_FILTER, param)) != NULL) {
		const char *name =
			config_get_block_string(param, "name", NULL);
		if (name == NULL)
			g_error("filter configuration without 'name' name in line %d",
				param->line);

		if (strcmp(name, filter_template_name) == 0)
			return param;
	}

	return NULL;
}

/**
 * Builds a filter chain from a configuration string on the form
 * "name1, name2, name3, ..." by looking up each name among the
 * configured filter sections.
 * @param chain the chain to append filters on
 * @param spec the filter chain specification
 * @return the number of filters which were successfully added
 */
unsigned int
filter_chain_parse(struct filter *chain, const char *spec)
{

	// Split on comma
	gchar** tokens = g_strsplit_set(spec, ",", 255);

	int added_filters = 0;

	// Add each name to the filter chain by instantiating an actual filter
	char **template_names = tokens;
	while (*template_names != NULL) {
		const char *plugin_name;
		const struct filter_plugin *fp;
		struct filter *f;
		const struct config_param *cfg;

		// Squeeze whitespace
		g_strstrip(*template_names);

		cfg = filter_plugin_config(*template_names);
		if (cfg == NULL) {
			g_error("Unable to locate filter template %s",
				*template_names);
			++template_names;
			continue;
		}

		// Figure out what kind of a plugin that is
		plugin_name = config_get_block_string(cfg, "plugin", NULL);
		if (plugin_name == NULL) {
			g_error("filter configuration without 'plugin' at line %d",
				cfg->line);
			++template_names;
			continue;
		}

		// Instantiate one of those filter plugins with the template name as a hint
		fp = filter_plugin_by_name(plugin_name);
		if (fp == NULL) {
			g_error("filter plugin not found: %s",
				plugin_name);
			++template_names;
			continue;
		}

		f = filter_new(fp, cfg, NULL);
		if (f == NULL) {
			g_error("filter plugin initialization failed: %s",
				plugin_name);
			++template_names;
			continue;
		}

		filter_chain_append(chain, f);
		++added_filters;

		++template_names;
	}

	g_strfreev(tokens);

	return added_filters;
}