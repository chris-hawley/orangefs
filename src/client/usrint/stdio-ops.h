/* 
 * (C) 2011 Clemson University and The University of Chicago 
 *
 * See COPYING in top-level directory.
 */

/** \file
 *  \ingroup usrint
 *
 *  PVFS2 user interface routines - implementation of stdio for pvfs
 */

#ifndef STDIO_OPS_H
#define STDIO_OPS_H

#ifndef _IO_MAGIC

struct _IO_marker {
  struct _IO_marker *_next;
  struct _IO_FILE *_sbuf;
  /* If _pos >= 0
 it points to _buf->Gbase()+_pos. FIXME comment */
  /* if _pos < 0, it points to _buf->eBptr()+_pos. FIXME comment */
  int _pos;
#if 0
    void set_streampos(streampos sp) { _spos = sp; }
    void set_offset(int offset) { _pos = offset; _spos = (streampos)(-2); }
  public:
    streammarker(streambuf *sb);
    ~streammarker();
    int saving() { return  _spos == -2; }
    int delta(streammarker&);
    int delta();
#endif
};

#define _IO_MAGIC 0xFBAD0000 /* Magic number */
#define _OLD_STDIO_MAGIC 0xFABC0000 /* Emulate old stdio. */
#define _IO_MAGIC_MASK 0xFFFF0000
#define _IO_USER_BUF 1 /* User owns buffer; don't delete it on close. */
#define _IO_UNBUFFERED 2
#define _IO_NO_READS 4 /* Reading not allowed */
#define _IO_NO_WRITES 8 /* Writing not allowd */
#define _IO_DELETE_DONT_CLOSE 0x40 /* Don't call close(_fileno) on cleanup. */
#define _IO_LINKED 0x80 /* Set if linked (using _chain) to streambuf::_list_all.*/
#define _IO_IN_BACKUP 0x100
#define _IO_LINE_BUF 0x200
#define _IO_TIED_PUT_GET 0x400 /* Set if put and get pointer logicly tied. */
#define _IO_CURRENTLY_PUTTING 0x800
#define _IO_IS_APPENDING 0x1000
#define _IO_IS_FILEBUF 0x2000
#define _IO_BAD_SEEN 0x4000
#define _IO_USER_LOCK 0x8000

#define _IO_FILE struct _IO_FILE

#endif

