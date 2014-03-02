#include "binheap.h"
#include "linklist.h"

typedef struct __binomial_tree_node_s {
    void *key;
    size_t klen;
    void *value;
    size_t vlen;
    struct __binomial_tree_node_s *parent;
    struct __binomial_tree_node_s **children;
    int num_children;
    binheap_t *bh;
} binomial_tree_node_t;

struct __binheap_s {
    linked_list_t *trees;
    binheap_cmp_keys_callback cmp_keys_cb;
    int count;
    int mode;
};

#define HAS_PRECEDENCE(__bh, __k1, __kl1, __k2, __kl2) \
((__bh->mode == BINHEAP_MODE_MAX  && __bh->cmp_keys_cb(__k1, __kl1, __k2, __kl2) >= 0) || \
 (__bh->mode == BINHEAP_MODE_MIN  && __bh->cmp_keys_cb(__k1, __kl1, __k2, __kl2) <= 0))

static int __cmp_keys_default(void *k1, size_t kl1, void *k2, size_t kl2)
{
    if (kl1 != kl2)
        return kl1 - kl2;
    return memcmp(k1, k2, kl1);
}

int
binomial_tree_node_add(binomial_tree_node_t *node,
                       binomial_tree_node_t *child)
{
    node->children = realloc(node->children, sizeof(binomial_tree_node_t *) * (node->num_children + 1));
    node->children[node->num_children++] = child;
    if (child->parent) {
        // TODO - remove the node
    }
    child->parent = node;
    return 0;
}

int
binomial_tree_node_find_min_child(binomial_tree_node_t *node)
{
    if (!node->num_children)
        return -1;

    int min_child_index = 0;
    int i;
    for (i = 0; i < node->num_children; i++) {
        binomial_tree_node_t *cur = node->children[i];
        binomial_tree_node_t *min = node->children[min_child_index];
        if (node->bh->cmp_keys_cb(cur->key, cur->klen, min->key, min->klen) <= 0)
            min_child_index = i;
    }
    return min_child_index;
}

int
binomial_tree_node_find_max_child(binomial_tree_node_t *node)
{
    if (!node->num_children)
        return -1;

    int max_child_index = 0;
    int i;
    for (i = 0; i < node->num_children; i++) {
        binomial_tree_node_t *cur = node->children[i];
        binomial_tree_node_t *max = node->children[max_child_index];
        if (node->bh->cmp_keys_cb(cur->key, cur->klen, max->key, max->klen) >= 0)
            max_child_index = i;
    }
    return max_child_index;
}

void
binomial_tree_node_destroy(binomial_tree_node_t *node)
{
    int i;
    binomial_tree_node_t *new_parent = NULL;

    if (node->parent) {
        new_parent = node->parent;
        int node_index = -1;
        for (i = 0; i < new_parent->num_children; i++) {
            if (new_parent->children[i] == node) {
                node_index = i;
                break;
            }
        }
        if (new_parent->num_children && node_index >= 0) {
            int to_copy = new_parent->num_children - (node_index + 1);
            if (to_copy) {
                memcpy(&new_parent->children[node_index],
                       &new_parent->children[node_index+1],
                       sizeof(binomial_tree_node_t *) * to_copy);
            } else {
                // TODO - Error messages
                // (something is badly corrupted if we are here)
            }
            new_parent->num_children--;
        }
    } else if (node->num_children) {
        int child_index = node->bh->mode == BINHEAP_MODE_MAX
                        ? binomial_tree_node_find_max_child(node)
                        : binomial_tree_node_find_min_child(node);

        if (child_index >= 0) {
            new_parent = node->children[child_index];
            if (child_index < node->num_children - 1) {
                memcpy(&node->children[child_index],
                       &node->children[child_index + 1],
                       sizeof(binomial_tree_node_t *) * (node->num_children - child_index + 1));
                       
            }
            node->num_children--;
        }
        new_parent->parent = NULL;
    }

    for (i = 0; i < node->num_children; i++) {
        if (new_parent)
            binomial_tree_node_add(new_parent, node->children[i]);
        else
            node->children[i]->parent = NULL;
    }

    free(node->key);
    free(node);
    node->bh->count--;
}

