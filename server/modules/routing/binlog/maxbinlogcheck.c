/*
 * Copyright (c) 2016 MariaDB Corporation Ab
 *
 * Use of this software is governed by the Business Source License included
 * in the LICENSE.TXT file and at www.mariadb.com/bsl.
 *
 * Change Date: 2019-07-01
 *
 * On the date above, in accordance with the Business Source License, use
 * of this software will be governed by version 2 or later of the General
 * Public License.
 */

/**
 * @file maxbinlogcheck.c - The MaxScale binlog check utility
 *
 * This utility checks a MySQL 5.6 and MariaDB 10.0.X binlog file and reports
 * any found error or an incomplete transaction.
 * It suggests the pos the file should be trucatetd at.
 *
 * @verbatim
 * Revision History
 *
 * Date         Who                 Description
 * 24/07/2015   Massimiliano Pinto  Initial implementation
 * 26/08/2015   Massimiliano Pinto  Added mariadb10 option
 *                                  for MariaDB 10 binlog compatibility
 *                                  Currently MariadDB 10 starting transactions
 *                                  are detected checking GTID event
 *                                  with flags = 0
 * @endverbatim
 */

#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include <maxscale/alloc.h>
#include <maxscale/log_manager.h>
#include "blr.h"


static void printVersion(const char *progname);
static void printUsage(const char *progname);

static struct option long_options[] =
{
    {"debug",     no_argument, 0, 'd'},
    {"version",   no_argument, 0, 'V'},
    {"fix",       no_argument, 0, 'f'},
    {"mariadb10", no_argument, 0, 'M'},
    {"help",      no_argument, 0, '?'},
    {0, 0, 0, 0}
};

char *binlog_check_version = "1.1.0";

int
maxscale_uptime()
{
    return 1;
}

int main(int argc, char **argv)
{
    int option_index = 0;
    int debug_out = 0;
    int fix_file = 0;
    int mariadb10_compat = 0;

    char c;
    while ((c = getopt_long(argc, argv, "dVfM?", long_options, &option_index)) >= 0)
    {
        switch (c)
        {
        case 'd':
            debug_out = 1;
            break;
        case 'V':
            printVersion(*argv);
            exit(EXIT_SUCCESS);
            break;
        case 'f':
            fix_file = 1;
            break;
        case 'M':
            mariadb10_compat = 1;
            break;
        case '?':
            printUsage(*argv);
            exit(optopt ? EXIT_FAILURE : EXIT_SUCCESS);
        }
    }

    int num_args = optind;

    if (argv[num_args] == NULL)
    {
        printf("ERROR: No binlog file was specified.\n");
        exit(EXIT_FAILURE);
    }

    size_t len = strlen(argv[num_args]);
    if (len > PATH_MAX)
    {
        printf("ERROR: The length of the provided path exceeds %d characters.\n", PATH_MAX);
        exit(EXIT_FAILURE);
    }

    char path[PATH_MAX + 1];
    strcpy(path, argv[num_args]);

    char *name = strrchr(path, '/');
    if (name)
    {
        ++name;
        len = strlen(name);
    }
    else
    {
        name = path;
    }

    if ((len == 0) || (len > BINLOG_FNAMELEN))
    {
        printf("ERROR: The length of the binlog filename is 0 or exceeds %d characters.\n",
               BINLOG_FNAMELEN);
        exit(EXIT_FAILURE);
    }

    ROUTER_INSTANCE *inst = (ROUTER_INSTANCE*)MXS_CALLOC(1, sizeof(ROUTER_INSTANCE));
    if (!inst)
    {
        exit(EXIT_FAILURE);
    }

    int fd = open(path, fix_file ? O_RDWR : O_RDONLY, 0666);
    if (fd == -1)
    {
        printf("ERROR: Failed to open binlog file %s: %s.\n",
               path, strerror(errno));
        MXS_FREE(inst);
        exit(EXIT_FAILURE);
    }

    inst->binlog_fd = fd;
    inst->mariadb10_compat = mariadb10_compat;
    strcpy(inst->binlog_name, name);

    // We ignore potential errors.
    mxs_log_init(NULL, NULL, MXS_LOG_TARGET_DEFAULT);
    mxs_log_set_augmentation(0);
    mxs_log_set_priority_enabled(LOG_DEBUG, debug_out);

    MXS_NOTICE("maxbinlogcheck %s", binlog_check_version);

    unsigned long filelen = 0;
    struct stat statb;
    if (fstat(inst->binlog_fd, &statb) == 0)
    {
        filelen = statb.st_size;
    }

    MXS_NOTICE("Checking %s (%s), size %lu bytes", path, inst->binlog_name, filelen);

    /* read binary log */
    int ret = blr_read_events_all_events(inst, fix_file, debug_out);

    mxs_log_flush_sync();

    MXS_NOTICE("Check retcode: %i, Binlog Pos = %lu", ret, inst->binlog_position);

    close(inst->binlog_fd);
    MXS_FREE(inst);

    mxs_log_flush_sync();
    mxs_log_finish();

    return 0;
}

/**
 * Print version information
 */
static void
printVersion(const char *progname)
{
    printf("%s Version %s\n", progname, binlog_check_version);
}

/**
 * Display the --help text.
 */
static void
printUsage(const char *progname)
{
    printVersion(progname);

    printf("The MaxScale binlog check utility.\n\n");
    printf("Usage: %s [-f] [-d] [-v] [<binlog file>]\n\n", progname);
    printf("  -f|--fix          Fix binlog file, require write permissions (truncate)\n");
    printf("  -d|--debug        Print debug messages\n");
    printf("  -M|--mariadb10    MariaDB 10 binlog compatibility\n");
    printf("  -V|--version      print version information and exit\n");
    printf("  -?|--help         Print this help text\n");
}

