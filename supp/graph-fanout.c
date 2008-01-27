/*
 *	graph-fanout.c - Fanout tree for Graphviz
 *	Copyright © CC Computer Consultants GmbH, 2007 - 2008
 *	Jan Engelhardt <jengelh@computergmbh.de>
 *
 *	This program is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation; either version 2 or 3 of the license.
 */
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libHX.h>

/* Definitions */
struct node {
	char *name;
	struct node *parent;
	struct HXdeque *children;
};

/* Variables */
static unsigned int fan_height = 8;
static unsigned int rt_node_counter;
static struct HXbtree *nodename_map;
static struct HXbtree *roots_map;

/* Functions */
static bool fanout_get_options(int *, const char ***);
static void fanout_process(FILE *);
static void fanout_build(const char *, const char *);
static void fanout_find_roots(void);
static void fanout_process_roots(struct HXbtree *);
static void fanout_one_node(struct node *);
static void fanout_dump_tree(const struct HXbtree *);
static void fanout_free_tree(struct HXbtree *);

//-----------------------------------------------------------------------------
int main(int argc, const char **argv)
{
	const char **file;
	FILE *fp;

	nodename_map = HXbtree_init(HXBT_MAP | HXBT_SCMP);
	if (nodename_map == NULL) {
		perror("HXbtree_init");
		abort();
	}

	roots_map = HXbtree_init(HXBT_ICMP);
	if (roots_map == NULL) {
		perror("HXbtree_init");
		abort();
	}

	if (!fanout_get_options(&argc, &argv))
		return EXIT_FAILURE;

	if (argc == 1) {
		fanout_process(stdin);
	} else {
		for (file = &argv[1]; *file != NULL; ++file) {
			fp = fopen(*file, "r");
			if (fp == NULL) {
				fprintf(stderr, "%s: Could not open %s: %s\n",
				        *argv, *file, strerror(errno));
				continue;
			}
			fanout_process(fp);
			fclose(fp);
		}
	}

	fanout_find_roots();
	fanout_process_roots(roots_map);
	fanout_dump_tree(nodename_map);
	fanout_free_tree(nodename_map);
	HXbtree_free(roots_map);
	return EXIT_SUCCESS;
}

static bool fanout_get_options(int *argc, const char ***argv)
{
	struct HXoption options_table[] = {
		{.sh = 'n', .type = HXTYPE_UINT, .ptr = &fan_height,
		 .help = "Maximum height per group", .htyp = "INT"},
		HXOPT_AUTOHELP,
		HXOPT_TABLEEND,
	};
	return HX_getopt(options_table, argc, argv, HXOPT_USAGEONERR) > 0;
}

static void fanout_process(FILE *fp)
{
	hmc_t *source_name = NULL, *target_name = NULL;

	while (HX_getl(&source_name, fp) != NULL) {
		if (HX_getl(&target_name, fp) == NULL)
			break;

		HX_chomp(source_name);
		HX_chomp(target_name);
		fanout_build(source_name, target_name);
	}

	hmc_free(source_name);
	hmc_free(target_name);
}

static void fanout_build(const char *child_name, const char *parent_name)
{
	struct node *child_ptr, *parent_ptr;
	struct HXbtree_node *tree_node;

	parent_ptr = HXbtree_get(nodename_map, parent_name);
	if (parent_ptr == NULL) {
		struct node local_node = {.name = HX_strdup(parent_name)};

		if (local_node.name == NULL) {
			perror("strdup");
			abort();
		}

		local_node.children = HXdeque_init();
		if (local_node.children == NULL) {
			perror("HXdeque_init");
			abort();
		}

		tree_node = HXbtree_add(nodename_map, local_node.name,
		            HX_memdup(&local_node, sizeof(local_node)));
		if (tree_node == NULL) {
			perror("HXbtree_add");
			abort();
		}
		parent_ptr = tree_node->data;
	}

	child_ptr = HXbtree_get(nodename_map, child_name);
	if (child_ptr == NULL) {
		struct node local_node = {.name = HX_strdup(child_name)};

		if (local_node.name == NULL) {
			perror("strdup");
			abort();
		}

		local_node.children = HXdeque_init();
		if (local_node.children == NULL) {
			perror("HXdeque_init");
			abort();
		}

		tree_node = HXbtree_add(nodename_map, local_node.name,
		            HX_memdup(&local_node, sizeof(local_node)));
		if (tree_node == NULL) {
			perror("HXbtree_add");
			abort();
		}
		child_ptr = tree_node->data;
	}

	HXdeque_push(parent_ptr->children, child_ptr);
	child_ptr->parent = parent_ptr;
}