binheap_t *
binheap_create(binheap_cmp_keys_callback cmp_keys_cb, binheap_mode_t mode)
{
    binheap_t *bh = calloc(1, sizeof(binheap_t));
    bh->trees = create_list();
    bh->cmp_keys_cb = cmp_keys_cb ? cmp_keys_cb : __cmp_keys_default;
    bh-> mode = mode;
    set_free_value_callback(bh->trees, (free_value_callback_t)binomial_tree_node_destroy);
    return bh;
}

void
binheap_destroy(binheap_t *bh)
{
    destroy_list(bh->trees);
    free(bh);
}

int binomial_tree_merge(binomial_tree_node_t *node1, binomial_tree_node_t *node2)
{
    node1->children = realloc(node1->children, sizeof(binomial_tree_node_t *) * (node1->num_children + 1));
    node1->children[node1->num_children++] = node2;
    node2->parent = node1;
    return 0;
}

int
binheap_insert(binheap_t *bh, void *key, size_t klen, void *value, size_t vlen)
{
    binomial_tree_node_t *node = calloc(1, sizeof(binomial_tree_node_t));
    node->bh = bh;
    node->key = malloc(klen);
    memcpy(node->key, key, klen);
    node->klen = klen;
    node->value = value;
    node->vlen = vlen;
    int order = 0;
    binomial_tree_node_t *tree = shift_value(bh->trees);
    while (tree && tree->num_children == order) {
        if (HAS_PRECEDENCE(bh, node->key, node->klen, tree->key, tree->klen)) {
            binomial_tree_merge(node, tree);
        } else {
            binomial_tree_merge(tree, node);
            node = tree;
        }
        order++;
        tree = shift_value(bh->trees);
    }
    if (tree)
        unshift_value(bh->trees, tree);
    unshift_value(bh->trees, node);

    bh->count++;
    return 0;
}

int
binheap_delete(binheap_t *bh, void *key, size_t klen, void **value, size_t *vlen)
{
    binomial_tree_node_t *tree = NULL;
    int i;
    for (i = 0; i < list_count(bh->trees); i++) {
        binomial_tree_node_t *cur_tree = pick_value(bh->trees, i);
        if (HAS_PRECEDENCE(bh, cur_tree->key, cur_tree->klen, key, klen)) {
            if (tree) {
                if (HAS_PRECEDENCE(bh, tree->key, tree->klen, cur_tree->key, cur_tree->klen)) {
                    tree = cur_tree;
                }
            } else {
                tree = cur_tree;
            }
        }
    }

    binomial_tree_node_t *to_delete = tree;
    while(to_delete && bh->cmp_keys_cb(to_delete->key, to_delete->klen, key, klen) != 0)
    {
        binomial_tree_node_t *next_tree = NULL;
        for (i = 0; i < to_delete->num_children; i++) {
            binomial_tree_node_t *child = to_delete->children[i];
            
            if (HAS_PRECEDENCE(bh, child->key, child->klen, key, klen)) {
                if (next_tree) {
                    if (HAS_PRECEDENCE(bh, next_tree->key, next_tree->klen, child->key, child->klen)) {
                        next_tree = child;
                    }
                } else {
                    next_tree = child;
                }
            }
        }
        if (next_tree) {
            to_delete = next_tree;
        } else {
            to_delete = NULL;
        }
    }

    if (to_delete) {
        if (value)
            *value = to_delete->value;
        if (vlen)
            *vlen = to_delete->vlen;
        binomial_tree_node_destroy(to_delete);
        return 0;
    }
    return -1;
}

