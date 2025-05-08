// --- INCLUDES END ---

struct treenode {
  char *key;
  void *value;
  struct treenode *child[2];
  int height;
};

void deltree(struct treenode *, void(void *), void(void *));
struct treenode *treefind(struct treenode *, const char *);
void *treeinsert(struct treenode **, char *, void *);
