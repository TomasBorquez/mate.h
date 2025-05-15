#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stdarg.h>
#include <string.h>
#include <sys/stat.h>
#include <poll.h>
#include <signal.h>
#include <spawn.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
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
struct edge;
void depsinit(const char *);
void depsclose(void);
void depsload(struct edge *);
void depsrecord(struct edge *);
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
struct hashtablekey {
  uint64_t hash;
  const char *str;
  size_t len;
};
void htabkey(struct hashtablekey *, const char *, size_t);
struct hashtable *mkhtab(size_t);
void delhtab(struct hashtable *, void(void *));
void **htabput(struct hashtable *, struct hashtablekey *);
void *htabget(struct hashtable *, struct hashtablekey *);
uint64_t murmurhash64a(const void *, size_t);
struct node;
void loginit(const char *);
void logclose(void);
void logrecord(struct node *);
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
extern const char *argv0;
#define ARGBEGIN                                                         \
  for (;;) {                                                             \
    if (argc > 0) ++argv, --argc;                                        \
    if (argc == 0 || (*argv)[0] != '-') break;                           \
    if ((*argv)[1] == '-' && !(*argv)[2]) {                              \
      ++argv, --argc;                                                    \
      break;                                                             \
    }                                                                    \
    for (char *opt_ = &(*argv)[1], done_ = 0; !done_ && *opt_; ++opt_) { \
      switch (*opt_)
#define ARGEND \
  }            \
  }
