/*
 *	graph-fanout.c - Fanout tree for Graphviz
 *	Copyright © Jan Engelhardt <jengelh [at] medozas de>, 2007 - 2010
 *
 *	This program is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation; either version 2 or 3 of the license.
 */

/*
 * Assume you have a group of nodes X_1 ... X_n that all point to a next-node P
 * (it is a directed graph). If n is within the threshold ("fan_height" in the
 * source code), do nothing with this group.
 *
 * Insert intermediate parent nodes for X_1 ... X_n so that P and every
 * intermediate only has at most n nodes pointing to it.
 *
 * (Just think of it like the n-level pagetables.)
 *
 * This is basically just a hack for graphviz because it tends to overlap edges
 * when it has to give them greater curvature. (Make yourself a tree with 32
 * nodes X_1...X_32 and one root node to which all 32 are attached.) The edges
 * X_1-->P and X_32-->P will have a great 'curvature', while X_16-->P is pretty
 * linear.
 *
 * The 'problem' (it's rather "we did not want to write code until the dawn of
 * time just to avoid this problem") lies in graphviz's math parts to form the
 * edge curvature -- so it happens that X_1-->P will overlap with X_4-->P. This
 * is not so nice when you really want to follow all edges from your name to
 * the root.
 */
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libHX/deque.h>
#include <libHX/init.h>
#include <libHX/map.h>
#include <libHX/option.h>
#include <libHX/string.h>

/* Definitions */
struct node {
	char *name;
	struct node *parent;
	struct HXdeque *children;
};

/* Variables */
static unsigned int fan_height = 8;
static unsigned int rt_node_counter;
static struct HXmap *nodename_map;
static struct HXmap *roots_map;

static void fanout_build(const char *child_name, const char *parent_name)
{
	struct node *child_ptr, *parent_ptr;
	int ret;

	parent_ptr = HXmap_get(nodename_map, parent_name);
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

		parent_ptr = HX_memdup(&local_node, sizeof(local_node));
		if (parent_ptr == NULL) {
			perror("HX_memdup");
			abort();
		}

		ret = HXmap_add(nodename_map, local_node.name, parent_ptr);
		if (ret <= 0) {
			fprintf(stderr, "HXmap_add: %s\n", strerror(-ret));
			abort();
		}
	}

	child_ptr = HXmap_get(nodename_map, child_name);
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

		child_ptr = HX_memdup(&local_node, sizeof(local_node));
		if (child_ptr == NULL) {
			perror("HX_memdup");
			abort();
		}
		ret = HXmap_add(nodename_map, local_node.name, child_ptr);
		if (ret <= 0) {
			fprintf(stderr, "HXmap_add: %s\n", strerror(-ret));
			abort();
		}
	}

	HXdeque_push(parent_ptr->children, child_ptr);
	child_ptr->parent = parent_ptr;
}

static void fanout_process(FILE *fp)
{
	hxmc_t *source_name = NULL, *target_name = NULL;

	while (HX_getl(&source_name, fp) != NULL) {
		if (HX_getl(&target_name, fp) == NULL)
			break;

		HX_chomp(source_name);
		HX_chomp(target_name);
		fanout_build(source_name, target_name);
	}

	HXmc_free(source_name);
	HXmc_free(target_name);
}

static void fanout_find_roots(void)
{
	const struct HXmap_node *tree_node;
	struct node *w;
	struct HXmap_trav *trav;

	trav = HXmap_travinit(nodename_map, 0);
	while ((tree_node = HXmap_traverse(trav)) != NULL) {
		w = tree_node->data;
		for (; w->parent != NULL; w = w->parent)
			;
		HXmap_add(roots_map, w, NULL);
	}

	HXmap_travfree(trav);
}

static void fanout_one_node(struct node *current)
{
	struct HXdeque *new_children;
	struct node *rt_node = NULL, *child;
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
			HXmap_add(nodename_map, rt_node->name, rt_node);
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

static void fanout_process_roots(struct HXmap *root_map)
{
	const struct HXmap_node *tree_node;
	struct HXmap_trav *trav;

	trav = HXmap_travinit(root_map, 0);
	while ((tree_node = HXmap_traverse(trav)) != NULL)
		fanout_one_node(tree_node->key);

	HXmap_travfree(trav);
}

static void fanout_dump_tree(const struct HXmap *tree)
{
	const struct HXmap_node *tree_node;
	const struct node *source;
	struct HXmap_trav *trav;

	trav = HXmap_travinit(tree, 0);
	while ((tree_node = HXmap_traverse(trav)) != NULL) {
		source = tree_node->data;
		if (source->parent != NULL)
			printf("%s\n%s\n",
			       source->name, source->parent->name);
	}

	HXmap_travfree(trav);
}

static void fanout_free_tree(struct HXmap *tree)
{
	const struct HXmap_node *tree_node;
	struct node *current;
	struct HXmap_trav *trav;

	trav = HXmap_travinit(tree, 0);
	while ((tree_node = HXmap_traverse(trav)) != NULL) {
		current = tree_node->data;
		HXdeque_free(current->children);
		free(current->name);
		free(current);
	}

	HXmap_travfree(trav);
	HXmap_free(tree);
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

static int main2(int argc, const char **argv)
{
	const char **file;
	FILE *fp;

	nodename_map = HXmap_init(HXMAPT_DEFAULT, HXMAP_SKEY);
	if (nodename_map == NULL) {
		perror("HXmap_init");
		abort();
	}

	roots_map = HXmap_init(HXMAPT_DEFAULT, HXMAP_SINGULAR);
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
	HXmap_free(roots_map);
	return EXIT_SUCCESS;
}

int main(int argc, const char **argv)
{
	int ret;

	if ((ret = HX_init()) <= 0) {
		fprintf(stderr, "HX_init: %s\n", strerror(-ret));
		abort();
	}
	ret = main2(argc, argv);
	HX_exit();
	return ret;
}
