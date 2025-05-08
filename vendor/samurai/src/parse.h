// --- INCLUDES END ---
struct environment;
struct node;

struct parseoptions {
  _Bool dupbuildwarn;
};

void parseinit(void);
void parse(const char *, struct environment *);

extern struct parseoptions parseopts;

enum {
  ninjamajor = 1,
  ninjaminor = 9,
};

void defaultnodes(void(struct node *));