struct stdio_ops_s
{
    FILE *(*fopen)(const char *path, const char *mode);
    FILE *(*fdopen)(int fd, const char *mode);
    FILE *(*freopen)(const char *path, const char *mode, FILE *stream);
    size_t (*fwrite)(const void *ptr, size_t size, size_t nmemb, FILE *stream);
    size_t (*fwrite_unlocked)(const void *ptr, size_t size, size_t nmemb, FILE *stream);
    size_t (*fread)(void *ptr, size_t size, size_t nmemb, FILE *stream);
    size_t (*fread_unlocked)(void *ptr, size_t size, size_t nmemb, FILE *stream);
    int (*fclose)(FILE *stream);
    int (*fseek)(FILE *stream, long offset, int whence);
    int (*fseek64)(FILE *stream, const off64_t offset, int whence);
    int (*fsetpos)(FILE *stream, const fpos_t *pos);
    void (*rewind)(FILE *stream);
    long int (*ftell)(FILE *stream);
    off64_t (*ftell64)(FILE *stream);
    int (*fgetpos)(FILE *stream, fpos_t *pos);
    int (*fflush)(FILE *stream);
    int (*fflush_unlocked)(FILE *stream);
    int (*fputc)(int c, FILE *stream);
    int (*fputc_unlocked)(int c, FILE *stream);
    int (*fputs)(const char *s, FILE *stream);
    int (*fputs_unlocked)(const char *s, FILE *stream);
    int (*putc)(int c, FILE *stream);
    int (*putc_unlocked)(int c, FILE *stream);
    int (*putchar)(int c);
    int (*putchar_unlocked)(int c);
    int (*puts)(const char *s);
    int (*putw)(int wd, FILE *stream);
    char *(*fgets)(char *s, int size, FILE *stream);
    char *(*fgets_unlocked)(char *s, int size, FILE *stream);
    int (*fgetc)(FILE *stream);
    int (*fgetc_unlocked)(FILE *stream);
    int (*getc)(FILE *stream);
    int (*getc_unlocked)(FILE *stream);
    int (*getchar)(void);
    int (*getchar_unlocked)(void);
    int (*getw)(FILE *stream);
    char *(*gets)(char * s);
    ssize_t (*getdelim)(char **lnptr, size_t *n, int delim, FILE *stream);
    int (*ungetc)(int c, FILE *stream);
    int (*vfprintf)(FILE *stream, const char *format, va_list ap);
    int (*vprintf)(const char *format, va_list ap);
    int (*fprintf)(FILE *stream, const char *format, ...);
    int (*printf)(const char *format, ...);
    void (*perror)(const char *s);
    int (*fscanf)(FILE *stream, const char *format, ...);
    int (*scanf)(const char *format, ...);
    void (*clearerr)(FILE *stream);
    void (*clearerr_unlocked)(FILE *stream);
    int (*feof)(FILE *stream);
    int (*feof_unlocked)(FILE *stream);
    int (*ferror)(FILE *stream);
    int (*ferror_unlocked)(FILE *stream);
    int (*fileno)(FILE *stream);
    int (*fileno_unlocked)(FILE *stream);
    int (*remove)(const char *path);
    void (*setbuf)(FILE *stream, char *buf);
    void (*setbuffer)(FILE *stream, char *buf, size_t size);
    void (*setlinebuf)(FILE *stream);
    int (*setvbuf)(FILE *stream, char *buf, int mode, size_t size);
    char *(*mkdtemp)(char *template);
    int (*mkstemp)(char *template);
    FILE *(*tmpfile)(void);
    DIR *(*opendir)(const char *name);
    DIR *(*fdopendir)(int fd);
    int (*dirfd)(DIR *dir);
    struct dirent *(*readdir)(DIR *dir);
    struct dirent64 *(*readdir64)(DIR *dir);
    void (*rewinddir)(DIR *dir);
    void (*seekdir)(DIR *dir, off_t offset);
    off_t (*telldir)(DIR *dir);
    int (*closedir)(DIR *dir);
#ifdef PVFS_SCANDIR_VOID
    int (*scandir)(const char *dir,
                   struct dirent ***namelist,
                   int(*filter)(const struct dirent *),
                   int(*compar)(const void *,
                                const void *));
    int (*scandir64)(const char *dir,
                     struct dirent64 ***namelist,
                     int(*filter)(const struct dirent64 *),
                     int(*compar)(const void *,
                                  const void *));
#else
    int (*scandir)(const char *dir,
                   struct dirent ***namelist,
                   int(*filter)(const struct dirent *),
                   int(*compar)(const struct dirent **,
                                const struct dirent **));
    int (*scandir64)(const char *dir,
                     struct dirent64 ***namelist,
                     int(*filter)(const struct dirent64 *),
                     int(*compar)(const struct dirent64 **,
                                  const struct dirent64 **));
#endif
    void (*flockfile)(FILE *stream);
    int (*ftrylockfile)(FILE *stream);
    void (*funlockfile)(FILE *stream);
};

/* various declarations and defines for pvfs stdio layer */

#if defined _G_IO_IO_FILE_VERSION && _G_IO_IO_FILE_VERSION == 0x20001
# define USE_OFFSET 1
#else
# define USE_OFFSET 0
#endif

#define _IO_pos_BAD -1
#define _IO_wide_NOT -1

#define SETMAGIC(s,m)   do{(s)->_flags = (m) & _IO_MAGIC_MASK;}while(0)
#define ISMAGICSET(s,m) (((s)->_flags & _IO_MAGIC_MASK) == (m))
#define SETFLAG(s,f)    do{(s)->_flags |= ((f) & ~_IO_MAGIC_MASK);}while(0)
#define CLEARFLAG(s,f)  do{(s)->_flags &= ~((f) & ~_IO_MAGIC_MASK);}while(0)
#define ISFLAGSET(s,f)  (((s)->_flags & (f)) == (f))

void pvfs_set_to_put(FILE *stream);
int pvfs_write_buf(FILE *stream);
int pvfs_set_to_get(FILE *stream);
int pvfs_read_buf(FILE *stream);

#endif

/*
 * Local variables:
 *  c-indent-level: 4
 *  c-basic-offset: 4
 * End:
 *
 * vim: ts=8 sts=4 sw=4 expandtab
 */
