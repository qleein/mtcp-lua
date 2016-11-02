#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <time.h>

#include "rbtree.h"


static inline void
rbtree_left_rotate(struct rbtree_node **root, struct rbtree_node *sentinel, struct rbtree_node *node)
{
	struct rbtree_node		*temp;

	temp = node->right;
	node->right = temp->left;

	if (temp->left != sentinel) {
		temp->left->parent = node;
	}

	temp->parent = node->parent;
	if (node == *root) {
		*root = temp;
	} else if (node == node->parent->left) {
		node->parent->left = temp;
	} else {
		node->parent->right = temp;
	}
	temp->left = node;
	node->parent = temp;
	return;
}


static inline void
rbtree_right_rotate(struct rbtree_node **root, struct rbtree_node *sentinel, struct rbtree_node *node)
{
	struct rbtree_node *temp;

	temp = node->left;
	node->left = temp->right;

	if (temp->right != sentinel) {
		temp->right->parent = node;
	}

	temp->parent = node->parent;
	if (node == *root) {
		*root = temp;
	} else if (node == node->parent->left) {
		node->parent->left = temp;
	} else {
		node->parent->right = temp;
	}
	temp->right = node;
	node->parent = temp;
	return;
}


void rbtree_insert(struct rbtree *tree, struct rbtree_node *node)
{
	struct rbtree_node		**root, *temp, *sentinel;

	/* a binary tree insert */
	root = (struct rbtree_node **)&tree->root;
	sentinel = tree->sentinel;

	if (*root == sentinel) {
		node->parent = NULL;
		node->left = sentinel;
		node->right = sentinel;
		rbtree_black(node);
		*root = node;
		return;
	}

	tree->insert(*root, node, sentinel);

	/* re-balanece tree */
	while (node != *root && rbtree_is_red(node->parent)) {
		if (node->parent == node->parent->parent->left) {
			temp = node->parent->parent->right;
		
			if (rbtree_is_red(temp)) {
	 			rbtree_black(node->parent);
				rbtree_black(temp);
				rbtree_red(node->parent->parent);
				node = node->parent->parent;

			} else {
	 			if (node == node->parent->right) {
					node = node->parent;
					rbtree_left_rotate(root, sentinel, node);
				}

				rbtree_black(node->parent);
				rbtree_red(node->parent->parent);
				rbtree_right_rotate(root, sentinel, node->parent->parent);
			}

		} else {
			temp = node->parent->parent->left;

			if (rbtree_is_red(temp)) {
				rbtree_black(node->parent);
				rbtree_black(temp);
				rbtree_red(node->parent->parent);
				node = node->parent->parent;

			} else {
				if (node == node->parent->left) {
					node = node->parent;
					rbtree_right_rotate(root, sentinel, node);
				}

				rbtree_black(node->parent);
				rbtree_red(node->parent->parent);
				rbtree_left_rotate(root, sentinel, node->parent->parent);
			}
		}
	}

	rbtree_black(*root);
}



/*  Nginx's red-black tree delete implementation' */
void
rbtree_delete(struct rbtree *tree, struct rbtree_node *node)
{
	int red;
	struct rbtree_node		**root, *sentinel, *subst, *temp, *w;

	/*  a binary tree delete */

	root = (struct rbtree_node **)&tree->root;
	sentinel = tree->sentinel;

	if (node->left == sentinel) {
		temp = node->right;
		subst = node;

	} else if (node->right == sentinel) {
		temp = node->left;
		subst = node;

	} else {
		subst = rbtree_min(node->right, sentinel);

		if (subst->left != sentinel) {
			temp = subst->left;

		} else {
			temp = subst->right;
		}
	}

	if (subst == *root) {
		*root = temp;
		rbtree_black(temp);

		node->left = NULL;
		node->right = NULL;
		node->parent = NULL;
		node->key = 0;

		return;
	}

	red = rbtree_is_red(subst);

	if (subst == subst->parent->left) {
		subst->parent->left = temp;
		
	} else {
		subst->parent->right = temp;
	}

	if (subst == node) {
		temp->parent = subst->parent;
	
	} else {

		if (subst->parent == node) {
			temp->parent = subst;

		} else {
			temp->parent = subst->parent;
		}

		subst->left = node->left;
		subst->right = node->right;
		subst->parent = node->parent;
		rbtree_copy_color(subst, node);

		if (node == *root) {
			*root = subst;

		} else {
			if (node == node->parent->left) {
				node->parent->left = subst;
			} else {
				node->parent->right = subst;
			}
		}

		if (subst->left != sentinel) {
			subst->left->parent = subst;
		}

		if (subst->right != sentinel) {
			subst->right->parent = subst;
		}
	}

	node->left = NULL;
	node->right = NULL;
	node->parent = NULL;
	node->key = 0;

	if (red) {
		return;
	}

	/*  a delete fixup */
	while (temp != *root && rbtree_is_black(temp)) {

		if (temp == temp->parent->left) {
			w = temp->parent->right;

			if (rbtree_is_red(w)) {
				rbtree_black(w);
				rbtree_red(temp->parent);
				rbtree_left_rotate(root, sentinel, temp->parent);
				
				w = temp->parent->right;
			}

			if (rbtree_is_black(w->left) && rbtree_is_black(w->right)) {
				rbtree_red(w);
				temp = temp->parent;

			} else {
				if (rbtree_is_black(w->right)) {
					rbtree_black(w->left);
					rbtree_red(w);
					rbtree_right_rotate(root, sentinel, w);
					w = temp->parent->right;
				}

				rbtree_copy_color(w, temp->parent);
				rbtree_black(temp->parent);
				rbtree_black(w->right);
				rbtree_left_rotate(root, sentinel, temp->parent);

				temp = *root;
			}

		} else {
			w = temp->parent->left;

			if (rbtree_is_red(w)) {
				rbtree_black(w);
				rbtree_red(temp->parent);
				rbtree_right_rotate(root, sentinel, temp->parent);

				w = temp->parent->left;
			}

			if (rbtree_is_black(w->left) && rbtree_is_black(w->right)) {
				rbtree_red(w);
				temp = temp->parent;

			} else {
				if (rbtree_is_black(w->left)) {
					rbtree_black(w->right);
					rbtree_red(w);
					rbtree_left_rotate(root, sentinel, w);
					w = temp->parent->left;
				}

				rbtree_copy_color(w, temp->parent);
				rbtree_black(temp->parent);
				rbtree_black(w->left);
				rbtree_right_rotate(root, sentinel, temp->parent);

				temp = *root;
			}
		}
	}

	rbtree_black(temp);
}


