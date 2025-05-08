// --- INCLUDES END ---
struct string;

void osgetcwd(char *, size_t);
void oschdir(const char *);
int osmkdirs(struct string *, _Bool);
int64_t osmtime(const char *);
