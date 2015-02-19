#include <config.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#define PLUGIN_OPT_ALLOC_AND_COPY(x, y) \
    x= (char *)malloc(sizeof(char) * strlen(y) + 1); \
    strncpy(x, y, strlen(y) + 1);

int main (void)
{
    puts ("Hello World!");
    puts ("This is " PACKAGE_STRING ".");
    
    static char value[BUFSIZ] = "hello111111";
    char *dest = NULL;
    
    int i = 1;
    
    if (i == 0) {
        PLUGIN_OPT_ALLOC_AND_COPY(dest, value);
    }
    else if (i == 1) {
        PLUGIN_OPT_ALLOC_AND_COPY(dest, "fdhdsgfhagdfhgadhfgajhd");
    }
    puts(dest);
    
    return 0;
}
