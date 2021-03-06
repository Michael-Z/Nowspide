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

#define _GNU_SOURCE 1

#include "nsp-feed-parser.h"

#include <assert.h>
#include <string.h>
#include <time.h>

#define ATOM_0_3_URI	"http://purl.org/atom/ns#"
#define ATOM_1_0_URI	"http://www.w3.org/2005/Atom"

static char* nsp_parser_sanitize_string(char*, gboolean);

static NspFeedType
nsp_parse_feed_type(xmlNode *node)
{
	if (node->name && node->type == XML_ELEMENT_NODE) {
		if (strcmp((const char *)node->name, "rss")==0) {
			xmlChar * version = xmlGetProp(node, (const xmlChar *)"version");
			if (!version) {
				xmlFree(version);
				g_warning("No RSS version");
			} else if (strcmp((char*)version, "0.91")==0 || strcmp((char*)version, "0.92")==0 || strcmp((char*)version, "0.94")==0) {
				xmlFree(version);
				return NSP_FEED_RSS_0_9;
			} else if (strcmp((char*)version, "1.0")==0 || strcmp((char*)version, "1")==0) {
				xmlFree(version);
				return NSP_FEED_RSS_1_0;
			} else if (strcmp((char*)version, "2.0")==0 || strcmp((char*)version, "2")==0) {
				xmlFree(version);
				return NSP_FEED_RSS_2_0;
			} else {
				xmlFree(version);
				g_warning("Unknown RSS version");
			}
		} else if (strcmp((const char *)node->name, "RDF")==0) {
			return NSP_FEED_RSS_1_0;
		} else if (strcmp((const char *)node->name, "feed")==0) {
			if (node->ns && node->ns->href) {
				if (strcmp((const char *)node->ns->href, ATOM_0_3_URI)==0) {
					return NSP_FEED_ATOM_0_3;
				} else if (strcmp((const char *)node->ns->href, ATOM_1_0_URI)==0) {
					return NSP_FEED_ATOM_1_0;
				} else {
					const char * version = (const char *)xmlGetProp(node, (const xmlChar *)"version");
					if (!version) {
						xmlFree((void *)version);
						g_warning("Unknown Atom version");
					} else if (strcmp(version, "0.3")==0) {
						xmlFree((void *)version);
						return NSP_FEED_ATOM_0_3;
					} else {
						xmlFree((void *)version);
						g_warning("Unknown Atom version");
					}
				}
			} else {
				g_warning("No Atom version");
			}
		}
	} else {
		g_warning("Invalid xml");
	}
	
	return NSP_FEED_UNKNOWN;
}

GList *
nsp_parse_items_rss (xmlNode *root, GError **error) {
	xmlNode *tpm = NULL;
	xmlNode *item = NULL;
	xmlNode *prop = NULL;
	time_t date;
	
	
	GList *items = NULL;
	
	assert(root != NULL);
	
	tpm = root->children;
	while( tpm != NULL ) {
		if ( !xmlStrcasecmp(tpm->name, (xmlChar *)"channel") ) {
			item = tpm->children;
			while ( item != NULL) {
				if ( strcmp((char *)item->name, "item") ) {
					item = item->next;
					continue;
				}
				NspFeedItem *feed_item = nsp_feed_item_new();
				
				prop = item->children;
				while ( prop != NULL ) {
					if ( !xmlStrcasecmp(prop->name, (xmlChar *)"title") ) {
						feed_item->title = nsp_parser_sanitize_string((char*) xmlNodeGetContent(prop), FALSE);
					} else if( !xmlStrcasecmp(prop->name, (xmlChar *)"link") ) {
						feed_item->link = (char*) xmlNodeGetContent(prop);
					} else if( !xmlStrcasecmp(prop->name, (xmlChar *)"description") && feed_item->description == NULL ) {
						feed_item->description = (char*) xmlNodeGetContent(prop);
					} else if( !xmlStrcasecmp(prop->name, (xmlChar *)"encoded") ) {
						feed_item->description = (char*) xmlNodeGetContent(prop);
					} else if( !xmlStrcasecmp(prop->name, (xmlChar *)"pubdate") ) {
						if ( feed_item->pubdate == NULL ) {
							feed_item->pubdate = malloc(sizeof(struct tm));
							assert(feed_item->pubdate != NULL);
							feed_item->pubdate->tm_zone = NULL;
						}
						strptime((const char*) xmlNodeGetContent(prop), "%a, %d %b %Y %H:%M:%S %z", feed_item->pubdate);
						date = timegm(feed_item->pubdate);
						memcpy( feed_item->pubdate, localtime(&date), sizeof(struct tm));
					}
					prop = prop->next;
				}
				
				if ( feed_item->pubdate == NULL ) {
					date = time(NULL);
					feed_item->pubdate = malloc(sizeof(struct tm));
					memcpy( feed_item->pubdate, localtime(&date), sizeof(struct tm));
				}
				
				items = g_list_prepend(items, (gpointer)feed_item);
				
				item = item->next;
			}
			break; // parse only one channel per feed
		}
		tpm = tpm->next;
	}
	
	return items;
}

