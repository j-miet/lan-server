#include "fixtures.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static char original_dir[512];
static char test_dir[512];
static int test_dir_created = 0;

void test_setup(void) {
    getcwd(original_dir, sizeof(original_dir));

    char tmpl[512];
    snprintf(tmpl, sizeof(tmpl), "/tmp/lan_server_test_XXXXXX");

    if (mkdtemp(tmpl) == NULL) {
        perror("mkdtemp");
        exit(1);
    }

    strcpy(test_dir, tmpl);

    if (chdir(test_dir) != 0) {
        perror("chdir");
        exit(1);
    }

    test_dir_created = 1;

    // permissions -> owner: r+w+x, group: r+x, others: r+x
    mkdir("uploads", 0755);
}

void test_teardown(void) {
    if (!test_dir_created)
        return;

    chdir(original_dir);

    char cmd[1024];
    snprintf(cmd, sizeof(cmd), "rm -rf '%s'", test_dir);

    //  executes "rm -rf 'test_dir'", but only after ensuring the dir exists
    system(cmd);
}