/*
 * --- avl3b.c (version 3.0) 
 * --- started oct 21 1995 
 * --- revised oct 24 1995
 * --- revised jul 6 1996 
 * --- revised jan 27 1997
 * --- revised feb 17 1997
 * --- author : dibyendu majumdar
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include "avl3.h"
#include "avl3int.h"

/* set right child */
void 
AVL_SetRight(AVLNode * self, AVLNode * r)
{
	self->right = r;
	if (r)
		r->parent = self;
}

/* set left child */
void 
AVL_SetLeft(AVLNode * self, AVLNode * l)
{
	self->left = l;
	if (l)
		l->parent = self;
}

/* rotate sub-tree left, return new root (do not change balance) */
AVLNode        *
AVL_RotateLeft(AVLNode * self)
{
#ifndef __CXC__
	AVLNode        *newroot = self->right;
#else
	AVLNode        *newroot;
	newroot = self->right;
#endif
	if (self->parent != 0) {
		if (self == self->parent->left)
			AVL_SetLeft(self->parent, newroot);
		else
			AVL_SetRight(self->parent, newroot);
	} else
		newroot->parent = 0;
	AVL_SetRight(self, newroot->left);
	AVL_SetLeft(newroot, self);
	return newroot;
}

/* rotate sub-tree right, return new root (do not change balance) */
AVLNode        *
AVL_RotateRight(AVLNode * self)
{
#ifndef __CXC__
	AVLNode        *newroot = self->left;
#else
	AVLNode        *newroot;
	newroot = self->left;
#endif
	if (self->parent != 0) {
		if (self == self->parent->left)
			AVL_SetLeft(self->parent, newroot);
		else
			AVL_SetRight(self->parent, newroot);
	} else
		newroot->parent = 0;
	AVL_SetLeft(self, newroot->right);
	AVL_SetRight(newroot, self);
	return newroot;
}

/* doubly rotate sub-tree and return new root (change balance as needed) */
AVLNode        *
AVL_DoubleRotateRight(AVLNode * self)
{
#ifndef __CXC__
	AVLNode        *lf = self->left;
	AVLNode        *rt = lf->right;
	AVLNode        *root;
#else
	AVLNode        *lf;
	AVLNode        *rt;
	AVLNode        *root;

	lf = self->left;
	rt = lf->right;
#endif

	switch (rt->balance) {
	case RightHigh:
		self->balance = Equal;
		lf->balance = LeftHigh;
		break;

	case Equal:
		self->balance = lf->balance = Equal;
		break;

	case LeftHigh:
		self->balance = RightHigh;
		lf->balance = Equal;
		break;
	}
	rt->balance = Equal;
	AVL_SetLeft(self, AVL_RotateLeft(lf));
	root = AVL_RotateRight(self);
	return root;
}

/* doubly rotate sub-tree and return new root (change balance as needed) */
AVLNode        *
AVL_DoubleRotateLeft(AVLNode * self)
{
#ifndef __CXC__
	AVLNode        *rt = self->right;
	AVLNode        *lf = rt->left;
	AVLNode        *root;
#else
	AVLNode        *rt;
	AVLNode        *lf;
	AVLNode        *root;

	rt = self->right;
	lf = rt->left;
#endif

	switch (lf->balance) {
	case RightHigh:
		self->balance = LeftHigh;
		rt->balance = Equal;
		break;

	case Equal:
		self->balance = rt->balance = Equal;
		break;

	case LeftHigh:
		self->balance = Equal;
		rt->balance = RightHigh;
		break;
	}
	lf->balance = Equal;
	AVL_SetRight(self, AVL_RotateRight(rt));
	root = AVL_RotateLeft(self);
	return root;
}

/*
 * Rebalance sub-tree whose left branch has become heavier because of
 * insertion 
 */
