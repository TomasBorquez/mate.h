#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "graph.h"
#include "log.h"
#include "util.h"
// --- INCLUDES END ---

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