static void fanout_find_roots(void)
{
	const struct HXbtree_node *tree_node;
	struct node *w;
	void *trav;

	trav = HXbtrav_init(nodename_map);
	while ((tree_node = HXbtraverse(trav)) != NULL) {
		w = tree_node->data;
		for (; w->parent != NULL; w = w->parent)
			;
		HXbtree_add(roots_map, w);
	}

	HXbtrav_free(trav);
}

static void fanout_process_roots(struct HXbtree *root_map)
{
	const struct HXbtree_node *tree_node;
	struct node *current;
	void *trav;

	trav = HXbtrav_init(root_map);
	while ((tree_node = HXbtraverse(trav)) != NULL) {
		current = tree_node->data;
		fanout_one_node(current);
	}

	HXbtrav_free(trav);
}

static void fanout_one_node(struct node *current)
{
	struct HXdeque *new_children;
	struct node *rt_node, *child;
	struct HXdeque_node *k;
	unsigned int i = 0;
	char buf[32];

	if (current->children->items <= fan_height) {
		for (k = current->children->first; k != NULL; k = k->next)
			fanout_one_node(k->ptr);
		return;
	}

	new_children = HXdeque_init();
	if (new_children == NULL) {
		perror("HXdeque_init");
		abort();
	}

	for (k = current->children->first; k != NULL; k = k->next) {
		if (i++ % fan_height == 0) {
			rt_node = malloc(sizeof(struct node));
			if (rt_node == NULL) {
				perror("malloc");
				abort();
			}

			snprintf(buf, sizeof(buf), "#%u", rt_node_counter++);
			rt_node->name = HX_strdup(buf);
			if (rt_node->name == NULL) {
				perror("strdup");
				abort();
			}

			rt_node->parent   = current;
			rt_node->children = HXdeque_init();
			if (rt_node->children == NULL) {
				perror("HXdeque_init");
				abort();
			}
			HXdeque_push(new_children, rt_node);
			HXbtree_add(nodename_map, rt_node->name, rt_node);
		}

		child = k->ptr;
		HXdeque_push(rt_node->children, child);
		child->parent = rt_node;
		fanout_one_node(child);
	}

	HXdeque_free(current->children);
	current->children = new_children;

	if (current->children->items > fan_height)
		/* Still need to fan out into more levels */
		fanout_one_node(current);
}

static void fanout_dump_tree(const struct HXbtree *tree)
{
	const struct HXbtree_node *tree_node;
	const struct node *source;
	void *trav;

	trav = HXbtrav_init(tree);
	while ((tree_node = HXbtraverse(trav)) != NULL) {
		source = tree_node->data;
		if (source->parent != NULL)
			printf("%s\n%s\n",
			       source->name, source->parent->name);
	}

	HXbtrav_free(trav);
}

static void fanout_free_tree(struct HXbtree *tree)
{
	const struct HXbtree_node *tree_node;
	struct node *current;
	void *trav;

	trav = HXbtrav_init(tree);
	while ((tree_node = HXbtraverse(trav)) != NULL) {
		current = tree_node->data;
		HXdeque_free(current->children);
		free(current->name);
		free(current);
	}

	HXbtrav_free(trav);
	HXbtree_free(tree);
}
