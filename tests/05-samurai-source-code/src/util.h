struct buffer {
	char *data;
	size_t len, cap;
};

struct string {
	size_t n;
	char s[];
};

/* an unevaluated string */
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

/* append a byte to a buffer */
void bufadd(struct buffer *buf, char c);

/* allocates a new string with length n. n + 1 bytes are allocated for
 * s, but not initialized. */
struct string *mkstr(size_t n);

/* delete an unevaluated string */
void delevalstr(void *);

/* canonicalizes the given path by removing duplicate slashes, and
 * folding '/.' and 'foo/..' */
void canonpath(struct string *);
/* write a new file with the given name and contents */
int writefile(const char *, struct string *);
