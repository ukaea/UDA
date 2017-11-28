#include <stdio.h>

int func(char*bucket, char*object, int *count, char **buffer);

int main(int argc, char** argv)
{
    if (argc < 3)
    {
        printf("To run this example, supply the name of an S3 bucket and object to"
               "download from it.\n" 
               "e.g.: get_object <bucketname> <filename>\n");
        return 1;
    }
    
    int count = 0;
    char *buffer = NULL;
    int rc = func(argv[1], argv[2], &count, &buffer);
    
    printf("bucket = %s\n", argv[1]);
    printf("object = %s\n", argv[2]);
    printf("object count = %d\n", count);
    printf("object:\n%s\n", buffer);
    
    return 0;
}
