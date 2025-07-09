#include <stdio.h>

#include "sds.h"

int main(int argc, char **argv) {
    sds s, *tokens;
    int count, j;

    s = sdsnew("ciao--come----va--tutto ok?--");
    tokens = sdssplitlen(s,sdslen(s),"--",1,&count);
    printf("%s spitted into %d tokens\n",s,count);
    for (j = 0; j < count; j++) {
        printf("Token %d (len %d): %s\n", j, sdslen(tokens[j]),tokens[j]);
    }
    return 0;
}
