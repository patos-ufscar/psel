#include "avl.h"

avlnode * avln_init(uint32_t key, void * data)
{
	avlnode *ret = (avlnode *) malloc(sizeof(avlnode));

	if (!ret)
	{
		return NULL;
	}

	ret->key 	= key;
	ret->data 	= data;

	ret->father 	= NULL;
	ret->right	= NULL;
	ret->left	= NULL;
	ret->height	= 0;
	ret->balance	= 0;

	return ret;
}

avl * avl_init()
{
	avl *ret = (avl *) malloc(sizeof(avl));
	if (!ret)
	{
		return NULL;
	}

	ret->root = NULL;
	ret->size = 0;

	return ret;
}

avlnode * avl_insert(avl *t, avlnode *tracker, avlnode *node) // Não permite repetição de chaves
{
	// Inserir nó
	if (tracker == NULL)
	{
		tracker = node;
		return node;
	}

	// Achar nó
	if (node->key > tracker->key)
	{
		return avl_insert(t, tracker->right, node);
	}

	else if (node->key < tracker->key)
	{
		return avl_insert(t, tracker->left, node);
	}

	else
	{
		return tracker;
	}

	// Subida	
	
	tracker->height  = max(get_h(tracker->right), get_h(tracker->left)) + 1;
	tracker->balance = get_b(tracker);

	uint32_t balance = tracker->balance;

	if (balance == 2) // left-right && right
	{
		if (tracker->left->balance == 1)
		{
			return rotate_right(t, tracker);
		}

		else if (tracker->left->balance == -1)
		{
			return rotate_left_right(t, tracker);
		}

	}
	
	else if (balance == -2) // right-left && left
	{
		if (tracker->right->balance == 1)
		{
			return rotate_right_left(t, tracker);
		}
		
		else if (tracker->right->balance == -1)
		{
			return rotate_left(t, tracker);
		}
	}

	
}


int32_t get_h(avlnode *a)
{
	if (a == NULL)
	{
		return -1;
	}

	return a->height;
}

int32_t get_b(avlnode *a)
{
	return get_h(a->left) - get_h(a->right);
}

avlnode * rotate_left(avl *t, avlnode *a) // Nó crítico = -2
{	
	avlnode *right 	= a->right;
	avlnode *father = a->father;

	// Ajustar o pai
	if (!father)
	{
		t->root = right;
	}

	else
	{
		father->right = right;
	}

	right->father = father;

	// Ajustar o nó corrente
	a->right	= right->left;
	a->father 	= right;
	right->left	= a;

	right->height = max(get_h(right->left), get_h(right->right)) + 1;
	a->height = max(get_h(a->left), get_h(a->right)) + 1;

	return right;	
}

avlnode * rotate_right(avl *t, avlnode *a) // Nó crítico = 2
{
	avlnode *left 	= a->left;
	avlnode *father = a->father;

	if (!father)
	{
		t->root = left;
	}	

	else
	{
		father->left 	= left;
	}
	left->father = father;

	// Ajustar nó corrente
	a->left 	= left->right;
	a->father	= left;
	left->right 	= a;

	left->height = max(get_h(left->left), get_h(left->right)) + 1;
	a->height = max(get_h(a->left), get_h(a->right)) + 1;

	return left;
}

avlnode * avl_remove(avl *t, avlnode *tracker, uint32_t key)
{
	// Busca
	if (tracker == NULL)
	{
		return NULL;
	}
	
	if (key > tracker->key)
	{
		tracker->right = avl_remove(t, tracker->right, key);
	}

	else if (key < tracker->key)
	{
		tracker->left  = avl_remove(t, tracker->left, key);
	}
	
	else
	{
		avlnode *ret;

		if (tracker->right == NULL)
		{
			ret = tracker->left;
			free(tracker);
			
			return ret;
		}

		if (tracker->left == NULL)
		{
			ret = tracker->right;
			free(tracker);

			return ret;
		}
	
		// Tem 2 filhos
		ret = sucessor(tracker->right);
		tracker->key = ret->key;

		tracker->right = avl_remove(t, ret, ret->key);
	}

	// Subida
	tracker->height  = max(get_h(tracker->left), get_h(tracker->right)) + 1; 
	tracker->balance = get_b(tracker);

	uint32_t balance = tracker->balance;

	if (balance == 2) // left-right && right
	{
		if (tracker->left->balance == 1)
		{
			return rotate_right(t, tracker);
		}

		else if (tracker->left->balance == -1)
		{
			return rotate_left_right(t, tracker);
		}

	}
	
	else if (balance == -2) // right-left && left
	{
		if (tracker->right->balance == 1)
		{
			return rotate_right_left(t, tracker);
		}
		
		else if (tracker->right->balance == -1)
		{
			return rotate_left(t, tracker);
		}
	}
	return tracker;
}

avlnode * avl_search(avl *t, uint32_t key)
{
	avlnode * tracker = t->root;
	while (tracker != NULL && tracker->key != key)
	{
		if (tracker->key > key)
		{
			tracker = tracker->left;
		}

		else 
		{
			tracker = tracker->right;
		}
	}

	return tracker;
}

avlnode * rotate_right_left(avl *t, avlnode *a) // Nó crítico = -2, direita = 1
{
	rotate_right(t, a->right);
	return rotate_left(t, a);
}

avlnode * rotate_left_right(avl *t, avlnode *a) // Nó criítico = 2, esquerda = -1
{
	rotate_left(t, a->left);
	return rotate_right(t, a);

}

avlnode * sucessor(avlnode *a)
{
	while (a->left != NULL)
	{
		a = a->left;
	}

	return a;
}

void see_tree(avlnode *node)
{
	if (!node)
	{
		return ;
	}

	see_tree(node->left);
	see_tree(node->right);
	
	printf("%d\n", node->key);
	return ;
}


int32_t max(int32_t a, int32_t b)
{
	return a > b? a : b;
}
