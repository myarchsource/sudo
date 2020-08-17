/*
 * SPDX-License-Identifier: ISC
 *
 * Copyright (c) 2009-2020 Todd C. Miller <Todd.Miller@sudo.ws>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef SUDO_IOLOG_H
#define SUDO_IOLOG_H

#include <sys/types.h>	/* for gid_t, mode_t, size_t, ssize_t, uid_t */

#ifdef HAVE_ZLIB_H
# include <zlib.h>	/* for gzFile */
#endif

/* Default maximum session ID */
#define SESSID_MAX	2176782336U

/*
 * I/O log event types as stored as the first field in the timing file.
 * Changing existing values will result in incompatible I/O log files.
 */
#define IO_EVENT_STDIN		0
#define IO_EVENT_STDOUT		1
#define IO_EVENT_STDERR		2
#define IO_EVENT_TTYIN		3
#define IO_EVENT_TTYOUT		4
#define IO_EVENT_WINSIZE	5
#define IO_EVENT_TTYOUT_1_8_7	6
#define IO_EVENT_SUSPEND	7
#define IO_EVENT_COUNT		8

/*
 * Indexes into iolog_files[] array.
 * These must match the IO_EVENT_ defines above.
 * TODO: eliminate use of IOFD_* and IO_EVENT_* as indexes in favor of
 *       a struct containing iolog_file *s for each (and names too?).
 */
#define IOFD_STDIN	0
#define IOFD_STDOUT	1
#define IOFD_STDERR	2
#define IOFD_TTYIN	3
#define IOFD_TTYOUT	4
#define IOFD_TIMING	5
#define IOFD_MAX	6

/*
 * Info present in the I/O log file
 */
struct iolog_info {
    char *cwd;
    char *user;
    char *runas_user;
    char *runas_group;
    char *tty;
    char *cmd;
    char *host;
    struct timespec tstamp;
    int lines;
    int cols;
    uid_t runas_uid;
    gid_t runas_gid;
    char **argv;
    char **envp;
};

struct timing_closure {
    struct timespec delay;
    const char *decimal;
    struct iolog_file *iol;
    int event;
    union {
	struct {
	    int lines;
	    int cols;
	} winsize;
	size_t nbytes;
	int signo;
    } u;
};

struct iolog_file {
    bool enabled;
    bool compressed;
    bool writable;
    union {
	FILE *f;
#ifdef HAVE_ZLIB_H
	gzFile g;
#endif
	void *v;
    } fd;
};

struct iolog_path_escape {
    const char *name;
    size_t (*copy_fn)(char *, size_t, void *);
};

/* host_port.c */
bool iolog_parse_host_port(char *str, char **hostp, char **portp, bool *tlsp, char *defport, char *defport_tls);

/* iolog_path.c */
bool expand_iolog_path(const char *inpath, char *path, size_t pathlen, const struct iolog_path_escape *escapes, void *closure);

/* iolog_util.c */
bool iolog_parse_timing(const char *line, struct timing_closure *timing);
char *iolog_parse_delay(const char *cp, struct timespec *delay, const char *decimal_point);
int iolog_read_timing_record(struct iolog_file *iol, struct timing_closure *timing);
struct iolog_info *iolog_parse_loginfo(int dfd, const char *iolog_dir);
bool iolog_parse_loginfo_json(FILE *fp, const char *iolog_dir, struct iolog_info *li);
void iolog_adjust_delay(struct timespec *delay, struct timespec *max_delay, double scale_factor);
void iolog_free_loginfo(struct iolog_info *li);

/* iolog_fileio.c */
struct passwd;
struct group;
bool iolog_close(struct iolog_file *iol, const char **errstr);
bool iolog_eof(struct iolog_file *iol);
bool iolog_mkdtemp(char *path);
bool iolog_mkpath(char *path);
bool iolog_nextid(char *iolog_dir, char sessid[7]);
bool iolog_open(struct iolog_file *iol, int dfd, int iofd, const char *mode);
bool iolog_rename(const char *from, const char *to);
bool iolog_write_info_file(int dfd, const char *parent, struct iolog_info *log_info);
char *iolog_gets(struct iolog_file *iol, char *buf, size_t nbytes, const char **errsttr);
const char *iolog_fd_to_name(int iofd);
int iolog_openat(int fdf, const char *path, int flags);
off_t iolog_seek(struct iolog_file *iol, off_t offset, int whence);
ssize_t iolog_read(struct iolog_file *iol, void *buf, size_t nbytes, const char **errstr);
ssize_t iolog_write(struct iolog_file *iol, const void *buf, size_t len, const char **errstr);
void iolog_clearerr(struct iolog_file *iol);
void iolog_rewind(struct iolog_file *iol);
void iolog_set_compress(bool);
void iolog_set_defaults(void);
void iolog_set_flush(bool);
void iolog_set_gid(gid_t gid);
void iolog_set_maxseq(unsigned int maxval);
void iolog_set_mode(mode_t mode);
void iolog_set_owner(uid_t uid, uid_t gid);

#endif /* SUDO_IOLOG_H */