#define EARGF(x) (done_ = 1, *++opt_ ? opt_ : argv[1] ? --argc, *++argv : ((x), abort(), (char *)0))
enum token {
  BUILD,
  DEFAULT,
  INCLUDE,
  POOL,
  RULE,
  SUBNINJA,
  VARIABLE,
};
struct scanner {
  FILE *f;
  const char *path;
  int chr, line, col;
};
extern struct evalstring **paths;
extern size_t npaths;
void scaninit(struct scanner *, const char *);
void scanclose(struct scanner *);
void scanerror(struct scanner *, const char *, ...);
int scankeyword(struct scanner *, char **);
char *scanname(struct scanner *);
struct evalstring *scanstring(struct scanner *, _Bool);
void scanpaths(struct scanner *);
void scanchar(struct scanner *, int);
int scanpipe(struct scanner *, int);
_Bool scanindent(struct scanner *);
void scannewline(struct scanner *);
struct tool {
  const char *name;
  int (*run)(int, char *[]);
};
const struct tool *toolget(const char *);
struct treenode {
  char *key;
  void *value;
  struct treenode *child[2];
  int height;
};
void deltree(struct treenode *, void(void *), void(void *));
struct treenode *treefind(struct treenode *, const char *);
void *treeinsert(struct treenode **, char *, void *);
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
struct string;
void osgetcwd(char *, size_t);
void oschdir(const char *);
int osmkdirs(struct string *, _Bool);
int64_t osmtime(const char *);
struct job {
  struct string *cmd;
  struct edge *edge;
  struct buffer buf;
  size_t next;
  pid_t pid;
  int fd;
  bool failed;
};
struct buildoptions buildopts = {.maxfail = 1};
static struct edge *work;
static size_t nstarted, nfinished, ntotal;
static bool consoleused;
static struct timespec starttime;
void buildreset(void) {
  struct edge *e;
  for (e = alledges; e; e = e->allnext)
    e->flags &= ~FLAG_WORK;
}
static bool isnewer(struct node *n1, struct node *n2) {
  return n1 && n1->mtime > n2->mtime;
}
static bool isdirty(struct node *n, struct node *newest, bool generator, bool restat) {
  struct edge *e;
  e = n->gen;
  if (e->rule == &phonyrule) {
    if (e->nin > 0 || n->mtime != MTIME_MISSING) return false;
    if (buildopts.explain) warn("explain %s: phony and no inputs", n->path->s);
    return true;
  }
  if (n->mtime == MTIME_MISSING) {
    if (buildopts.explain) warn("explain %s: missing", n->path->s);
    return true;
  }
  if (isnewer(newest, n) && (!restat || n->logmtime == MTIME_MISSING)) {
    if (buildopts.explain) {
      warn("explain %s: older than input '%s': %" PRId64 " vs %" PRId64, n->path->s, newest->path->s, n->mtime, newest->mtime);
    }
    return true;
  }
  if (n->logmtime == MTIME_MISSING) {
    if (!generator) {
      if (buildopts.explain) warn("explain %s: no record in .ninja_log", n->path->s);
      return true;
    }
  } else if (newest && n->logmtime < newest->mtime) {
    if (buildopts.explain) {
      warn("explain %s: recorded mtime is older than input '%s': %" PRId64 " vs %" PRId64, n->path->s, newest->path->s, n->logmtime, newest->mtime);
    }
    return true;
  }
  if (generator) return false;
  edgehash(e);
  if (e->hash == n->hash) return false;
  if (buildopts.explain) warn("explain %s: command line changed", n->path->s);
  return true;
}
static void queue(struct edge *e) {
  struct edge **front = &work;
  if (e->pool && e->rule != &phonyrule) {
    if (e->pool->numjobs == e->pool->maxjobs) front = &e->pool->work;
    else ++e->pool->numjobs;
  }
  e->worknext = *front;
  *front = e;
}
void buildadd(struct node *n) {
  struct edge *e;
  struct node *newest;
  size_t i;
  bool generator, restat;
  e = n->gen;
  if (!e) {
    if (n->mtime == MTIME_UNKNOWN) nodestat(n);
    if (n->mtime == MTIME_MISSING) fatal("file is missing and not created by any action: '%s'", n->path->s);
    n->dirty = false;
    return;
  }
  if (e->flags & FLAG_CYCLE) fatal("dependency cycle involving '%s'", n->path->s);
  if (e->flags & FLAG_WORK) return;
  e->flags |= FLAG_CYCLE | FLAG_WORK;
  for (i = 0; i < e->nout; ++i) {
    n = e->out[i];
    n->dirty = false;
    if (n->mtime == MTIME_UNKNOWN) nodestat(n);
  }
  depsload(e);
  e->nblock = 0;
  newest = NULL;
  for (i = 0; i < e->nin; ++i) {
    n = e->in[i];
    buildadd(n);
    if (i < e->inorderidx) {
      if (n->dirty) e->flags |= FLAG_DIRTY_IN;
      if (n->mtime != MTIME_MISSING && !isnewer(newest, n)) newest = n;
    }
    if (n->dirty || (n->gen && n->gen->nblock > 0)) ++e->nblock;
  }
  generator = edgevar(e, "generator", true);
  restat = edgevar(e, "restat", true);
  for (i = 0; i < e->nout && !(e->flags & FLAG_DIRTY_OUT); ++i) {
    n = e->out[i];
    if (isdirty(n, newest, generator, restat)) {
      n->dirty = true;
      e->flags |= FLAG_DIRTY_OUT;
    }
  }
  if (e->flags & FLAG_DIRTY) {
    for (i = 0; i < e->nout; ++i) {
      n = e->out[i];
      if (buildopts.explain && !n->dirty) {
        if (e->flags & FLAG_DIRTY_IN) warn("explain %s: input is dirty", n->path->s);
        else if (e->flags & FLAG_DIRTY_OUT) warn("explain %s: output of generating action is dirty", n->path->s);
      }
      n->dirty = true;
    }
  }
  if (!(e->flags & FLAG_DIRTY_OUT)) e->nprune = e->nblock;
  if (e->flags & FLAG_DIRTY) {
    if (e->nblock == 0) queue(e);
    if (e->rule != &phonyrule) ++ntotal;
  }
  e->flags &= ~FLAG_CYCLE;
}
static size_t formatstatus(char *buf, size_t len) {
  const char *fmt;
  size_t ret = 0;
  int n;
  struct timespec endtime;
  for (fmt = buildopts.statusfmt; *fmt; ++fmt) {
    if (*fmt != '%' || *++fmt == '%') {
      if (len > 1) {
        *buf++ = *fmt;
        --len;
      }
      ++ret;
      continue;
    }
    n = 0;
    switch (*fmt) {
    case 's':
      n = snprintf(buf, len, "%zu", nstarted);
      break;
    case 'f':
      n = snprintf(buf, len, "%zu", nfinished);
      break;
    case 't':
      n = snprintf(buf, len, "%zu", ntotal);
      break;
    case 'r':
      n = snprintf(buf, len, "%zu", nstarted - nfinished);
      break;
    case 'u':
      n = snprintf(buf, len, "%zu", ntotal - nstarted);
      break;
    case 'p':
      n = snprintf(buf, len, "%3zu%%", 100 * nfinished / ntotal);
      break;
    case 'o':
      if (clock_gettime(CLOCK_MONOTONIC, &endtime) != 0) {
        warn("clock_gettime:");
        break;
      }
      n = snprintf(buf, len, "%.1f", nfinished / ((endtime.tv_sec - starttime.tv_sec) + 0.000000001 * (endtime.tv_nsec - starttime.tv_nsec)));
      break;
    case 'e':
      if (clock_gettime(CLOCK_MONOTONIC, &endtime) != 0) {
        warn("clock_gettime:");
        break;
      }
      n = snprintf(buf, len, "%.3f", (endtime.tv_sec - starttime.tv_sec) + 0.000000001 * (endtime.tv_nsec - starttime.tv_nsec));
      break;
    default:
      fatal("unknown placeholder '%%%c' in $NINJA_STATUS", *fmt);
      continue;
    }
    if (n < 0) fatal("snprintf:");
    ret += n;
    if ((size_t)n > len) n = len;
    buf += n;
    len -= n;
  }
  if (len > 0) *buf = '\0';
  return ret;
}
static void printstatus(struct edge *e, struct string *cmd) {
  struct string *description;
  char status[256];
  description = buildopts.verbose ? NULL : edgevar(e, "description", true);
  if (!description || description->n == 0) description = cmd;
  formatstatus(status, sizeof(status));
  fputs(status, stdout);
  puts(description->s);
}
static int jobstart(struct job *j, struct edge *e) {
  extern char **environ;
  size_t i;
  struct node *n;
  struct string *rspfile, *content;
  int fd[2];
  posix_spawn_file_actions_t actions;
  char *argv[] = {"/bin/sh", "-c", NULL, NULL};
  ++nstarted;
  for (i = 0; i < e->nout; ++i) {
    n = e->out[i];
    if (n->mtime == MTIME_MISSING) {
      if (osmkdirs(n->path, true) < 0) goto err0;
    }
  }
  rspfile = edgevar(e, "rspfile", false);
  if (rspfile) {
    content = edgevar(e, "rspfile_content", true);
    if (writefile(rspfile->s, content) < 0) goto err0;
  }
  if (pipe(fd) < 0) {
    warn("pipe:");
    goto err1;
  }
  j->edge = e;
  j->cmd = edgevar(e, "command", true);
  j->fd = fd[0];
  argv[2] = j->cmd->s;
  if (!consoleused) printstatus(e, j->cmd);
  if ((errno = posix_spawn_file_actions_init(&actions))) {
    warn("posix_spawn_file_actions_init:");
    goto err2;
  }
  if ((errno = posix_spawn_file_actions_addclose(&actions, fd[0]))) {
    warn("posix_spawn_file_actions_addclose:");
    goto err3;
  }
  if (e->pool != &consolepool) {
    if ((errno = posix_spawn_file_actions_addopen(&actions, 0, "/dev/null", O_RDONLY, 0))) {
      warn("posix_spawn_file_actions_addopen:");
      goto err3;
    }
    if ((errno = posix_spawn_file_actions_adddup2(&actions, fd[1], 1))) {
      warn("posix_spawn_file_actions_adddup2:");
      goto err3;
    }
    if ((errno = posix_spawn_file_actions_adddup2(&actions, fd[1], 2))) {
      warn("posix_spawn_file_actions_adddup2:");
      goto err3;
    }
    if ((errno = posix_spawn_file_actions_addclose(&actions, fd[1]))) {
      warn("posix_spawn_file_actions_addclose:");
      goto err3;
    }
  }
  if ((errno = posix_spawn(&j->pid, argv[0], &actions, NULL, argv, environ))) {
    warn("posix_spawn %s:", j->cmd->s);
    goto err3;
  }
  posix_spawn_file_actions_destroy(&actions);
  close(fd[1]);
  j->failed = false;
  if (e->pool == &consolepool) consoleused = true;
  return j->fd;
err3:
  posix_spawn_file_actions_destroy(&actions);
err2:
  close(fd[0]);
  close(fd[1]);
err1:
  if (rspfile && !buildopts.keeprsp) remove(rspfile->s);
err0:
  return -1;
}
static void nodedone(struct node *n, bool prune) {
  struct edge *e;
  size_t i, j;
  for (i = 0; i < n->nuse; ++i) {
    e = n->use[i];
    if (!(e->flags & FLAG_WORK)) continue;
    if (!(e->flags & (prune ? FLAG_DIRTY_OUT : FLAG_DIRTY)) && --e->nprune == 0) {
      for (j = 0; j < e->nout; ++j)
        nodedone(e->out[j], true);
      if (e->flags & FLAG_DIRTY && e->rule != &phonyrule) --ntotal;
    } else if (--e->nblock == 0) {
      queue(e);
    }
  }
}
static bool shouldprune(struct edge *e, struct node *n, int64_t old) {
  struct node *in, *newest;
  size_t i;
  if (old != n->mtime) return false;
  newest = NULL;
  for (i = 0; i < e->inorderidx; ++i) {
    in = e->in[i];
    nodestat(in);
    if (in->mtime != MTIME_MISSING && !isnewer(newest, in)) newest = in;
  }
  if (newest) n->logmtime = newest->mtime;
  return true;
}
static void edgedone(struct edge *e) {
  struct node *n;
  size_t i;
  struct string *rspfile;
  bool restat;
  int64_t old;
  restat = edgevar(e, "restat", true);
  for (i = 0; i < e->nout; ++i) {
    n = e->out[i];
    old = n->mtime;
    nodestat(n);
    n->logmtime = n->mtime == MTIME_MISSING ? 0 : n->mtime;
    nodedone(n, restat && shouldprune(e, n, old));
  }
  rspfile = edgevar(e, "rspfile", false);
  if (rspfile && !buildopts.keeprsp) remove(rspfile->s);
  edgehash(e);
  depsrecord(e);
  for (i = 0; i < e->nout; ++i) {
    n = e->out[i];
    n->hash = e->hash;
    logrecord(n);
  }
}
static void jobdone(struct job *j) {
  int status;
  struct edge *e, *new;
  struct pool *p;
  ++nfinished;
  if (waitpid(j->pid, &status, 0) < 0) {
    warn("waitpid %d:", j->pid);
    j->failed = true;
  } else if (WIFEXITED(status)) {
    if (WEXITSTATUS(status) != 0) {
      warn("job failed with status %d: %s", WEXITSTATUS(status), j->cmd->s);
      j->failed = true;
    }
  } else if (WIFSIGNALED(status)) {
    warn("job terminated due to signal %d: %s", WTERMSIG(status), j->cmd->s);
    j->failed = true;
  } else {
    warn("job status unknown: %s", j->cmd->s);
    j->failed = true;
  }
  close(j->fd);
  if (j->buf.len && (!consoleused || j->failed)) fwrite(j->buf.data, 1, j->buf.len, stdout);
  j->buf.len = 0;
  e = j->edge;
  if (e->pool) {
    p = e->pool;
    if (p == &consolepool) consoleused = false;
    if (p->work) {
      new = p->work;
      p->work = p->work->worknext;
      new->worknext = work;
      work = new;
    } else {
      --p->numjobs;
    }
  }
  if (!j->failed) edgedone(e);
}
static bool jobwork(struct job *j) {
  char *newdata;
  size_t newcap;
  ssize_t n;
  if (j->buf.cap - j->buf.len < BUFSIZ / 2) {
    newcap = j->buf.cap + BUFSIZ;
    newdata = realloc(j->buf.data, newcap);
    if (!newdata) {
      warn("realloc:");
      goto kill;
    }
    j->buf.cap = newcap;
    j->buf.data = newdata;
  }
  n = read(j->fd, j->buf.data + j->buf.len, j->buf.cap - j->buf.len);
  if (n > 0) {
    j->buf.len += n;
    return true;
  }
  if (n == 0) goto done;
  warn("read:");
kill:
  kill(j->pid, SIGTERM);
  j->failed = true;
done:
  jobdone(j);
  return false;
}
static double queryload(void) {
#ifdef HAVE_GETLOADAVG
  double load;
  if (getloadavg(&load, 1) == -1) {
    warn("getloadavg:");
    load = 100.0;
  }
  return load;
#else
  return 0;
#endif
}
void build(void) {
  struct job *jobs = NULL;
  struct pollfd *fds = NULL;
  size_t i, next = 0, jobslen = 0, maxjobs = buildopts.maxjobs, numjobs = 0, numfail = 0;
  struct edge *e;
  if (ntotal == 0) {
    warn("nothing to do");
    return;
  }
  clock_gettime(CLOCK_MONOTONIC, &starttime);
  formatstatus(NULL, 0);
  nstarted = 0;
  for (;;) {
    if (buildopts.maxload) maxjobs = queryload() > buildopts.maxload ? 1 : buildopts.maxjobs;
    while (work && numjobs < maxjobs && numfail < buildopts.maxfail) {
      e = work;
      work = work->worknext;
      if (e->rule != &phonyrule && buildopts.dryrun) {
        ++nstarted;
        printstatus(e, edgevar(e, "command", true));
        ++nfinished;
      }
      if (e->rule == &phonyrule || buildopts.dryrun) {
        for (i = 0; i < e->nout; ++i)
          nodedone(e->out[i], false);
        continue;
      }
      if (next == jobslen) {
        jobslen = jobslen ? jobslen * 2 : 8;
        if (jobslen > buildopts.maxjobs) jobslen = buildopts.maxjobs;
        jobs = xreallocarray(jobs, jobslen, sizeof(jobs[0]));
        fds = xreallocarray(fds, jobslen, sizeof(fds[0]));
        for (i = next; i < jobslen; ++i) {
          jobs[i].buf.data = NULL;
          jobs[i].buf.len = 0;
          jobs[i].buf.cap = 0;
          jobs[i].next = i + 1;
          fds[i].fd = -1;
          fds[i].events = POLLIN;
        }
      }
      fds[next].fd = jobstart(&jobs[next], e);
      if (fds[next].fd < 0) {
        warn("job failed to start");
        ++numfail;
      } else {
        next = jobs[next].next;
        ++numjobs;
      }
    }
    if (numjobs == 0) break;
    if (poll(fds, jobslen, 5000) < 0) fatal("poll:");
    for (i = 0; i < jobslen; ++i) {
      if (!fds[i].revents || jobwork(&jobs[i])) continue;
      --numjobs;
      jobs[i].next = next;
      fds[i].fd = -1;
      next = i;
      if (jobs[i].failed) ++numfail;
    }
  }
  for (i = 0; i < jobslen; ++i)
    free(jobs[i].buf.data);
  free(jobs);
  free(fds);
  if (numfail > 0) {
    if (numfail < buildopts.maxfail) fatal("cannot make progress due to previous errors");
    else if (numfail > 1) fatal("subcommands failed");
    else fatal("subcommand failed");
  }
  ntotal = 0;
}
#define MAX_RECORD_SIZE (1 << 19)
struct nodearray {
  struct node **node;
  size_t len;
};
struct entry {
  struct node *node;
  struct nodearray deps;
  int64_t mtime;
};
static const char depsname[] = ".ninja_deps";
static const char depstmpname[] = ".ninja_deps.tmp";
static const char depsheader[] = "# ninjadeps\n";
static const uint32_t depsver = 4;
static FILE *depsfile;
static struct entry *entries;
static size_t entrieslen, entriescap;
static void depswrite(const void *p, size_t n, size_t m) {
  if (fwrite(p, n, m, depsfile) != m) fatal("deps log write:");
}
static bool recordid(struct node *n) {
  uint32_t sz, chk;
  if (n->id != -1) return false;
  if (entrieslen == INT32_MAX) fatal("too many nodes");
  n->id = entrieslen++;
  sz = (n->path->n + 7) & ~3;
  if (sz + 4 >= MAX_RECORD_SIZE) fatal("ID record too large");
  depswrite(&sz, 4, 1);
  depswrite(n->path->s, 1, n->path->n);
  depswrite((char[4]){0}, 1, sz - n->path->n - 4);
  chk = ~n->id;
  depswrite(&chk, 4, 1);
  return true;
}
static void recorddeps(struct node *out, struct nodearray *deps, int64_t mtime) {
  uint32_t sz, m;
  size_t i;
  sz = 12 + deps->len * 4;
  if (sz + 4 >= MAX_RECORD_SIZE) fatal("deps record too large");
  sz |= 0x80000000;
  depswrite(&sz, 4, 1);
  depswrite(&out->id, 4, 1);
  m = mtime & 0xffffffff;
  depswrite(&m, 4, 1);
  m = (mtime >> 32) & 0xffffffff;
  depswrite(&m, 4, 1);
  for (i = 0; i < deps->len; ++i)
    depswrite(&deps->node[i]->id, 4, 1);
}
void depsinit(const char *builddir) {
  char *depspath = (char *)depsname, *depstmppath = (char *)depstmpname;
  uint32_t *buf, cap, ver, sz, id;
  size_t len, i, j, nrecord;
  bool isdep;
  struct string *path;
  struct node *n;
  struct edge *e;
  struct entry *entry, *oldentries;
  if (depsfile) fclose(depsfile);
  entrieslen = 0;
  cap = BUFSIZ;
  buf = xmalloc(cap);
  if (builddir) xasprintf(&depspath, "%s/%s", builddir, depsname);
  depsfile = fopen(depspath, "r+");
  if (!depsfile) {
    if (errno != ENOENT) fatal("open %s:", depspath);
    goto rewrite;
  }
  if (!fgets((char *)buf, sizeof(depsheader), depsfile)) goto rewrite;
  if (strcmp((char *)buf, depsheader) != 0) {
    warn("invalid deps log header");
    goto rewrite;
  }
  if (fread(&ver, sizeof(ver), 1, depsfile) != 1) {
    warn(ferror(depsfile) ? "deps log read:" : "deps log truncated");
    goto rewrite;
  }
  if (ver != depsver) {
    warn("unknown deps log version");
    goto rewrite;
  }
  for (nrecord = 0;; ++nrecord) {
    if (fread(&sz, sizeof(sz), 1, depsfile) != 1) break;
    isdep = sz & 0x80000000;
    sz &= 0x7fffffff;
    if (sz > MAX_RECORD_SIZE) {
      warn("deps record too large");
      goto rewrite;
    }
    if (sz > cap) {
      do
        cap *= 2;
      while (sz > cap);
      free(buf);
      buf = xmalloc(cap);
    }
    if (fread(buf, sz, 1, depsfile) != 1) {
      warn(ferror(depsfile) ? "deps log read:" : "deps log truncated");
      goto rewrite;
    }
    if (sz % 4) {
      warn("invalid size, must be multiple of 4: %" PRIu32, sz);
      goto rewrite;
    }
    if (isdep) {
      if (sz < 12) {
        warn("invalid size, must be at least 12: %" PRIu32, sz);
        goto rewrite;
      }
      sz -= 12;
      id = buf[0];
      if (id >= entrieslen) {
        warn("invalid node ID: %" PRIu32, id);
        goto rewrite;
      }
      entry = &entries[id];
      entry->mtime = (int64_t)buf[2] << 32 | buf[1];
      e = entry->node->gen;
      if (!e || !edgevar(e, "deps", true)) continue;
      sz /= 4;
      free(entry->deps.node);
      entry->deps.len = sz;
      entry->deps.node = xreallocarray(NULL, sz, sizeof(n));
      for (i = 0; i < sz; ++i) {
        id = buf[3 + i];
        if (id >= entrieslen) {
          warn("invalid node ID: %" PRIu32, id);
          goto rewrite;
        }
        entry->deps.node[i] = entries[id].node;
      }
    } else {
      if (sz <= 4) {
        warn("invalid size, must be greater than 4: %" PRIu32, sz);
        goto rewrite;
      }
      if (entrieslen != ~buf[sz / 4 - 1]) {
        warn("corrupt deps log, bad checksum");
        goto rewrite;
      }
      if (entrieslen == INT32_MAX) {
        warn("too many nodes in deps log");
        goto rewrite;
      }
      len = sz - 4;
      while (((char *)buf)[len - 1] == '\0')
        --len;
      path = mkstr(len);
      memcpy(path->s, buf, len);
      path->s[len] = '\0';
      n = mknode(path);
      if (entrieslen >= entriescap) {
        entriescap = entriescap ? entriescap * 2 : 1024;
        entries = xreallocarray(entries, entriescap, sizeof(entries[0]));
      }
      n->id = entrieslen;
      entries[entrieslen++] = (struct entry){.node = n};
    }
  }
  if (ferror(depsfile)) {
    warn("deps log read:");
    goto rewrite;
  }
  if (nrecord <= 1000 || nrecord < 3 * entrieslen) {
    if (builddir) free(depspath);
    free(buf);
    return;
  }
rewrite:
  free(buf);
  if (depsfile) fclose(depsfile);
  if (builddir) xasprintf(&depstmppath, "%s/%s", builddir, depstmpname);
  depsfile = fopen(depstmppath, "w");
  if (!depsfile) fatal("open %s:", depstmppath);
  depswrite(depsheader, 1, sizeof(depsheader) - 1);
  depswrite(&depsver, 1, sizeof(depsver));
  for (i = 0; i < entrieslen; ++i)
    entries[i].node->id = -1;
  oldentries = xreallocarray(NULL, entrieslen, sizeof(entries[0]));
  memcpy(oldentries, entries, entrieslen * sizeof(entries[0]));
  len = entrieslen;
  entrieslen = 0;
  for (i = 0; i < len; ++i) {
    entry = &oldentries[i];
    if (!entry->deps.len) continue;
    recordid(entry->node);
    entries[entry->node->id] = *entry;
    for (j = 0; j < entry->deps.len; ++j)
      recordid(entry->deps.node[j]);
    recorddeps(entry->node, &entry->deps, entry->mtime);
  }
  free(oldentries);
  fflush(depsfile);
  if (ferror(depsfile)) fatal("deps log write failed");
  if (rename(depstmppath, depspath) < 0) fatal("deps log rename:");
  if (builddir) {
    free(depstmppath);
    free(depspath);
  }
}
void depsclose(void) {
  fflush(depsfile);
  if (ferror(depsfile)) fatal("deps log write failed");
  fclose(depsfile);
}
static struct nodearray *depsparse(const char *name, bool allowmissing) {
  static struct buffer buf;
  static struct nodearray deps;
  static size_t depscap;
  struct string *in, *out = NULL;
  FILE *f;
  int c, n;
  bool sawcolon;
  deps.len = 0;
  f = fopen(name, "r");
  if (!f) {
    if (errno == ENOENT && allowmissing) return &deps;
    return NULL;
  }
  sawcolon = false;
  buf.len = 0;
  c = getc(f);
  for (;;) {
    while (isalnum(c) || strchr("$+,-./@\\_", c)) {
      switch (c) {
      case '\\':
        n = 0;
        do {
          c = getc(f);
          if (++n % 2 == 0) bufadd(&buf, '\\');
        } while (c == '\\');
        if ((c == ' ' || c == '\t') && n % 2 != 0) break;
        for (; n > 2; n -= 2)
          bufadd(&buf, '\\');
        switch (c) {
        case '#':
          break;
        case '\n':
          c = ' ';
          continue;
        default:
          bufadd(&buf, '\\');
          continue;
        }
        break;
      case '$':
        c = getc(f);
        if (c != '$') {
          warn("bad depfile '%s': contains variable reference", name);
          goto err;
        }
        break;
      }
      bufadd(&buf, c);
      c = getc(f);
    }
    if (sawcolon) {
      if (!isspace(c) && c != EOF) {
        warn("bad depfile '%s': '%c' is not a valid target character", name, c);
        goto err;
      }
      if (buf.len > 0) {
        if (deps.len == depscap) {
          depscap = deps.node ? depscap * 2 : 32;
          deps.node = xreallocarray(deps.node, depscap, sizeof(deps.node[0]));
        }
        in = mkstr(buf.len);
        memcpy(in->s, buf.data, buf.len);
        in->s[buf.len] = '\0';
        deps.node[deps.len++] = mknode(in);
      }
      if (c == '\n') {
        sawcolon = false;
        do
          c = getc(f);
        while (c == '\n');
      }
      if (c == EOF) break;
    } else {
      while (isblank(c))
        c = getc(f);
      if (c == EOF) break;
      if (c != ':') {
        warn("bad depfile '%s': expected ':', saw '%c'", name, c);
        goto err;
      }
      if (!out) {
        out = mkstr(buf.len);
        memcpy(out->s, buf.data, buf.len);
        out->s[buf.len] = '\0';
      } else if (out->n != buf.len || memcmp(buf.data, out->s, buf.len) != 0) {
        warn("bad depfile '%s': multiple outputs: %.*s != %s", name, (int)buf.len, buf.data, out->s);
        goto err;
      }
      sawcolon = true;
      c = getc(f);
    }
    buf.len = 0;
    for (;;) {
      if (c == '\\') {
        if (getc(f) != '\n') {
          warn("bad depfile '%s': '\\' only allowed before newline", name);
          goto err;
        }
      } else if (!isblank(c)) {
        break;
      }
      c = getc(f);
    }
  }
  if (ferror(f)) {
    warn("depfile read '%s':", name);
    goto err;
  }
  fclose(f);
  return &deps;
err:
  fclose(f);
  return NULL;
}
void depsload(struct edge *e) {
  struct string *deptype, *depfile;
  struct nodearray *deps = NULL;
  struct node *n;
  if (e->flags & FLAG_DEPS) return;
  e->flags |= FLAG_DEPS;
  n = e->out[0];
  deptype = edgevar(e, "deps", true);
  if (deptype) {
    if (n->id != -1 && n->mtime <= entries[n->id].mtime) deps = &entries[n->id].deps;
    else if (buildopts.explain) warn("explain %s: missing or outdated record in .ninja_deps", n->path->s);
  } else {
    depfile = edgevar(e, "depfile", false);
    if (!depfile) return;
    deps = depsparse(depfile->s, false);
    if (buildopts.explain && !deps) warn("explain %s: missing or invalid depfile", n->path->s);
  }
  if (deps) {
    edgeadddeps(e, deps->node, deps->len);
  } else {
    n->dirty = true;
    e->flags |= FLAG_DIRTY_OUT;
  }
}
void depsrecord(struct edge *e) {
  struct string *deptype, *depfile;
  struct nodearray *deps;
  struct node *out, *n;
  struct entry *entry;
  size_t i;
  bool update;
  deptype = edgevar(e, "deps", true);
  if (!deptype || deptype->n == 0) return;
  if (strcmp(deptype->s, "gcc") != 0) {
    warn("unsuported deps type: %s", deptype->s);
    return;
  }
  depfile = edgevar(e, "depfile", false);
  if (!depfile || depfile->n == 0) {
    warn("deps but no depfile");
    return;
  }
  out = e->out[0];
  deps = depsparse(depfile->s, true);
  if (!buildopts.keepdepfile) remove(depfile->s);
  if (!deps) return;
  update = false;
  entry = NULL;
  if (recordid(out)) {
    update = true;
  } else {
    entry = &entries[out->id];
    if (entry->mtime != out->mtime || entry->deps.len != deps->len) update = true;
    for (i = 0; i < deps->len && !update; ++i) {
      if (entry->deps.node[i] != deps->node[i]) update = true;
    }
  }
  for (i = 0; i < deps->len; ++i) {
    n = deps->node[i];
    if (recordid(n)) update = true;
  }
  if (update) {
    recorddeps(out, deps, out->mtime);
    if (fflush(depsfile) < 0) fatal("deps log flush:");
  }
}
struct environment {
  struct environment *parent;
  struct treenode *bindings;
  struct treenode *rules;
  struct environment *allnext;
};
struct environment *rootenv;
struct rule phonyrule = {.name = "phony"};
struct pool consolepool = {.name = "console", .maxjobs = 1};
static struct treenode *pools;
static struct environment *allenvs;
static void addpool(struct pool *);
static void delpool(void *);
static void delrule(void *);
void envinit(void) {
  struct environment *env;
  while (allenvs) {
    env = allenvs;
    allenvs = env->allnext;
    deltree(env->bindings, free, free);
    deltree(env->rules, NULL, delrule);
    free(env);
  }
  deltree(pools, NULL, delpool);
  rootenv = mkenv(NULL);
  envaddrule(rootenv, &phonyrule);
  pools = NULL;
  addpool(&consolepool);
}
static void addvar(struct treenode **tree, char *var, void *val) {
  char *old;
  old = treeinsert(tree, var, val);
  if (old) free(old);
}
struct environment *mkenv(struct environment *parent) {
  struct environment *env;
  env = xmalloc(sizeof(*env));
  env->parent = parent;
  env->bindings = NULL;
  env->rules = NULL;
  env->allnext = allenvs;
  allenvs = env;
  return env;
}
struct string *envvar(struct environment *env, char *var) {
  struct treenode *n;
  do {
    n = treefind(env->bindings, var);
    if (n) return n->value;
    env = env->parent;
  } while (env);
  return NULL;
}
void envaddvar(struct environment *env, char *var, struct string *val) {
  addvar(&env->bindings, var, val);
}
static struct string *merge(struct evalstring *str, size_t n) {
  struct string *result;
  struct evalstring *p;
  char *s;
  result = mkstr(n);
  s = result->s;
  for (p = str; p; p = p->next) {
    if (!p->str) continue;
    memcpy(s, p->str->s, p->str->n);
    s += p->str->n;
  }
  *s = '\0';
  return result;
}
struct string *enveval(struct environment *env, struct evalstring *str) {
  size_t n;
  struct evalstring *p;
  struct string *res;
  n = 0;
  for (p = str; p; p = p->next) {
    if (p->var) p->str = envvar(env, p->var);
    if (p->str) n += p->str->n;
  }
  res = merge(str, n);
  delevalstr(str);
  return res;
}
void envaddrule(struct environment *env, struct rule *r) {
  if (treeinsert(&env->rules, r->name, r)) fatal("rule '%s' redefined", r->name);
}
struct rule *envrule(struct environment *env, char *name) {
  struct treenode *n;
  do {
    n = treefind(env->rules, name);
    if (n) return n->value;
    env = env->parent;
  } while (env);
  return NULL;
}
static struct string *pathlist(struct node **nodes, size_t n, char sep, bool escape) {
  size_t i, len;
  struct string *path, *result;
  char *s;
  if (n == 0) return NULL;
  if (n == 1) return nodepath(nodes[0], escape);
  for (i = 0, len = 0; i < n; ++i)
    len += nodepath(nodes[i], escape)->n;
  result = mkstr(len + n - 1);
  s = result->s;
  for (i = 0; i < n; ++i) {
    path = nodepath(nodes[i], escape);
    memcpy(s, path->s, path->n);
    s += path->n;
    *s++ = sep;
  }
  *--s = '\0';
  return result;
}
struct rule *mkrule(char *name) {
  struct rule *r;
  r = xmalloc(sizeof(*r));
  r->name = name;
  r->bindings = NULL;
  return r;
}
static void delrule(void *ptr) {
  struct rule *r = ptr;
  if (r == &phonyrule) return;
  deltree(r->bindings, free, delevalstr);
  free(r->name);
  free(r);
}
void ruleaddvar(struct rule *r, char *var, struct evalstring *val) {
  addvar(&r->bindings, var, val);
}
struct string *edgevar(struct edge *e, char *var, bool escape) {
  static void *const cycle = (void *)&cycle;
  struct evalstring *str, *p;
  struct treenode *n;
  size_t len;
  if (strcmp(var, "in") == 0) return pathlist(e->in, e->inimpidx, ' ', escape);
  if (strcmp(var, "in_newline") == 0) return pathlist(e->in, e->inimpidx, '\n', escape);
  if (strcmp(var, "out") == 0) return pathlist(e->out, e->outimpidx, ' ', escape);
  n = treefind(e->env->bindings, var);
  if (n) return n->value;
  n = treefind(e->rule->bindings, var);
  if (!n) return envvar(e->env->parent, var);
  if (n->value == cycle) fatal("cycle in rule variable involving '%s'", var);
  str = n->value;
  n->value = cycle;
  len = 0;
  for (p = str; p; p = p->next) {
    if (p->var) p->str = edgevar(e, p->var, escape);
    if (p->str) len += p->str->n;
  }
  n->value = str;
  return merge(str, len);
}
static void addpool(struct pool *p) {
  if (treeinsert(&pools, p->name, p)) fatal("pool '%s' redefined", p->name);
}
struct pool *mkpool(char *name) {
  struct pool *p;
  p = xmalloc(sizeof(*p));
  p->name = name;
  p->numjobs = 0;
  p->maxjobs = 0;
  p->work = NULL;
  addpool(p);
  return p;
}
static void delpool(void *ptr) {
  struct pool *p = ptr;
  if (p == &consolepool) return;
  free(p->name);
  free(p);
}
struct pool *poolget(char *name) {
  struct treenode *n;
  n = treefind(pools, name);
  if (!n) fatal("unknown pool '%s'", name);
  return n->value;
}
static struct hashtable *allnodes;
struct edge *alledges;
static void delnode(void *p) {
  struct node *n = p;
  if (n->shellpath != n->path) free(n->shellpath);
  free(n->use);
  free(n->path);
  free(n);
}
void graphinit(void) {
  struct edge *e;
  delhtab(allnodes, delnode);
  while (alledges) {
    e = alledges;
    alledges = e->allnext;
    free(e->out);
    free(e->in);
    free(e);
  }
  allnodes = mkhtab(1024);
}
struct node *mknode(struct string *path) {
  void **v;
  struct node *n;
  struct hashtablekey k;
  htabkey(&k, path->s, path->n);
  v = htabput(allnodes, &k);
  if (*v) {
    free(path);
    return *v;
  }
  n = xmalloc(sizeof(*n));
  n->path = path;
  n->shellpath = NULL;
  n->gen = NULL;
  n->use = NULL;
  n->nuse = 0;
  n->mtime = MTIME_UNKNOWN;
  n->logmtime = MTIME_MISSING;
  n->hash = 0;
  n->id = -1;
  *v = n;
  return n;
}
struct node *nodeget(const char *path, size_t len) {
  struct hashtablekey k;
  if (!len) len = strlen(path);
  htabkey(&k, path, len);
  return htabget(allnodes, &k);
}
void nodestat(struct node *n) {
  n->mtime = osmtime(n->path->s);
}
struct string *nodepath(struct node *n, bool escape) {
  char *s, *d;
  int nquote;
  if (!escape) return n->path;
  if (n->shellpath) return n->shellpath;
  escape = false;
  nquote = 0;
  for (s = n->path->s; *s; ++s) {
    if (!isalnum(*(unsigned char *)s) && !strchr("_+-./", *s)) escape = true;
    if (*s == '\'') ++nquote;
  }
  if (escape) {
    n->shellpath = mkstr(n->path->n + 2 + 3 * nquote);
    d = n->shellpath->s;
    *d++ = '\'';
    for (s = n->path->s; *s; ++s) {
      *d++ = *s;
      if (*s == '\'') {
        *d++ = '\\';
        *d++ = '\'';
        *d++ = '\'';
      }
    }
    *d++ = '\'';
  } else {
    n->shellpath = n->path;
  }
  return n->shellpath;
}
void nodeuse(struct node *n, struct edge *e) {
  if (!(n->nuse & (n->nuse - 1))) n->use = xreallocarray(n->use, n->nuse ? n->nuse * 2 : 1, sizeof(e));
  n->use[n->nuse++] = e;
}
struct edge *mkedge(struct environment *parent) {
  struct edge *e;
  e = xmalloc(sizeof(*e));
  e->env = mkenv(parent);
  e->pool = NULL;
  e->out = NULL;
  e->nout = 0;
  e->in = NULL;
  e->nin = 0;
  e->flags = 0;
  e->allnext = alledges;
  alledges = e;
  return e;
}
void edgehash(struct edge *e) {
  static const char sep[] = ";rspfile=";
  struct string *cmd, *rsp, *s;
  if (e->flags & FLAG_HASH) return;
  e->flags |= FLAG_HASH;
  cmd = edgevar(e, "command", true);
  if (!cmd) fatal("rule '%s' has no command", e->rule->name);
  rsp = edgevar(e, "rspfile_content", true);
  if (rsp && rsp->n > 0) {
    s = mkstr(cmd->n + sizeof(sep) - 1 + rsp->n);
    memcpy(s->s, cmd->s, cmd->n);
    memcpy(s->s + cmd->n, sep, sizeof(sep) - 1);
    memcpy(s->s + cmd->n + sizeof(sep) - 1, rsp->s, rsp->n);
    s->s[s->n] = '\0';
    e->hash = murmurhash64a(s->s, s->n);
    free(s);
  } else {
    e->hash = murmurhash64a(cmd->s, cmd->n);
  }
}
static struct edge *mkphony(struct node *n) {
  struct edge *e;
  e = mkedge(rootenv);
  e->rule = &phonyrule;
  e->inimpidx = 0;
  e->inorderidx = 0;
  e->outimpidx = 1;
  e->nout = 1;
  e->out = xmalloc(sizeof(n));
  e->out[0] = n;
  return e;
}
void edgeadddeps(struct edge *e, struct node **deps, size_t ndeps) {
  struct node **order, *n;
  size_t norder, i;
  for (i = 0; i < ndeps; ++i) {
    n = deps[i];
    if (!n->gen) n->gen = mkphony(n);
    nodeuse(n, e);
  }
  e->in = xreallocarray(e->in, e->nin + ndeps, sizeof(e->in[0]));
  order = e->in + e->inorderidx;
  norder = e->nin - e->inorderidx;
  memmove(order + ndeps, order, norder * sizeof(e->in[0]));
  memcpy(order, deps, ndeps * sizeof(e->in[0]));
  e->inorderidx += ndeps;
  e->nin += ndeps;
}
struct hashtable {
  size_t len, cap;
  struct hashtablekey *keys;
  void **vals;
};
void htabkey(struct hashtablekey *k, const char *s, size_t n) {
  k->str = s;
  k->len = n;
  k->hash = murmurhash64a(s, n);
}
struct hashtable *mkhtab(size_t cap) {
  struct hashtable *h;
  size_t i;
  assert(!(cap & (cap - 1)));
  h = xmalloc(sizeof(*h));
  h->len = 0;
  h->cap = cap;
  h->keys = xreallocarray(NULL, cap, sizeof(h->keys[0]));
  h->vals = xreallocarray(NULL, cap, sizeof(h->vals[0]));
  for (i = 0; i < cap; ++i)
    h->keys[i].str = NULL;
  return h;
}
void delhtab(struct hashtable *h, void del(void *)) {
  size_t i;
  if (!h) return;
  if (del) {
    for (i = 0; i < h->cap; ++i) {
      if (h->keys[i].str) del(h->vals[i]);
    }
  }
  free(h->keys);
  free(h->vals);
  free(h);
}
static bool keyequal(struct hashtablekey *k1, struct hashtablekey *k2) {
  if (k1->hash != k2->hash || k1->len != k2->len) return false;
  return memcmp(k1->str, k2->str, k1->len) == 0;
}
static size_t keyindex(struct hashtable *h, struct hashtablekey *k) {
  size_t i;
  i = k->hash & (h->cap - 1);
  while (h->keys[i].str && !keyequal(&h->keys[i], k))
    i = (i + 1) & (h->cap - 1);
  return i;
}
void **htabput(struct hashtable *h, struct hashtablekey *k) {
  struct hashtablekey *oldkeys;
  void **oldvals;
  size_t i, j, oldcap;
  if (h->cap / 2 < h->len) {
    oldkeys = h->keys;
    oldvals = h->vals;
    oldcap = h->cap;
    h->cap *= 2;
    h->keys = xreallocarray(NULL, h->cap, sizeof(h->keys[0]));
    h->vals = xreallocarray(NULL, h->cap, sizeof(h->vals[0]));
    for (i = 0; i < h->cap; ++i)
      h->keys[i].str = NULL;
    for (i = 0; i < oldcap; ++i) {
      if (oldkeys[i].str) {
        j = keyindex(h, &oldkeys[i]);
        h->keys[j] = oldkeys[i];
        h->vals[j] = oldvals[i];
      }
    }
    free(oldkeys);
    free(oldvals);
  }
  i = keyindex(h, k);
  if (!h->keys[i].str) {
    h->keys[i] = *k;
    h->vals[i] = NULL;
    ++h->len;
  }
  return &h->vals[i];
}
void *htabget(struct hashtable *h, struct hashtablekey *k) {
  size_t i;
  i = keyindex(h, k);
  return h->keys[i].str ? h->vals[i] : NULL;
}
uint64_t murmurhash64a(const void *ptr, size_t len) {
  const uint64_t seed = 0xdecafbaddecafbadull;
  const uint64_t m = 0xc6a4a7935bd1e995ull;
  uint64_t h, k, n;
  const uint8_t *p, *end;
  int r = 47;
  h = seed ^ (len * m);
  n = len & ~0x7ull;
  end = ptr;
  end += n;
  for (p = ptr; p != end; p += 8) {
    memcpy(&k, p, sizeof(k));
    k *= m;
    k ^= k >> r;
    k *= m;
    h ^= k;
    h *= m;
  }
  switch (len & 0x7) {
  case 7:
    h ^= (uint64_t)p[6] << 48;
  case 6:
    h ^= (uint64_t)p[5] << 40;
  case 5:
    h ^= (uint64_t)p[4] << 32;
  case 4:
    h ^= (uint64_t)p[3] << 24;
  case 3:
    h ^= (uint64_t)p[2] << 16;
  case 2:
    h ^= (uint64_t)p[1] << 8;
  case 1:
    h ^= (uint64_t)p[0];
    h *= m;
  }
  h ^= h >> r;
  h *= m;
  h ^= h >> r;
  return h;
}
static FILE *logfile;
static const char *logname = ".ninja_log";
static const char *logtmpname = ".ninja_log.tmp";
static const char *logfmt = "# ninja log v%d\n";
static const int logver = 5;
static char *nextfield(char **end) {
  char *s = *end;
  if (!*s) {
    warn("corrupt build log: missing field");
    return NULL;
  }
  *end += strcspn(*end, "\t\n");
  if (**end) *(*end)++ = '\0';
  return s;
}
void loginit(const char *builddir) {
  int ver;
  char *logpath = (char *)logname, *logtmppath = (char *)logtmpname, *p, *s;
  size_t nline, nentry, i;
  struct edge *e;
  struct node *n;
  int64_t mtime;
  struct buffer buf = {0};
  nline = 0;
  nentry = 0;
  if (logfile) fclose(logfile);
  if (builddir) xasprintf(&logpath, "%s/%s", builddir, logname);
  logfile = fopen(logpath, "r+");
  if (!logfile) {
    if (errno != ENOENT) fatal("open %s:", logpath);
    goto rewrite;
  }
  setvbuf(logfile, NULL, _IOLBF, 0);
  if (fscanf(logfile, logfmt, &ver) < 1) goto rewrite;
  if (ver != logver) goto rewrite;
  for (;;) {
    if (buf.cap - buf.len < BUFSIZ) {
      buf.cap = buf.cap ? buf.cap * 2 : BUFSIZ;
      buf.data = xreallocarray(buf.data, buf.cap, 1);
    }
    buf.data[buf.cap - 2] = '\0';
    if (!fgets(buf.data + buf.len, buf.cap - buf.len, logfile)) break;
    if (buf.data[buf.cap - 2] && buf.data[buf.cap - 2] != '\n') {
      buf.len = buf.cap - 1;
      continue;
    }
    ++nline;
    p = buf.data;
    buf.len = 0;
    if (!nextfield(&p)) continue;
    if (!nextfield(&p)) continue;
    s = nextfield(&p);
    if (!s) continue;
    mtime = strtoll(s, &s, 10);
    if (*s) {
      warn("corrupt build log: invalid mtime");
      continue;
    }
    s = nextfield(&p);
    if (!s) continue;
    n = nodeget(s, 0);
    if (!n || !n->gen) continue;
    if (n->logmtime == MTIME_MISSING) ++nentry;
    n->logmtime = mtime;
    s = nextfield(&p);
    if (!s) continue;
    n->hash = strtoull(s, &s, 16);
    if (*s) {
      warn("corrupt build log: invalid hash for '%s'", n->path->s);
      continue;
    }
  }
  free(buf.data);
  if (ferror(logfile)) {
    warn("build log read:");
    goto rewrite;
  }
  if (nline <= 100 || nline <= 3 * nentry) {
    if (builddir) free(logpath);
    return;
  }
rewrite:
  if (logfile) fclose(logfile);
  if (builddir) xasprintf(&logtmppath, "%s/%s", builddir, logtmpname);
  logfile = fopen(logtmppath, "w");
  if (!logfile) fatal("open %s:", logtmppath);
  setvbuf(logfile, NULL, _IOLBF, 0);
  fprintf(logfile, logfmt, logver);
  if (nentry > 0) {
    for (e = alledges; e; e = e->allnext) {
      for (i = 0; i < e->nout; ++i) {
        n = e->out[i];
        if (!n->hash) continue;
        logrecord(n);
      }
    }
  }
  fflush(logfile);
  if (ferror(logfile)) fatal("build log write failed");
  if (rename(logtmppath, logpath) < 0) fatal("build log rename:");
  if (builddir) {
    free(logpath);
    free(logtmppath);
  }
}
void logclose(void) {
  fflush(logfile);
  if (ferror(logfile)) fatal("build log write failed");
  fclose(logfile);
}
void logrecord(struct node *n) {
  fprintf(logfile, "0\t0\t%" PRId64 "\t%s\t%" PRIx64 "\n", n->logmtime, n->path->s, n->hash);
}
struct parseoptions parseopts;
static struct node **deftarg;
static size_t ndeftarg;
void parseinit(void) {
  free(deftarg);
  deftarg = NULL;
  ndeftarg = 0;
}
static void parselet(struct scanner *s, struct evalstring **val) {
  scanchar(s, '=');
  *val = scanstring(s, false);
  scannewline(s);
}
static void parserule(struct scanner *s, struct environment *env) {
  struct rule *r;
  char *var;
  struct evalstring *val;
  bool hascommand = false, hasrspfile = false, hasrspcontent = false;
  r = mkrule(scanname(s));
  scannewline(s);
  while (scanindent(s)) {
    var = scanname(s);
    parselet(s, &val);
    ruleaddvar(r, var, val);
    if (!val) continue;
    if (strcmp(var, "command") == 0) hascommand = true;
    else if (strcmp(var, "rspfile") == 0) hasrspfile = true;
    else if (strcmp(var, "rspfile_content") == 0) hasrspcontent = true;
  }
  if (!hascommand) fatal("rule '%s' has no command", r->name);
  if (hasrspfile != hasrspcontent) fatal("rule '%s' has rspfile and no rspfile_content or vice versa", r->name);
  envaddrule(env, r);
}
static void parseedge(struct scanner *s, struct environment *env) {
  struct edge *e;
  struct evalstring *str, **path;
  char *name;
  struct string *val;
  struct node *n;
  size_t i;
  int p;
  e = mkedge(env);
  scanpaths(s);
  e->outimpidx = npaths;
  if (scanpipe(s, 1)) scanpaths(s);
  e->nout = npaths;
  if (e->nout == 0) scanerror(s, "expected output path");
  scanchar(s, ':');
  name = scanname(s);
  e->rule = envrule(env, name);
  if (!e->rule) fatal("undefined rule '%s'", name);
  free(name);
  scanpaths(s);
  e->inimpidx = npaths - e->nout;
  p = scanpipe(s, 1 | 2);
  if (p == 1) {
    scanpaths(s);
    p = scanpipe(s, 2);
  }
  e->inorderidx = npaths - e->nout;
  if (p == 2) scanpaths(s);
  e->nin = npaths - e->nout;
  scannewline(s);
  while (scanindent(s)) {
    name = scanname(s);
    parselet(s, &str);
    val = enveval(env, str);
    envaddvar(e->env, name, val);
  }
  e->out = xreallocarray(NULL, e->nout, sizeof(e->out[0]));
  for (i = 0, path = paths; i < e->nout; ++path) {
    val = enveval(e->env, *path);
    canonpath(val);
    n = mknode(val);
    if (n->gen) {
      if (!parseopts.dupbuildwarn) fatal("multiple rules generate '%s'", n->path->s);
      warn("multiple rules generate '%s'", n->path->s);
      --e->nout;
      if (i < e->outimpidx) --e->outimpidx;
    } else {
      n->gen = e;
      e->out[i] = n;
      ++i;
    }
  }
  e->in = xreallocarray(NULL, e->nin, sizeof(e->in[0]));
  for (i = 0; i < e->nin; ++i, ++path) {
    val = enveval(e->env, *path);
    canonpath(val);
    n = mknode(val);
    e->in[i] = n;
    nodeuse(n, e);
  }
  npaths = 0;
  val = edgevar(e, "pool", true);
  if (val) e->pool = poolget(val->s);
}
static void parseinclude(struct scanner *s, struct environment *env, bool newscope) {
  struct evalstring *str;
  struct string *path;
  str = scanstring(s, true);
  if (!str) scanerror(s, "expected include path");
  scannewline(s);
  path = enveval(env, str);
  if (newscope) env = mkenv(env);
  parse(path->s, env);
  free(path);
}
static void parsedefault(struct scanner *s, struct environment *env) {
  struct string *path;
  struct node *n;
  size_t i;
  scanpaths(s);
  deftarg = xreallocarray(deftarg, ndeftarg + npaths, sizeof(*deftarg));
  for (i = 0; i < npaths; ++i) {
    path = enveval(env, paths[i]);
    canonpath(path);
    n = nodeget(path->s, path->n);
    if (!n) fatal("unknown target '%s'", path->s);
    free(path);
    deftarg[ndeftarg++] = n;
  }
  scannewline(s);
  npaths = 0;
}
static void parsepool(struct scanner *s, struct environment *env) {
  struct pool *p;
  struct evalstring *val;
  struct string *str;
  char *var, *end;
  p = mkpool(scanname(s));
  scannewline(s);
  while (scanindent(s)) {
    var = scanname(s);
    parselet(s, &val);
    if (strcmp(var, "depth") == 0) {
      str = enveval(env, val);
      p->maxjobs = strtol(str->s, &end, 10);
      if (*end) fatal("invalid pool depth '%s'", str->s);
      free(str);
    } else {
      fatal("unexpected pool variable '%s'", var);
    }
  }
  if (!p->maxjobs) fatal("pool '%s' has no depth", p->name);
}
static void checkversion(const char *ver) {
  int major, minor = 0;
  if (sscanf(ver, "%d.%d", &major, &minor) < 1) fatal("invalid ninja_required_version");
  if (major > ninjamajor || (major == ninjamajor && minor > ninjaminor)) fatal("ninja_required_version %s is newer than %d.%d", ver, ninjamajor, ninjaminor);
}
void parse(const char *name, struct environment *env) {
  struct scanner s;
  char *var;
  struct string *val;
  struct evalstring *str;
  scaninit(&s, name);
  for (;;) {
    switch (scankeyword(&s, &var)) {
    case RULE:
      parserule(&s, env);
      break;
    case BUILD:
      parseedge(&s, env);
      break;
    case INCLUDE:
      parseinclude(&s, env, false);
      break;
    case SUBNINJA:
      parseinclude(&s, env, true);
      break;
    case DEFAULT:
      parsedefault(&s, env);
      break;
    case POOL:
      parsepool(&s, env);
      break;
    case VARIABLE:
      parselet(&s, &str);
      val = enveval(env, str);
      if (strcmp(var, "ninja_required_version") == 0) checkversion(val->s);
      envaddvar(env, var, val);
      break;
    case EOF:
      scanclose(&s);
      return;
    }
  }
}
void defaultnodes(void fn(struct node *)) {
  struct edge *e;
  struct node *n;
  size_t i;
  if (ndeftarg > 0) {
    for (i = 0; i < ndeftarg; ++i)
      fn(deftarg[i]);
  } else {
    for (e = alledges; e; e = e->allnext) {
      for (i = 0; i < e->nout; ++i) {
        n = e->out[i];
        if (n->nuse == 0) fn(n);
      }
    }
  }
}
const char *argv0;
static void usage(void) {
  fprintf(stderr, "usage: %s [-C dir] [-f buildfile] [-j maxjobs] [-k maxfail] [-l maxload] [-n]\n", argv0);
  exit(2);
}
static char *getbuilddir(void) {
  struct string *builddir;
  builddir = envvar(rootenv, "builddir");
  if (!builddir) return NULL;
  if (osmkdirs(builddir, false) < 0) exit(1);
  return builddir->s;
}
static void debugflag(const char *flag) {
  if (strcmp(flag, "explain") == 0) buildopts.explain = true;
  else if (strcmp(flag, "keepdepfile") == 0) buildopts.keepdepfile = true;
  else if (strcmp(flag, "keeprsp") == 0) buildopts.keeprsp = true;
  else fatal("unknown debug flag '%s'", flag);
}
static void loadflag(const char *flag) {
#ifdef HAVE_GETLOADAVG
  double value;
  char *end;
  errno = 0;
  value = strtod(flag, &end);
  if (*end || value < 0 || errno != 0) fatal("invalid -l parameter");
  buildopts.maxload = value;
#else
  warn("job scheduling based on load average is not supported");
#endif
}
static void warnflag(const char *flag) {
  if (strcmp(flag, "dupbuild=err") == 0) parseopts.dupbuildwarn = false;
  else if (strcmp(flag, "dupbuild=warn") == 0) parseopts.dupbuildwarn = true;
  else fatal("unknown warning flag '%s'", flag);
}
static void jobsflag(const char *flag) {
  long num;
  char *end;
  num = strtol(flag, &end, 10);
  if (*end || num < 0) fatal("invalid -j parameter");
  buildopts.maxjobs = num > 0 ? num : -1;
}
static void parseenvargs(char *env) {
  char *arg, *argvbuf[64], **argv = argvbuf;
  int argc;
  if (!env) return;
  env = xmemdup(env, strlen(env) + 1);
  argc = 1;
  argv[0] = NULL;
  arg = strtok(env, " ");
  while (arg) {
    if ((size_t)argc >= LEN(argvbuf) - 1) fatal("too many arguments in SAMUFLAGS");
    argv[argc++] = arg;
    arg = strtok(NULL, " ");
  }
  argv[argc] = NULL;
  ARGBEGIN {
  case 'j':
    jobsflag(EARGF(usage()));
    break;
  case 'v':
    buildopts.verbose = true;
    break;
  case 'l':
    loadflag(EARGF(usage()));
    break;
  default:
    fatal("invalid option in SAMUFLAGS");
  }
  ARGEND
  free(env);
}
static const char *progname(const char *arg, const char *def) {
  const char *slash;
  if (!arg) return def;
  slash = strrchr(arg, '/');
  return slash ? slash + 1 : arg;
}
int main(int argc, char *argv[]) {
  char *builddir, *manifest = "build.ninja", *end, *arg;
  const struct tool *tool = NULL;
  struct node *n;
  long num;
  int tries;
  argv0 = progname(argv[0], "samu");
  parseenvargs(getenv("SAMUFLAGS"));
  ARGBEGIN {
  case '-':
    arg = EARGF(usage());
    if (strcmp(arg, "version") == 0) {
      printf("%d.%d.0\n", ninjamajor, ninjaminor);
      return 0;
    } else if (strcmp(arg, "verbose") == 0) {
      buildopts.verbose = true;
    } else {
      usage();
    }
    break;
  case 'C':
    arg = EARGF(usage());
    warn("entering directory '%s'", arg);
    oschdir(arg);
    break;
  case 'd':
    debugflag(EARGF(usage()));
    break;
  case 'f':
    manifest = EARGF(usage());
    break;
  case 'j':
    jobsflag(EARGF(usage()));
    break;
  case 'k':
    num = strtol(EARGF(usage()), &end, 10);
    if (*end) fatal("invalid -k parameter");
    buildopts.maxfail = num > 0 ? num : -1;
    break;
  case 'l':
    loadflag(EARGF(usage()));
    break;
  case 'n':
    buildopts.dryrun = true;
    break;
  case 't':
    tool = toolget(EARGF(usage()));
    goto argdone;
  case 'v':
    buildopts.verbose = true;
    break;
  case 'w':
    warnflag(EARGF(usage()));
    break;
  default:
    usage();
  }
  ARGEND
argdone:
  if (!buildopts.maxjobs) {
#ifdef _SC_NPROCESSORS_ONLN
    long nproc = sysconf(_SC_NPROCESSORS_ONLN);
    switch (nproc) {
    case -1:
    case 0:
    case 1:
      buildopts.maxjobs = 2;
      break;
    case 2:
      buildopts.maxjobs = 3;
      break;
    default:
      buildopts.maxjobs = nproc + 2;
      break;
    }
#else
    buildopts.maxjobs = 2;
#endif
  }
  buildopts.statusfmt = getenv("NINJA_STATUS");
  if (!buildopts.statusfmt) buildopts.statusfmt = "[%s/%t] ";
  setvbuf(stdout, NULL, _IOLBF, 0);
  tries = 0;
retry:
  graphinit();
  envinit();
  parseinit();
  parse(manifest, rootenv);
  if (tool) return tool->run(argc, argv);
  builddir = getbuilddir();
  loginit(builddir);
  depsinit(builddir);
  n = nodeget(manifest, 0);
  if (n && n->gen) {
    buildadd(n);
    if (n->dirty) {
      build();
      if (n->gen->flags & FLAG_DIRTY_OUT || n->gen->nprune > 0) {
        if (++tries > 100) fatal("manifest '%s' dirty after 100 tries", manifest);
        if (!buildopts.dryrun) goto retry;
      }
      buildreset();
    }
  }
  if (argc) {
    for (; *argv; ++argv) {
      n = nodeget(*argv, 0);
      if (!n) fatal("unknown target '%s'", *argv);
      buildadd(n);
    }
  } else {
    defaultnodes(buildadd);
  }
  build();
  logclose();
  depsclose();
  return 0;
}
struct evalstring **paths;
size_t npaths;
static struct buffer buf;
void scaninit(struct scanner *s, const char *path) {
  s->path = path;
  s->line = 1;
  s->col = 1;
  s->f = fopen(path, "r");
  if (!s->f) fatal("open %s:", path);
  s->chr = getc(s->f);
}
void scanclose(struct scanner *s) {
  fclose(s->f);
}
void scanerror(struct scanner *s, const char *fmt, ...) {
  extern const char *argv0;
  va_list ap;
  fprintf(stderr, "%s: %s:%d:%d: ", argv0, s->path, s->line, s->col);
  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  va_end(ap);
  putc('\n', stderr);
  exit(1);
}
static int next(struct scanner *s) {
  if (s->chr == '\n') {
    ++s->line;
    s->col = 1;
  } else {
    ++s->col;
  }
  s->chr = getc(s->f);
  return s->chr;
}
static int issimplevar(int c) {
  return isalnum(c) || c == '_' || c == '-';
}
static int isvar(int c) {
  return issimplevar(c) || c == '.';
}
static bool newline(struct scanner *s) {
  switch (s->chr) {
  case '\r':
    if (next(s) != '\n') scanerror(s, "expected '\\n' after '\\r'");
  case '\n':
    next(s);
    return true;
  }
  return false;
}
static bool singlespace(struct scanner *s) {
  switch (s->chr) {
  case '$':
    next(s);
    if (newline(s)) return true;
    ungetc(s->chr, s->f);
    s->chr = '$';
    return false;
  case ' ':
    next(s);
    return true;
  }
  return false;
}
static bool space(struct scanner *s) {
  if (!singlespace(s)) return false;
  while (singlespace(s))
    ;
  return true;
}
static bool comment(struct scanner *s) {
  if (s->chr != '#') return false;
  do
    next(s);
  while (!newline(s));
  return true;
}
static void name(struct scanner *s) {
  buf.len = 0;
  while (isvar(s->chr)) {
    bufadd(&buf, s->chr);
    next(s);
  }
  if (!buf.len) scanerror(s, "expected name");
  bufadd(&buf, '\0');
  space(s);
}
int scankeyword(struct scanner *s, char **var) {
  static const struct {
    const char *name;
    int value;
  } keywords[] = {
      {"build", BUILD},
      {"default", DEFAULT},
      {"include", INCLUDE},
      {"pool", POOL},
      {"rule", RULE},
      {"subninja", SUBNINJA},
  };
  int low = 0, high = LEN(keywords) - 1, mid, cmp;
  for (;;) {
    switch (s->chr) {
    case ' ':
      space(s);
      if (!comment(s) && !newline(s)) scanerror(s, "unexpected indent");
      break;
    case '#':
      comment(s);
      break;
    case '\r':
    case '\n':
      newline(s);
      break;
    case EOF:
      return EOF;
    default:
      name(s);
      while (low <= high) {
        mid = (low + high) / 2;
        cmp = strcmp(buf.data, keywords[mid].name);
        if (cmp == 0) return keywords[mid].value;
        if (cmp < 0) high = mid - 1;
        else low = mid + 1;
      }
      *var = xmemdup(buf.data, buf.len);
      return VARIABLE;
    }
  }
}
char *scanname(struct scanner *s) {
  name(s);
  return xmemdup(buf.data, buf.len);
}
static void addstringpart(struct evalstring ***end, bool var) {
  struct evalstring *p;
  p = xmalloc(sizeof(*p));
  p->next = NULL;
  **end = p;
  if (var) {
    bufadd(&buf, '\0');
    p->var = xmemdup(buf.data, buf.len);
  } else {
    p->var = NULL;
    p->str = mkstr(buf.len);
    memcpy(p->str->s, buf.data, buf.len);
    p->str->s[buf.len] = '\0';
  }
  *end = &p->next;
  buf.len = 0;
}
static void escape(struct scanner *s, struct evalstring ***end) {
  switch (s->chr) {
  case '$':
  case ' ':
  case ':':
    bufadd(&buf, s->chr);
    next(s);
    break;
  case '{':
    if (buf.len > 0) addstringpart(end, false);
    while (isvar(next(s)))
      bufadd(&buf, s->chr);
    if (s->chr != '}') scanerror(s, "invalid variable name");
    next(s);
    addstringpart(end, true);
    break;
  case '\r':
  case '\n':
    newline(s);
    space(s);
    break;
  default:
    if (buf.len > 0) addstringpart(end, false);
    while (issimplevar(s->chr)) {
      bufadd(&buf, s->chr);
      next(s);
    }
    if (!buf.len) scanerror(s, "invalid $ escape");
    addstringpart(end, true);
  }
}
struct evalstring *scanstring(struct scanner *s, bool path) {
  struct evalstring *str = NULL, **end = &str;
  buf.len = 0;
  for (;;) {
    switch (s->chr) {
    case '$':
      next(s);
      escape(s, &end);
      break;
    case ':':
    case '|':
    case ' ':
      if (path) goto out;
    default:
      bufadd(&buf, s->chr);
      next(s);
      break;
    case '\r':
    case '\n':
    case EOF:
      goto out;
    }
  }
out:
  if (buf.len > 0) addstringpart(&end, 0);
  if (path) space(s);
  return str;
}
void scanpaths(struct scanner *s) {
  static size_t max;
  struct evalstring *str;
  while ((str = scanstring(s, true))) {
    if (npaths == max) {
      max = max ? max * 2 : 32;
      paths = xreallocarray(paths, max, sizeof(paths[0]));
    }
    paths[npaths++] = str;
  }
}
void scanchar(struct scanner *s, int c) {
  if (s->chr != c) scanerror(s, "expected '%c'", c);
  next(s);
  space(s);
}
int scanpipe(struct scanner *s, int n) {
  if (s->chr != '|') return 0;
  next(s);
  if (s->chr != '|') {
    if (!(n & 1)) scanerror(s, "expected '||'");
    space(s);
    return 1;
  }
  if (!(n & 2)) scanerror(s, "unexpected '||'");
  next(s);
  space(s);
  return 2;
}
bool scanindent(struct scanner *s) {
  bool indent;
  for (;;) {
    indent = space(s);
    if (!comment(s)) return indent && !newline(s);
  }
}
void scannewline(struct scanner *s) {
  if (!newline(s)) scanerror(s, "expected newline");
}
static int cleanpath(struct string *path) {
  if (path) {
    if (remove(path->s) == 0) {
      printf("remove %s\n", path->s);
    } else if (errno != ENOENT) {
      warn("remove %s:", path->s);
      return -1;
    }
  }
  return 0;
}
static int cleanedge(struct edge *e) {
  int ret = 0;
  size_t i;
  for (i = 0; i < e->nout; ++i) {
    if (cleanpath(e->out[i]->path) < 0) ret = -1;
  }
  if (cleanpath(edgevar(e, "rspfile", false)) < 0) ret = -1;
  if (cleanpath(edgevar(e, "depfile", false)) < 0) ret = -1;
  return ret;
}
static int cleantarget(struct node *n) {
  int ret = 0;
  size_t i;
  if (!n->gen || n->gen->rule == &phonyrule) return 0;
  if (cleanpath(n->path) < 0) ret = -1;
  for (i = 0; i < n->gen->nin; ++i) {
    if (cleantarget(n->gen->in[i]) < 0) ret = -1;
  }
  return ret;
}
static int clean(int argc, char *argv[]) {
  int ret = 0;
  bool cleangen = false, cleanrule = false;
  struct edge *e;
  struct node *n;
  struct rule *r;
  ARGBEGIN {
  case 'g':
    cleangen = true;
    break;
  case 'r':
    cleanrule = true;
    break;
  default:
    fprintf(stderr, "usage: %s ... -t clean [-gr] [targets...]\n", argv0);
    return 2;
  }
  ARGEND
  if (cleanrule) {
    if (!argc) fatal("expected a rule to clean");
    for (; *argv; ++argv) {
      r = envrule(rootenv, *argv);
      if (!r) {
        warn("unknown rule '%s'", *argv);
        ret = 1;
        continue;
      }
      for (e = alledges; e; e = e->allnext) {
        if (e->rule != r) continue;
        if (cleanedge(e) < 0) ret = 1;
      }
    }
  } else if (argc > 0) {
    for (; *argv; ++argv) {
      n = nodeget(*argv, 0);
      if (!n) {
        warn("unknown target '%s'", *argv);
        ret = 1;
        continue;
      }
      if (cleantarget(n) < 0) ret = 1;
    }
  } else {
    for (e = alledges; e; e = e->allnext) {
      if (e->rule == &phonyrule) continue;
      if (!cleangen && edgevar(e, "generator", true)) continue;
      if (cleanedge(e) < 0) ret = 1;
    }
  }
  return ret;
}
static void targetcommands(struct node *n) {
  struct edge *e = n->gen;
  struct string *command;
  size_t i;
  if (!e || (e->flags & FLAG_WORK)) return;
  e->flags |= FLAG_WORK;
  for (i = 0; i < e->nin; ++i)
    targetcommands(e->in[i]);
  command = edgevar(e, "command", true);
  if (command && command->n) puts(command->s);
}
static int commands(int argc, char *argv[]) {
  struct node *n;
  if (argc > 1) {
    while (*++argv) {
      n = nodeget(*argv, 0);
      if (!n) fatal("unknown target '%s'", *argv);
      targetcommands(n);
    }
  } else {
    defaultnodes(targetcommands);
  }
  if (fflush(stdout) || ferror(stdout)) fatal("write failed");
  return 0;
}
static void printquoted(const char *s, size_t n, bool join) {
  size_t i;
  char c;
  for (i = 0; i < n; ++i) {
    c = s[i];
    switch (c) {
    case '"':
    case '\\':
      putchar('\\');
      break;
    case '\n':
      if (join) c = ' ';
      break;
    case '\0':
      return;
    }
    putchar(c);
  }
}
static int compdb(int argc, char *argv[]) {
  char dir[1024], *p;
  struct edge *e;
  struct string *cmd, *rspfile, *content;
  bool expandrsp = false, first = true;
  int i;
  size_t off;
  ARGBEGIN {
  case 'x':
    expandrsp = true;
    break;
  default:
    fprintf(stderr, "usage: %s ... -t compdb [-x] [rules...]\n", argv0);
    return 2;
  }
  ARGEND
  osgetcwd(dir, sizeof(dir));
  putchar('[');
  for (e = alledges; e; e = e->allnext) {
    if (e->nin == 0) continue;
    for (i = 0; i < argc; ++i) {
      if (strcmp(e->rule->name, argv[i]) == 0) {
        if (first) first = false;
        else putchar(',');
        printf("\n  {\n    \"directory\": \"");
        printquoted(dir, -1, false);
        printf("\",\n    \"command\": \"");
        cmd = edgevar(e, "command", true);
        rspfile = expandrsp ? edgevar(e, "rspfile", true) : NULL;
        p = rspfile ? strstr(cmd->s, rspfile->s) : NULL;
        if (!p || p == cmd->s || p[-1] != '@') {
          printquoted(cmd->s, cmd->n, false);
        } else {
          off = p - cmd->s;
          printquoted(cmd->s, off - 1, false);
          content = edgevar(e, "rspfile_content", true);
          printquoted(content->s, content->n, true);
          off += rspfile->n;
          printquoted(cmd->s + off, cmd->n - off, false);
        }
        printf("\",\n    \"file\": \"");
        printquoted(e->in[0]->path->s, -1, false);
        printf("\",\n    \"output\": \"");
        printquoted(e->out[0]->path->s, -1, false);
        printf("\"\n  }");
        break;
      }
    }
  }
  puts("\n]");
  if (fflush(stdout) || ferror(stdout)) fatal("write failed");
  return 0;
}
static void graphnode(struct node *n) {
  struct edge *e = n->gen;
  size_t i;
  const char *style;
  printf("\"%p\" [label=\"", (void *)n);
  printquoted(n->path->s, n->path->n, false);
  printf("\"]\n");
  if (!e || (e->flags & FLAG_WORK)) return;
  e->flags |= FLAG_WORK;
  for (i = 0; i < e->nin; ++i)
    graphnode(e->in[i]);
  if (e->nin == 1 && e->nout == 1) {
    printf("\"%p\" -> \"%p\" [label=\"%s\"]\n", (void *)e->in[0], (void *)e->out[0], e->rule->name);
  } else {
    printf("\"%p\" [label=\"%s\", shape=ellipse]\n", (void *)e, e->rule->name);
    for (i = 0; i < e->nout; ++i)
      printf("\"%p\" -> \"%p\"\n", (void *)e, (void *)e->out[i]);
    for (i = 0; i < e->nin; ++i) {
      style = i >= e->inorderidx ? " style=dotted" : "";
      printf("\"%p\" -> \"%p\" [arrowhead=none%s]\n", (void *)e->in[i], (void *)e, style);
    }
  }
}
static int graph(int argc, char *argv[]) {
  struct node *n;
  puts("digraph ninja {");
  puts("rankdir=\"LR\"");
  puts("node [fontsize=10, shape=box, height=0.25]");
  puts("edge [fontsize=10]");
  if (argc > 1) {
    while (*++argv) {
      n = nodeget(*argv, 0);
      if (!n) fatal("unknown target '%s'", *argv);
      graphnode(n);
    }
  } else {
    defaultnodes(graphnode);
  }
  puts("}");
  if (fflush(stdout) || ferror(stdout)) fatal("write failed");
  return 0;
}
static int query(int argc, char *argv[]) {
  struct node *n;
  struct edge *e;
  char *path;
  int i;
  size_t j, k;
  if (argc == 1) {
    fprintf(stderr, "usage: %s ... -t query target...\n", argv0);
    exit(2);
  }
  for (i = 1; i < argc; ++i) {
    path = argv[i];
    n = nodeget(path, 0);
    if (!n) fatal("unknown target '%s'", path);
    printf("%s:\n", argv[i]);
    e = n->gen;
    if (e) {
      printf("  input: %s\n", e->rule->name);
      for (j = 0; j < e->nin; ++j)
        printf("    %s\n", e->in[j]->path->s);
    }
    puts("  outputs:");
    for (j = 0; j < n->nuse; ++j) {
      e = n->use[j];
      for (k = 0; k < e->nout; ++k)
        printf("    %s\n", e->out[k]->path->s);
    }
  }
  return 0;
}
static void targetsdepth(struct node *n, size_t depth, size_t indent) {
  struct edge *e = n->gen;
  size_t i;
  for (i = 0; i < indent; ++i)
    printf("  ");
  if (e) {
    printf("%s: %s\n", n->path->s, e->rule->name);
    if (depth != 1) {
      for (i = 0; i < e->nin; ++i)
        targetsdepth(e->in[i], depth - 1, indent + 1);
    }
  } else {
    puts(n->path->s);
  }
}
static void targetsusage(void) {
  fprintf(stderr,
          "usage: %s ... -t targets [depth [maxdepth]]\n"
          "       %s ... -t targets rule [rulename]\n"
          "       %s ... -t targets all\n",
          argv0,
          argv0,
          argv0);
  exit(2);
}
static int targets(int argc, char *argv[]) {
  struct edge *e;
  size_t depth = 1, i;
  char *end, *mode, *name;
  if (argc > 3) targetsusage();
  mode = argv[1];
  if (!mode || strcmp(mode, "depth") == 0) {
    if (argc == 3) {
      depth = strtol(argv[2], &end, 10);
      if (*end) targetsusage();
    }
    for (e = alledges; e; e = e->allnext) {
      for (i = 0; i < e->nout; ++i) {
        if (e->out[i]->nuse == 0) targetsdepth(e->out[i], depth, 0);
      }
    }
  } else if (strcmp(mode, "rule") == 0) {
    name = argv[2];
    for (e = alledges; e; e = e->allnext) {
      if (!name) {
        for (i = 0; i < e->nin; ++i) {
          if (!e->in[i]->gen) puts(e->in[i]->path->s);
        }
      } else if (strcmp(e->rule->name, name) == 0) {
        for (i = 0; i < e->nout; ++i)
          puts(e->out[i]->path->s);
      }
    }
  } else if (strcmp(mode, "all") == 0 && argc == 2) {
    for (e = alledges; e; e = e->allnext) {
      for (i = 0; i < e->nout; ++i)
        printf("%s: %s\n", e->out[i]->path->s, e->rule->name);
    }
  } else {
    targetsusage();
  }
  if (fflush(stdout) || ferror(stdout)) fatal("write failed");
  return 0;
}
static const struct tool tools[] = {
    {"clean", clean},
    {"commands", commands},
    {"compdb", compdb},
    {"graph", graph},
    {"query", query},
    {"targets", targets},
};
const struct tool *toolget(const char *name) {
  const struct tool *t;
  size_t i;
  t = NULL;
  for (i = 0; i < LEN(tools); ++i) {
    if (strcmp(name, tools[i].name) == 0) {
      t = &tools[i];
      break;
    }
  }
  if (!t) fatal("unknown tool '%s'", name);
  return t;
}
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
extern const char *argv0;
static void vwarn(const char *fmt, va_list ap) {
  fprintf(stderr, "%s: ", argv0);
  vfprintf(stderr, fmt, ap);
  if (fmt[0] && fmt[strlen(fmt) - 1] == ':') {
    putc(' ', stderr);
    perror(NULL);
  } else {
    putc('\n', stderr);
  }
}
void warn(const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vwarn(fmt, ap);
  va_end(ap);
}
void fatal(const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vwarn(fmt, ap);
  va_end(ap);
  exit(1);
}
void *xmalloc(size_t n) {
  void *p;
  p = malloc(n);
  if (!p) fatal("malloc:");
  return p;
}
static void *reallocarray_(void *p, size_t n, size_t m) {
  if (m && n > SIZE_MAX / m) {
    errno = ENOMEM;
    return NULL;
  }
  return realloc(p, n * m);
}
void *xreallocarray(void *p, size_t n, size_t m) {
  p = reallocarray_(p, n, m);
  if (!p) fatal("reallocarray:");
  return p;
}
char *xmemdup(const char *s, size_t n) {
  char *p;
  p = xmalloc(n);
  memcpy(p, s, n);
  return p;
}
int xasprintf(char **s, const char *fmt, ...) {
  va_list ap;
  int ret;
  size_t n;
  va_start(ap, fmt);
  ret = vsnprintf(NULL, 0, fmt, ap);
  va_end(ap);
  if (ret < 0) fatal("vsnprintf:");
  n = ret + 1;
  *s = xmalloc(n);
  va_start(ap, fmt);
  ret = vsnprintf(*s, n, fmt, ap);
  va_end(ap);
  if (ret < 0 || (size_t)ret >= n) fatal("vsnprintf:");
  return ret;
}
void bufadd(struct buffer *buf, char c) {
  if (buf->len >= buf->cap) {
    buf->cap = buf->cap ? buf->cap * 2 : 1 << 8;
    buf->data = realloc(buf->data, buf->cap);
    if (!buf->data) fatal("realloc:");
  }
  buf->data[buf->len++] = c;
}
struct string *mkstr(size_t n) {
  struct string *str;
  str = xmalloc(sizeof(*str) + n + 1);
  str->n = n;
  return str;
}
void delevalstr(void *ptr) {
  struct evalstring *str = ptr, *p;
  while (str) {
    p = str;
    str = str->next;
    if (p->var) free(p->var);
    else free(p->str);
    free(p);
  }
}
void canonpath(struct string *path) {
  char *component[60];
  int n;
  char *s, *d, *end;
  if (path->n == 0) fatal("empty path");
  s = d = path->s;
  end = path->s + path->n;
  n = 0;
  if (*s == '/') {
    ++s;
    ++d;
  }
  while (s < end) {
    switch (s[0]) {
    case '/':
      ++s;
      continue;
    case '.':
      switch (s[1]) {
      case '\0':
      case '/':
        s += 2;
        continue;
      case '.':
        if (s[2] != '/' && s[2] != '\0') break;
        if (n > 0) {
          d = component[--n];
        } else {
          *d++ = s[0];
          *d++ = s[1];
          *d++ = s[2];
        }
        s += 3;
        continue;
      }
    }
    if (n == LEN(component)) fatal("path has too many components: %s", path->s);
    component[n++] = d;
    while (*s != '/' && *s != '\0')
      *d++ = *s++;
    *d++ = *s++;
  }
  if (d == path->s) {
    *d++ = '.';
    *d = '\0';
  } else {
    *--d = '\0';
  }
  path->n = d - path->s;
}
int writefile(const char *name, struct string *s) {
  FILE *f;
  int ret;
  f = fopen(name, "w");
  if (!f) {
    warn("open %s:", name);
    return -1;
  }
  ret = 0;
  if (s && (fwrite(s->s, 1, s->n, f) != s->n || fflush(f) != 0)) {
    warn("write %s:", name);
    ret = -1;
  }
  fclose(f);
  return ret;
}
void osgetcwd(char *buf, size_t len) {
  if (!getcwd(buf, len)) fatal("getcwd:");
}
void oschdir(const char *dir) {
  if (chdir(dir) < 0) fatal("chdir %s:", dir);
}
int osmkdirs(struct string *path, bool parent) {
  int ret;
  struct stat st;
  char *s, *end;
  ret = 0;
  end = path->s + path->n;
  for (s = end - parent; s > path->s; --s) {
    if (*s != '/' && *s) continue;
    *s = '\0';
    if (stat(path->s, &st) == 0) break;
    if (errno != ENOENT) {
      warn("stat %s:", path->s);
      ret = -1;
      break;
    }
  }
  if (s > path->s && s < end) *s = '/';
  while (++s <= end - parent) {
    if (*s != '\0') continue;
    if (ret == 0 && mkdir(path->s, 0777) < 0 && errno != EEXIST) {
      warn("mkdir %s:", path->s);
      ret = -1;
    }
    if (s < end) *s = '/';
  }
  return ret;
}
int64_t osmtime(const char *name) {
  struct stat st;
  if (stat(name, &st) < 0) {
    if (errno != ENOENT) fatal("stat %s:", name);
    return MTIME_MISSING;
  } else {
#ifdef __APPLE__
    return (int64_t)st.st_mtime * 1000000000 + st.st_mtimensec;
#elif defined(__sun)
    return (int64_t)st.st_mtim.__tv_sec * 1000000000 + st.st_mtim.__tv_nsec;
#else
    return (int64_t)st.st_mtim.tv_sec * 1000000000 + st.st_mtim.tv_nsec;
#endif
  }
}