static binomial_tree_node_t *
__binheap_maxmin(binheap_t *bh, uint32_t *index, int maxmin)
{
    int i;
    binomial_tree_node_t *node = NULL;
    for (i = 0; i < list_count(bh->trees); i ++) {
        binomial_tree_node_t *curtree = pick_value(bh->trees, i);
        if (!node) {
            node = curtree;
            if (index)
                *index = i;
            continue;
        }
        int is_bigger = (bh->cmp_keys_cb(curtree->key, curtree->klen, node->key, node->klen) >= 0);
        if ((maxmin == 0 && is_bigger) || (maxmin != 0 && !is_bigger))
        {
            node = curtree;
            if (index)
                *index = i;
        }
    }

    return node;
}

int
binheap_maximum(binheap_t *bh, void **key, size_t *klen, void **value, size_t *vlen)
{
    binomial_tree_node_t *maxroot = __binheap_maxmin(bh, NULL, 0);
    if (!maxroot)
        return -1;
    if (bh->mode == BINHEAP_MODE_MAX) {
        if (key)
            *key = maxroot->key;
        if (klen)
            *klen = maxroot->klen;
        if (value)
            *value = maxroot->value;
        if (vlen)
            *vlen = maxroot->vlen;
        return 0;
    } else {
        while (maxroot->num_children) {
            int max_child_index = binomial_tree_node_find_max_child(maxroot);
            maxroot = maxroot->children[max_child_index];
        }
        if (key)
            *key = maxroot->key;
        if (klen)
            *klen = maxroot->klen;
        if (value)
            *value = maxroot->value;
        if (vlen)
            *vlen = maxroot->vlen;
        return 0;
    }
}

int
binheap_minimum(binheap_t *bh, void **key, size_t *klen, void **value, size_t *vlen)
{
    binomial_tree_node_t *minroot = __binheap_maxmin(bh, NULL, 1);
    if (!minroot)
        return -1;
    if (bh->mode == BINHEAP_MODE_MAX) {
        if (minroot) { 
            while (minroot->num_children) {
                int min_child_index = binomial_tree_node_find_min_child(minroot);
                minroot = minroot->children[min_child_index];
            }
            if (key)
                *key = minroot->key;
            if (klen)
                *klen = minroot->klen;
            if (value)
                *value = minroot->value;
            if (vlen)
                *vlen = minroot->vlen;
            return 0;
        }
    } else {
        if (key)
            *key = minroot->key;
        if (klen)
            *klen = minroot->klen;
        if (value)
            *value = minroot->value;
        if (vlen)
            *vlen = minroot->vlen;
        return 0;
    }
    return -1;
}

int
binheap_delete_minimum(binheap_t *bh, void **value, size_t *vlen)
{
    uint32_t minidx = 0;
    binomial_tree_node_t *minroot = __binheap_maxmin(bh, &minidx, 1);

    if (!minroot)
        return -1;

    if (bh->mode == BINHEAP_MODE_MAX && minroot->num_children) {
        while (minroot->num_children) {
            int min_child_index = binomial_tree_node_find_min_child(minroot);
            minroot = minroot->children[min_child_index];
        }
    } else {
        (void)fetch_value(bh->trees, minidx);
        if (minroot->num_children) {
            int child_index = bh->mode == BINHEAP_MODE_MAX
                            ? binomial_tree_node_find_max_child(minroot)
                            : binomial_tree_node_find_min_child(minroot);
            insert_value(bh->trees, minroot->children[child_index], minidx);
        }
    }

    if (value)
        *value = minroot->value;
    if (vlen)
        *vlen = minroot->vlen;

    binomial_tree_node_destroy(minroot);

    return 0;
}

int
binheap_delete_maximum(binheap_t *bh, void **value, size_t *vlen)
{
    uint32_t maxidx = 0;
    binomial_tree_node_t *maxroot = __binheap_maxmin(bh, &maxidx, 0);

    if (!maxroot)
        return -1;

    if (bh->mode != BINHEAP_MODE_MAX && maxroot->num_children) {
        while (maxroot->num_children) {
            int max_child_index = binomial_tree_node_find_max_child(maxroot);
            maxroot = maxroot->children[max_child_index];
        }
    } else {
        (void)fetch_value(bh->trees, maxidx);
        if (maxroot->num_children) {
            int child_index = bh->mode == BINHEAP_MODE_MAX
                            ? binomial_tree_node_find_max_child(maxroot)
                            : binomial_tree_node_find_min_child(maxroot);
            insert_value(bh->trees, maxroot->children[child_index], maxidx);
        }
    }

    if (value)
        *value = maxroot->value;
    if (vlen)
        *vlen = maxroot->vlen;
    binomial_tree_node_destroy(maxroot);

    return 0;
}


