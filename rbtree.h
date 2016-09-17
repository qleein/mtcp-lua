/*************************************************************************
	> File Name: rbtree.h
	> Author: qlee
	> Mail: qlee001@163.com 
	> Created Time: 2016年09月17日 星期六 19时40分06秒
 ************************************************************************/
#ifndef __RBTREE_H__
#define __RBTREE_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <time.h>

typedef unsigned long           rbtree_key;
typedef long                    rbtree_key_int;
typedef struct rbtree_node      rbtree_node_t;
typedef struct rbtree           rbtree_t;

struct rbtree_node {
	rbtree_key			        key;
	struct rbtree_node		    *left;
	struct rbtree_node		    *right;
	struct rbtree_node		    *parent;
	u_char					    color;
	u_char					    data;
};


typedef void (*rbtree_insert_pt)(struct rbtree_node *root, struct rbtree_node *node, struct rbtree_node *sentinel);

struct rbtree {
	struct rbtree_node		    *root;
	struct rbtree_node		    *sentinel;
	rbtree_insert_pt		    insert;
};


#define rbtree_init(tree, s, i) \
	{ rbtree_sentinel_init(s);	\
	  (tree)->root = s;		    \
	  (tree)->sentinel = s;	    \
	  (tree)->insert = i;		\
	}

#define rbtree_red(node)				((node)->color = 1)
#define rbtree_black(node)				((node)->color = 0)
#define rbtree_is_red(node)				((node)->color)
#define rbtree_is_black(node)			(!rbtree_is_red(node))
#define rbtree_copy_color(n1, n2)		(n1->color = n2->color)

#define rbtree_sentinel_init(node)		rbtree_black(node)


static inline struct rbtree_node *
rbtree_min(struct rbtree_node *node, struct rbtree_node *sentinel)
{
	while(node->left != sentinel) {
		node = node->left;
	}
	return node;
}

void rbtree_insert(struct rbtree *tree, struct rbtree_node *node);

void rbtree_delete(struct rbtree *tree, struct rbtree_node *node);
void rbtree_insert_value(struct rbtree_node *temp, struct rbtree_node *node,
    struct rbtree_node *sentinel);
void rbtree_insert_timer_value(struct rbtree_node *temp, struct rbtree_node *node,
    struct rbtree_node *sentinel);



struct rbtree;
struct rbtree_node;



#endif
