#include <stdio.h>
#include "original.h"

int main(void) {
    int r = add(2, 3);
    printf("[app] add returned %d\n", r);
    return 0;
}