uint32_t
binheap_count(binheap_t *bh)
{
    return bh->count;
}

binheap_t *binheap_merge(binheap_t *bh1, binheap_t *bh2)
{
    if (bh1->mode != bh2->mode) {
        // refuse to mere binomial heaps initialized with a different mode
        // TODO - error message
        return NULL;
    }
    linked_list_t *new_list = create_list();
    set_free_value_callback(new_list, (free_value_callback_t)binomial_tree_node_destroy);

    binomial_tree_node_t *node1 = shift_value(bh1->trees);
    binomial_tree_node_t *node2 = shift_value(bh2->trees);
    binomial_tree_node_t *merged = NULL;

    while (node1 || node2 || merged) {

        if (merged) {
            binomial_tree_node_t *node = NULL;
            if (node1 && node1->num_children == merged->num_children) {
                node = node1;
            } else if (node2 && node2->num_children  == merged->num_children) {
                node = node2;
            } else {
                if (!node1 && !node2) {
                    push_value(new_list, merged);
                    merged = NULL;
                    continue;
                }
                if (node1 && node1->num_children > merged->num_children) {
                    binomial_tree_node_t *tmp = node1;
                    node1 = merged;
                    merged = tmp;
                } else if (node2 && node2->num_children > merged->num_children) {
                    binomial_tree_node_t *tmp = node2;
                    node2 = merged;
                    merged = tmp;
                }
            }

            if (node) {
                if (HAS_PRECEDENCE(bh1, node->key, node->klen, merged->key, merged->klen)) {
                    binomial_tree_merge(node, merged);
                } else {
                    binomial_tree_merge(merged, node);
                    if (node == node1)
                        node1 = merged;
                    else
                        node2 = merged;
                }
                merged = NULL;
            }
        }

        if (node1 && !node2) {
            push_value(new_list, node1);
            node1 = shift_value(bh1->trees);
            continue;
        } else if (node2 && !node1) {
            push_value(new_list, node2);
            node2 = shift_value(bh2->trees);
            continue;
        } else if (merged && !node1 && !node2) {
            push_value(new_list, merged);
            merged = NULL;
            continue;
        }

        int order1 = node1->num_children;
        int order2 = node2->num_children;

        if (order1 < order2) {
            push_value(new_list, node1);
            node1 = shift_value(bh1->trees);
            continue;
        } else if (order1 > order2) {
            push_value(new_list, node2);
            node2 = shift_value(bh2->trees);
            continue;
        }

        if (HAS_PRECEDENCE(bh1, node1->key, node1->klen, node2->key, node2->klen)) {
            binomial_tree_merge(node1, node2);
            if (merged) {
                if (bh1->cmp_keys_cb(node1->key, node1->klen, merged->key, merged->klen) >= 0) {
                    binomial_tree_merge(node1, merged);
                    merged = node1;
                } else {
                    binomial_tree_merge(merged, node1);
                }
            } else {
                merged = node1;
            }
        } else {
            binomial_tree_merge(node2, node1);
            if (merged) {
                if (HAS_PRECEDENCE(bh1, node2->key, node2->klen, merged->key, merged->klen)) {
                    binomial_tree_merge(node2, merged);
                    merged = node2;
                } else {
                    binomial_tree_merge(merged, node2);
                }
            } else {
                merged = node2;
            }
        }

        node1 = shift_value(bh1->trees);
        node2 = shift_value(bh2->trees);
    }

    bh1->count += bh2->count;

    binheap_destroy(bh2);

    destroy_list(bh1->trees);
    bh1->trees = new_list;

    return bh1;
}