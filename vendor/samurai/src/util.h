// --- INCLUDES END ---

struct buffer {
  char *data;
  size_t len, cap;
};

struct string {
  size_t n;
  char s[];
};

struct evalstring {
  char *var;
  struct string *str;
  struct evalstring *next;
};

#define LEN(a) (sizeof(a) / sizeof((a)[0]))

void warn(const char *, ...);
void fatal(const char *, ...);

void *xmalloc(size_t);
void *xreallocarray(void *, size_t, size_t);
char *xmemdup(const char *, size_t);
int xasprintf(char **, const char *, ...);

void bufadd(struct buffer *buf, char c);

struct string *mkstr(size_t n);

void delevalstr(void *);

void canonpath(struct string *);
int writefile(const char *, struct string *);