AVLNode        *
AVL_RebalanceHeavierLeft(AVLNode * self, int *height_changed)
{
#ifndef __CXC__
	AVLNode        *root = self;
#else
	AVLNode        *root;

	root = self;
#endif

	/* left-subtree has grown by one level */
	switch (root->balance) {
	case RightHigh:
		root->balance = Equal;
		*height_changed = 0;
		break;

	case Equal:
		root->balance = LeftHigh;
		break;

	case LeftHigh:
		{
#ifndef __CXC__
			AVLNode        *lf = self->left;
#else
			AVLNode        *lf;
			lf = self->left;
#endif
			switch (lf->balance) {
			case LeftHigh:
				self->balance = Equal;
				lf->balance = Equal;
				root = AVL_RotateRight(self);
				break;

			case RightHigh:
				root = AVL_DoubleRotateRight(self);
				break;
			}
			*height_changed = 0;
			break;
		}
	}
	return root;
}

/*
 * Rebalance sub-tree whose right branch has become heavier because of
 * insertion 
 */
AVLNode        *
AVL_RebalanceHeavierRight(AVLNode * self, int *height_changed)
{
#ifndef __CXC__
	AVLNode        *root = self;
#else
	AVLNode        *root;
	root = self;
#endif
	/* right-subtree has grown by one level */
	switch (root->balance) {
	case LeftHigh:
		root->balance = Equal;
		*height_changed = 0;
		break;

	case Equal:
		root->balance = RightHigh;
		break;

	case RightHigh:
		{
#ifndef __CXC__
			AVLNode        *rt = self->right;
#else
			AVLNode        *rt;
			rt = self->right;
#endif
			switch (rt->balance) {
			case RightHigh:
				self->balance = Equal;
				rt->balance = Equal;
				root = AVL_RotateLeft(self);
				break;

			case LeftHigh:
				root = AVL_DoubleRotateLeft(self);
				break;
			}
			*height_changed = 0;
			break;
		}
	}
	return root;
}

/* Rebalance sub-tree whose left branch has become short because of deletion */
AVLNode        *
AVL_RebalanceShorterLeft(AVLNode * self, int *height_changed)
{
#ifndef __CXC__
	AVLNode        *p = self;
#else
	AVLNode        *p;
	p = self;
#endif
	switch (p->balance) {
	case Equal:
		p->balance = RightHigh;
		*height_changed = 0;
		break;

	case LeftHigh:
		/* taller (left) subtree was shortened */
		p->balance = Equal;
		break;

	case RightHigh:
		{
			/*
			 * shorter (left) subtree was shortened, violating
			 * AVL rules 
			 */
			AVLNode        *q;
			q = p->right;

			switch (q->balance) {
			case Equal:
				*height_changed = 0;
				q->balance = LeftHigh;
				p = AVL_RotateLeft(p);
				break;

			case RightHigh:
				p->balance = q->balance = Equal;
				p = AVL_RotateLeft(p);
				break;

			case LeftHigh:
				p = AVL_DoubleRotateLeft(p);
				break;
			}
			break;
		}
	}
	return p;
}

/*
 * rebalance a sub-tree whose right branch has been reduced by one level
 * because of deletion 
 */
AVLNode        *
AVL_RebalanceShorterRight(AVLNode * self, int *height_changed)
{
#ifndef __CXC__
	AVLNode        *p = self;
#else
	AVLNode        *p;
	p = self;
#endif
	switch (p->balance) {
	case Equal:
		p->balance = LeftHigh;
		*height_changed = 0;
		break;

	case RightHigh:
		/* taller (right) subtree was shortened */
		p->balance = Equal;
		break;

	case LeftHigh:
		{
			/*
			 * shorter (right) sub-tree was shortened, violating
			 * AVL rules 
			 */
			AVLNode        *q;
			q = p->left;

			switch (q->balance) {
			case Equal:
				*height_changed = 0;
				q->balance = RightHigh;
				p = AVL_RotateRight(p);
				break;

			case LeftHigh:
				p->balance = q->balance = Equal;
				p = AVL_RotateRight(p);
				break;

			case RightHigh:
				p = AVL_DoubleRotateRight(p);
				break;
			}
			break;
		}
	}
	return p;
}