static int
nsp_parse_feed_rss(xmlNode *node, NspFeed *feed)
{
	xmlNode *tmp = node->children;
	
	while ( tmp != NULL ) {
		if ( !xmlStrcasecmp(tmp->name, (xmlChar *)"channel") ) {
			tmp = tmp->children;
			
			while ( tmp != NULL ) {
				if ( !xmlStrcasecmp(tmp->name, (xmlChar *) "title") ) {
					if (feed->title != NULL) {
						free(feed->title);
					}
					feed->title = nsp_parser_sanitize_string( (char *) xmlNodeGetContent(tmp), TRUE );
				} else if ( !xmlStrcasecmp(tmp->name, (xmlChar *) "description") && feed->description == NULL ) {
					feed->description = nsp_parser_sanitize_string((char *) xmlNodeGetContent(tmp), TRUE);
				} else if ( !xmlStrcasecmp(tmp->name, (xmlChar *) "link") && tmp->ns == NULL && feed->site_url == NULL ) {
					feed->site_url = (char *) xmlNodeGetContent(tmp);
				}
				
				tmp = tmp->next;
			}
			
			break; // parse only one channel per feed
		}
		tmp = tmp->next;
	}
	return 0;
}


int
nsp_feed_parse (xmlDoc *xml, NspFeed *feed)
{
	xmlNode *node = NULL;
	
	assert(xml != NULL);
	node = xmlDocGetRootElement(xml);

	feed->type = nsp_parse_feed_type(node);
	
	switch( feed->type ) {
		case NSP_FEED_RSS_2_0:
		case NSP_FEED_RSS_1_0:
		case NSP_FEED_RSS_0_9:
			if ( feed->title == NULL || feed->description == NULL) {
				nsp_parse_feed_rss(node, feed);
			}
			feed->items = nsp_parse_items_rss(node, NULL);
			break;
		default:
			break;
	}

	return 0;
}

static char*
nsp_parser_sanitize_string(char *title, gboolean valid_html) {
	GRegex *regex;
	char *result;
	char *tmp;
	
	regex = g_regex_new ("(\\R| +)", G_REGEX_MULTILINE, 0, NULL);
	
	tmp = g_regex_replace(regex, title, -1, 0, " ", 0, NULL);
	g_regex_unref(regex);
	
	if ( valid_html ) {
		regex = g_regex_new ("&(?!amp;)", G_REGEX_MULTILINE, 0, NULL);
	
		result = g_regex_replace(regex, tmp, -1, 0, "&amp;", 0, NULL);
		g_regex_unref(regex);
		g_free(tmp);
	} else {
		result = tmp;
	}
	
	return result;
}



