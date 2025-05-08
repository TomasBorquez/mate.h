// --- INCLUDES END ---
struct node;

struct buildoptions {
  size_t maxjobs, maxfail;
  _Bool verbose, explain, keepdepfile, keeprsp, dryrun;
  const char *statusfmt;
  double maxload;
};

extern struct buildoptions buildopts;

void buildreset(void);
void buildadd(struct node *);
void build(void);