int 
AVL_Height(AVLNode * self)
{
#ifndef __CXC__
	int             height = 0;
	int             lf_height = 0, rt_height = 0;
#else
	int             height;
	int             lf_height, rt_height;

	height = 0;
	lf_height = 0, rt_height = 0;
#endif

	if (self == 0)
		return 0;
	if (self->left)
		lf_height = AVL_Height(self->left);
	if (self->right)
		rt_height = AVL_Height(self->right);
	if (lf_height > rt_height)
		height += lf_height + 1;
	else
		height += rt_height + 1;
	return height;
}

/* find the first object in the tree */
void           *
AVLTree_FindFirst(AVLTree * tree)
{
#ifndef __CXC__
	void           *root = tree->root;
#else
	void           *root;
	root = tree->root;
#endif
	if (root) {
		register AVLNode *current = (AVLNode *) root;
		while (current->left != NULL)
			current = current->left;
		return (void *) (current + 1);
	}
	return NULL;
}

/* find the last object in the tree */
void           *
AVLTree_FindLast(AVLTree * tree)
{
#ifndef __CXC__
	void           *root = tree->root;
#else
	void           *root;
	root = tree->root;
#endif
	if (root) {
		register AVLNode *current = (AVLNode *) root;
		while (current->right != NULL)
			current = current->right;
		return (void *) (current + 1);;
	}
	return NULL;
}

/* find the next object in the tree */
void           *
AVLTree_FindNext(AVLTree * tree, void *currnode)
{
#ifndef __CXC__
	AVLNode        *current = (AVLNode *) currnode;
#else
	AVLNode        *current;
	current = (AVLNode *) currnode;
#endif
	if (current) {
		--current;
		if (current->right) {
			current = current->right;
			while (current->left != NULL)
				current = current->left;
		} else {
#ifndef __CXC__
			AVLNode        *p = current;
#else
			AVLNode        *p;
			p = current;
#endif
			current = p->parent;
			while (current) {
				if (current->right == p) {
					p = current;
					current = current->parent;
				} else
					break;
			}
		}
	}
	if (current)
		return (void *) (current + 1);
	else
		return NULL;
}

/* find the previous object in the tree */
void           *
AVLTree_FindPrevious(AVLTree * tree, void *currnode)
{
#ifndef __CXC__
	AVLNode        *current = (AVLNode *) currnode;
#else
	AVLNode        *current;
	current = (AVLNode *) currnode;
#endif
	if (current) {
		--current;
		if (current->left) {
			current = current->left;
			while (current->right != NULL)
				current = current->right;
		} else {
#ifndef __CXC__
			AVLNode        *p = current;
#else
			AVLNode        *p;
			p = current;
#endif
			current = p->parent;
			while (current) {
				if (current->left == p) {
					p = current;
					current = current->parent;
				} else
					break;
			}
		}
	}
	if (current)
		return (void *) (current + 1);
	else
		return NULL;
}

int 
AVLTree_Height(AVLTree * tree)
{
	return AVL_Height((AVLNode *) tree->root);
}

void
AVLTree_BackwardApply(AVLNode * root, void (*funcptr)(void *))
{
	if (root == NULL)
		return;
	AVLTree_BackwardApply(root->right, funcptr);
	(*funcptr)((void *)(root+1));
	AVLTree_BackwardApply(root->left, funcptr);
}
	
void
AVLTree_Backeach(AVLTree * tree, void (*funcptr)(void *))
{
	AVLTree_BackwardApply((AVLNode *)tree->root, funcptr);
}

void
AVLTree_ForwardApply(AVLNode * root, void (*funcptr)(void *))
{
	if (root == NULL)
		return;
	AVLTree_ForwardApply(root->left, funcptr);
	(*funcptr)((void *)(root+1));
	AVLTree_ForwardApply(root->right, funcptr);
}
	
void
AVLTree_Foreach(AVLTree * tree, void (*funcptr)(void *))
{
	AVLTree_ForwardApply((AVLNode *)tree->root, funcptr);
}
