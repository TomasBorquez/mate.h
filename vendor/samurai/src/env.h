// --- INCLUDES END ---
struct evalstring;
struct string;

struct rule {
  char *name;
  struct treenode *bindings;
};

struct pool {
  char *name;
  int numjobs, maxjobs;

  struct edge *work;
};

void envinit(void);

struct environment *mkenv(struct environment *);
struct string *envvar(struct environment *, char *);
void envaddvar(struct environment *, char *, struct string *);
struct string *enveval(struct environment *, struct evalstring *);
struct rule *envrule(struct environment *, char *);
void envaddrule(struct environment *, struct rule *);

struct rule *mkrule(char *);
void ruleaddvar(struct rule *, char *, struct evalstring *);

struct pool *mkpool(char *);
struct pool *poolget(char *);

struct string *edgevar(struct edge *, char *, _Bool);

extern struct environment *rootenv;
extern struct rule phonyrule;
extern struct pool consolepool;
