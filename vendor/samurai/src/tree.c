#include <stdlib.h>
#include <string.h>

#include "tree.h"
#include "util.h"
// --- INCLUDES END ---

#define MAXH (sizeof(void *) * 8 * 3 / 2)

void deltree(struct treenode *n, void delkey(void *), void delval(void *)) {
  if (!n) return;
  if (delkey) delkey(n->key);
  if (delval) delval(n->value);
  deltree(n->child[0], delkey, delval);
  deltree(n->child[1], delkey, delval);
  free(n);
}

static inline int height(struct treenode *n) {
  return n ? n->height : 0;
}

static int rot(struct treenode **p, struct treenode *x, int dir) {
  struct treenode *y = x->child[dir];
  struct treenode *z = y->child[!dir];
  int hx = x->height;
  int hz = height(z);

  if (hz > height(y->child[dir])) {
    x->child[dir] = z->child[!dir];
    y->child[!dir] = z->child[dir];
    z->child[!dir] = x;
    z->child[dir] = y;
    x->height = hz;
    y->height = hz;
    z->height = hz + 1;
  } else {
    x->child[dir] = z;
    y->child[!dir] = x;
    x->height = hz + 1;
    y->height = hz + 2;
    z = y;
  }
  *p = z;
  return z->height - hx;
}

static int balance(struct treenode **p) {
  struct treenode *n = *p;
  int h0 = height(n->child[0]);
  int h1 = height(n->child[1]);

  if (h0 - h1 + 1u < 3u) {
    int old = n->height;
    n->height = h0 < h1 ? h1 + 1 : h0 + 1;
    return n->height - old;
  }
  return rot(p, n, h0 < h1);
}

struct treenode *treefind(struct treenode *n, const char *key) {
  int c;

  while (n) {
    c = strcmp(key, n->key);
    if (c == 0) return n;
    n = n->child[c > 0];
  }
  return NULL;
}

void *treeinsert(struct treenode **rootp, char *key, void *value) {
  struct treenode **a[MAXH], *n = *rootp, *r;
  void *old;
  int i = 0, c;

  a[i++] = rootp;
  while (n) {
    c = strcmp(key, n->key);
    if (c == 0) {
      old = n->value;
      n->value = value;
      return old;
    }
    a[i++] = &n->child[c > 0];
    n = n->child[c > 0];
  }
  r = xmalloc(sizeof(*r));
  r->key = key;
  r->value = value;
  r->child[0] = r->child[1] = NULL;
  r->height = 1;
  *a[--i] = r;
  while (i && balance(a[--i]))
    ;
  return NULL;
}