/*
 * find and insert_pt 需使用时定义。
 */
static struct rbtree_node *
rbtree_find(struct rbtree *tree, rbtree_key key)
{
    struct rbtree_node     *node;
    node = tree->root;
    
    while (node) {
        if (key < node->key) {
            node = node->left;

        } else if (key > node->key) {
            node = node->right;

        } else {
            return node;
        }
    }
    return NULL;
}


void
rbtree_insert_value(struct rbtree_node *temp, struct rbtree_node *node, struct rbtree_node *sentinel)
{
	struct rbtree_node	**p;

	for (; ;) {

		p = (node->key < temp->key) ? &temp->left : &temp->right;

		if (*p == sentinel) {
			break;
		}

		temp = *p;
	}

	*p = node;
	node->parent = temp;
	node->left = sentinel;
	node->right = sentinel;
	rbtree_red(node);
}

void
rbtree_insert_timer_value(struct rbtree_node *temp, struct rbtree_node *node,
    struct rbtree_node *sentinel)
{
    struct rbtree_node **p;

    for (;;) {
        p = ((rbtree_key_int)(node->key - temp->key) < 0)
             ? &temp->left : &temp->right;
        if (*p == sentinel)
            break;

        temp = *p;
    }

    *p = node;
    node->parent = temp;
    node->left = sentinel;
    node->right = sentinel;
    rbtree_red(node);
}

/*
 * Test if valid red-black tree
 */
static void 
rbtree_traverse(struct rbtree_node *node, struct rbtree_node *sentinel)
{
    if (node == sentinel) {
        return;
    }

    if (node->left != sentinel) {
        if (node->key <= node->left->key) {
            fprintf(stderr, "Something wrong.\n");
            fprintf(stderr, "left key:%ld, node key:%ld.\n", node->left->key, node->key);
            exit(1);
        }
        rbtree_traverse(node->left, sentinel);
    }
    fprintf(stderr, "%ld\t", node->key);

    if (node->right != sentinel) {
        if (node->key >= node->right->key) {
            fprintf(stderr, "Something wrong.\n");
            fprintf(stderr, "right key:%ld, node key:%ld.\n", node->right->key, node->key);
            exit(1);
        }
        rbtree_traverse(node->right, sentinel);
    }

    return;
}



/*  
 * a simple test.
 *  */
/*  
#define TEST_NUM 10000

int main(int argc, char *argv[])
{
    rbtree_key number[TEST_NUM];
    fprintf(stdout, "sizeof long: %lu.\n", sizeof(long));

    int i;
    srand(time(NULL));
    for (i = 0; i < TEST_NUM; i++) {
        number[i] = random();
    }
    
    struct rbtree      tree;
    struct rbtree_node *sentinel;
    memset(&tree, 0, sizeof(tree));

    sentinel = malloc(sizeof(struct rbtree_node));
    if (sentinel == NULL) {
        fprintf(stderr, "malloc failed.\n");
        exit(1);
    }

    rbtree_init(&tree, sentinel, rbtree_insert_value);
    rbtree_sentinel_init(sentinel);

    for (i = 0; i < TEST_NUM; i++) {
        struct rbtree_node *node;
        node = rbtree_find(&tree, number[i]);
        if (node) {
            number[i] = random();
            i--;
            continue;
        }
        node = malloc(sizeof(struct rbtree_node));
        if (node == NULL) {
            fprintf(stderr, "malloc failed.\n");
            exit(1);
        }
        node->key = number[i];
        rbtree_insert(&tree, node);
    }

    rbtree_traverse(tree.root, tree.sentinel);
    return 0;
}
*/
