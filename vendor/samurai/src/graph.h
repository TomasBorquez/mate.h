#include <stdint.h>
// --- INCLUDES END ---

enum {
  MTIME_UNKNOWN = -1,
  MTIME_MISSING = -2,
};

struct node {
  struct string *path, *shellpath;

  int64_t mtime, logmtime;

  struct edge *gen, **use;
  size_t nuse;

  uint64_t hash;

  int32_t id;

  _Bool dirty;
};

struct edge {
  struct rule *rule;
  struct pool *pool;
  struct environment *env;

  struct node **out, **in;
  size_t nout, nin;

  size_t outimpidx;
  size_t inimpidx, inorderidx;

  uint64_t hash;

  size_t nblock;
  size_t nprune;

  enum {
    FLAG_WORK = 1 << 0,
    FLAG_HASH = 1 << 1,
    FLAG_DIRTY_IN = 1 << 3,
    FLAG_DIRTY_OUT = 1 << 4,
    FLAG_DIRTY = FLAG_DIRTY_IN | FLAG_DIRTY_OUT,
    FLAG_CYCLE = 1 << 5,
    FLAG_DEPS = 1 << 6,
  } flags;

  struct edge *worknext;
  struct edge *allnext;
};

void graphinit(void);

struct node *mknode(struct string *);
struct node *nodeget(const char *, size_t);
void nodestat(struct node *);
struct string *nodepath(struct node *, _Bool);
void nodeuse(struct node *, struct edge *);

struct edge *mkedge(struct environment *parent);
void edgehash(struct edge *);
void edgeadddeps(struct edge *e, struct node **deps, size_t ndeps);

extern struct edge *alledges;
