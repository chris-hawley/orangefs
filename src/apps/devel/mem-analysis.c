/*
 * (C) 2001 Clemson University and The University of Chicago
 *
 * See COPYING in top-level directory.
 */

/** \defgroup mem-analysis memory allocation analysis tool
 *
 *
 * @{
 */

/** \file
 *
 *  Core of memory allocation analysis tool
 *  including processing arguments, calling the parser, and producing
 *  warning and error messages.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef WIN32
#include <unistd.h>
#endif

/* force malloc to not be redefined for statecomp */
#define PVFS_MALLOC_REDEF_OVERRIDE 1
/* #include "pvfs2-internal.h" */
#include "mem-analysis.h"

#ifdef __GNUC__
#ifdef YYPARSE_PARAM
int yyparse (void *);
#else
int yyparse (void);
#endif
#endif

#ifdef WIN32
#define __func__    __FUNCTION__
#define unlink      _unlink

extern int yyparse();
#endif

/*
 * Global Variables
 */
int line = 1;
int col = 1;
FILE *out_file;
const char *progname;
const char *in_file_name;
static char *out_file_name;

static void parse_args(int argc, char **argv);
static void finalize(void);
static char *estrdup(const char *oldstring);

int main(int argc, char **argv)
{
    int retval;
    parse_args(argc, argv);
    retval = yyparse();
    switch (retval)
    {
        case 0:
            /* successful parse */
            break;
        case 1:
            /* syntax error */
            fprintf(stderr,"yyparse returned syntax error\n");
            break;
        case 2:
            /* out of memory error */
            fprintf(stderr,"yyparse returned out of memory error\n");
            break;
        default:
            /* unknown error */
            fprintf(stderr,"yyparse returned unknown error\n");
            break;
    }
    finalize();
    return retval;
}

static void usage(void)
{
    fprintf(stderr, "Usage: %s input_file [output_file]\n", progname);
    exit(1);
}

static void parse_args(int argc, char **argv)
{
    int len;
    const char *cp;

    for (cp=progname=argv[0]; *cp; cp++)
    {
	if (*cp == '/')
        {
	    progname = cp+1;
        }
    }

    if (argc < 2 || argc > 3)
    {
        usage();
    }

    in_file_name = argv[1];
    out_file_name = (argc == 3) ? argv[2] : NULL;

    if (!freopen(in_file_name, "r", stdin))
    {
	perror("open input file");
	exit(1);
    }

    if (!out_file_name)
    {
	/* construct output file name from input file name */
	out_file_name = estrdup(in_file_name);
        strcat(out_file_name, ".out");
    }

    out_file = fopen(out_file_name, "w");
    if (!out_file)
    {
        perror("opening output file");
        exit(1);
    }

    /* dump header comment into out file */
    fprintf(out_file,
    "/* WARNING: THIS FILE IS AUTOMATICALLY GENERATED FROM A .SM FILE.\n");
    fprintf(out_file,
    " * Changes made here will certainly be overwritten.\n");
    fprintf(out_file,
    " */\n\n");
}

static void finalize(void)
{
    fclose(out_file);
}

void yyerror(char *s)
{
    fprintf(stderr, "%s: %s:%d:%d: %s\n", progname, in_file_name, line, col, s);
    unlink(out_file_name);
    exit(1);
}

/*
 * Error checking malloc.
 */
static void *emalloc(size_t size)
{
    void *p;

    p = malloc(size);
    if (!p)
    {
	fprintf(stderr, "%s: no more dynamic storage - aborting\n", progname);
	exit(1);
    }
    return p;
}

static char *estrdup(const char *oldstring)
{
    char *s;

    s = emalloc(strlen(oldstring) + 4 + 1);
    strcpy(s, oldstring);
    return s;
}

/* @} */

/*
 * Local variables:
 *  c-indent-level: 4
 *  c-basic-offset: 4
 * End:
 *
 * vim: ts=8 sts=4 sw=4 expandtab
 */