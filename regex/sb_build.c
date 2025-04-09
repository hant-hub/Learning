#define SB_IMPL
#include "sb.h"



int main(int argc, char* argv[]) {
    AUTO_REBUILD(argc, argv);


    sb_cmd* c = &(sb_cmd){0};
    sb_cmd_push(c, "mkdir", "-p", "build");
    if (sb_cmd_sync_and_reset(c)) {
        exit(-1);
    }

    sb_cmd_push(c, "cc", "-g");
    //sb_cmd_push(c, "-fsanitize=leak,address");
    sb_cmd_push(c, "src/main.c");
    sb_cmd_push(c, "src/regex.c");
    sb_cmd_push(c, "-o", "build/test");

    if (sb_cmd_sync_and_reset(c)) {
        exit(-1);
    }

    //test
    sb_cmd_push(c, "./build/test");
    if (sb_cmd_sync_and_reset(c)) {
        exit(-1);
    }

    return 0;
}
