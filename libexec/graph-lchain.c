/*
 *	graph-lchain.c - Longest-chain tree nodes
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
	bool mark;
};

/* Variables */
static struct HXbtree *nodename_map;

/* Functions */
static void lchain_process(FILE *);
static bool lchain_contains(const struct node *, const struct node *);
static void lchain_link(const char *, const char *);
static void lchain_relink(struct node *, struct node *);
static void lchain_dump_tree(const struct HXbtree *);
static void lchain_free_tree(struct HXbtree *);

//-----------------------------------------------------------------------------
int main(int argc, const char **argv)
{
	const char **file;
	FILE *fp;

	setvbuf(stdout, NULL, _IOLBF, 0);

	nodename_map = HXbtree_init(HXBT_MAP | HXBT_SCMP);
	if (nodename_map == NULL) {
		perror("HXbtree_init");
		abort();
	}

	if (argc == 1) {
		lchain_process(stdin);
	} else {
		for (file = &argv[1]; *file != NULL; ++file) {
			fp = fopen(*file, "r");
			if (fp == NULL) {
				fprintf(stderr, "%s: Could not open %s: %s\n",
				        *argv, *file, strerror(errno));
				continue;
			}
			lchain_process(fp);
			fclose(fp);
		}
	}

	lchain_dump_tree(nodename_map);
	lchain_free_tree(nodename_map);
	return EXIT_SUCCESS;
}

static void lchain_process(FILE *fp)
{
	hmc_t *source_name = NULL, *target_name = NULL;

	while (HX_getl(&source_name, fp) != NULL) {
		if (HX_getl(&target_name, fp) == NULL)
			break;

		HX_chomp(source_name);
		HX_chomp(target_name);
		lchain_link(source_name, target_name);
	}

	hmc_free(source_name);
	hmc_free(target_name);
}

static inline bool lchain_contains(const struct node *start,
    const struct node *needle)
{
	const struct node *w;

	for (w = start; w != NULL; w = w->parent)
		if (w == needle)
			return true;
	return false;
}

static void lchain_link(const char *source_name, const char *target_name)
{
	struct node *source_ptr, *target_ptr;
	struct HXbtree_node *tree_node;

	target_ptr = HXbtree_get(nodename_map, target_name);
	if (target_ptr == NULL) {
		struct node local_node = {.name = HX_strdup(target_name)};

		if (local_node.name == NULL) {
			perror("strdup");
			abort();
		}

		tree_node = HXbtree_add(nodename_map, local_node.name,
		            HX_memdup(&local_node, sizeof(local_node)));
		if (tree_node == NULL) {
			perror("HXbtree_add");
			abort();
		}
		target_ptr = tree_node->data;
	}

	source_ptr = HXbtree_get(nodename_map, source_name);
	if (source_ptr == NULL) {
		struct node local_node = {
			.name   = HX_strdup(source_name),
			.parent = target_ptr,
		};

		if (local_node.name == NULL) {
			perror("strdup");
			abort();
		}

		tree_node = HXbtree_add(nodename_map, local_node.name,
		            HX_memdup(&local_node, sizeof(local_node)));
		if (tree_node == NULL) {
			perror("HXbtree_add");
			abort();
		}
		source_ptr = tree_node->data;

		/* New parent */
		return;
	}

	/*
	 * If there is no edge <a1, ANY> in the (directed) tree, add edge to
	 * tree, restart loop with next tuple ("CONTINUE"/RETURN).
	 */
	if (source_ptr->parent == NULL) {
		if (lchain_contains(target_ptr, source_ptr))
			return;
		/* New parent */
		source_ptr->parent = target_ptr;
		return;
	}

	/* [ Now we know there must be a <a1, ANY> edge in the tree. ] */

	/* If the edge <a1, a2> is already in the tree, CONTINUE. */

	if (strcmp(source_ptr->parent->name, target_name) == 0)
		/* Parent did not change */
		return;

	/* [ Now there is <a1, a3> in the tree, with a2 != a3 ] */

	lchain_relink(source_ptr, target_ptr);
}

static void lchain_relink(struct node *source, struct node *new_parent)
{
	unsigned int old_path_length, new_path_length;
	struct node *common_ancestor, *w;

	/*
	 * Check whether adding <a1, a2> to the tree would generate a
	 * circle/loop a1-->a2-->...-->a1. If so, CONTINUE.
	 *
	 * This ensures the tree remains a tree. Once you have loops
	 * the graph would quickly get cramped.
	 */

	/* Make sure it will be loop-free */
	if (lchain_contains(new_parent, source))
		return;

	/*
	 * [ Now we know that <a1, a2> is a potential candidate to replace
	 * <a1, a3> and that the tree is indeed loop-free. ]
	 */

	/*
	 * Find longest path towards common ancestor.
	 */

	/* Walk down old path and mark */
	for (w = source->parent; w != NULL; w = w->parent) {
		if (w->mark) {
			fprintf(stderr, "Impossible loop detected at %s\n",
			        w->name);
			abort();
		}
		w->mark = true;
	}

	/* Walk down new path and check for mark=1 */
	new_path_length = 0;
	for (w = new_parent; w != NULL; w = w->parent) {
		++new_path_length;
		if (w->mark)
			break;
	}

	/*
	 * Let c be a common ancestor of an existing path, e.g.
	 * a1-->a3-->...-->c, and the "proposed" new path
	 * a1-->a2-->...-->c. If no such common ancestor exists, CONTINUE.
	 */

	common_ancestor = w;
	if (common_ancestor == NULL) {
		/* No common ancestor. Clear flags. Bail out. */
		for (w = source->parent; w != NULL; w = w->parent)
			w->mark = false;
		return;
	}

	/* Walk down old path again and reset mark for further traversals. */
	old_path_length = 0;
	for (w = source; w != common_ancestor; w = w->parent) {
		++old_path_length;
		w->mark = false;
	}
	for (; w != NULL; w = w->parent)
		w->mark = false;

	/*
	 * If the path a1-->a2-->...->c is longer (has more edges) than
	 * a1-->a3->...-->c, delete <a1, a3> from the tree and add
	 * <a1, a2> instead.
	 *
	 * This I call longest-chaining.
	 *
	 * One can see that this results in a tree with only ever one
	 * tuple to match <a1, ANY>.
	 * This ensures that no two edges ever overlap which "helps"
	 * decramping the graph.
	 */

	source->parent = new_parent;
}

#if 0
static void lchain_loopcheck(const struct HXbtree *tree)
{
	const struct HXbtree_node *tree_node;
	const struct node *source;
	struct node *w;
	void *trav;

	trav = HXbtrav_init(tree);
	while ((tree_node = HXbtraverse(trav)) != NULL) {
		source = tree_node->data;
		for (w = source->parent; w != NULL; w = w->parent) {
			if (w->mark)
				fprintf(stderr, "Loop detected at %s\n",
				        w->name);
			w->mark = true;
		}
		for (w = source->parent; w != NULL; w = w->parent)
			w->mark = false;
	}

	HXbtrav_free(trav);
}
#endif

static void lchain_dump_tree(const struct HXbtree *tree)
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

static void lchain_free_tree(struct HXbtree *tree)
{
	const struct HXbtree_node *tree_node;
	struct node *source;
	void *trav;

	trav = HXbtrav_init(tree);
	while ((tree_node = HXbtraverse(trav)) != NULL) {
		source = tree_node->data;
		free(source->name);
		free(source);
	}

	HXbtrav_free(trav);
	HXbtree_free(tree);
}
