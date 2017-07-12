#include <stdlib.h>
#include <libxml/xpath.h>

int main()
{
    size_t len = 10;
    int value = 0;

    xmlChar* xPathExpr = malloc(len + sizeof(xmlChar));
    xmlStrPrintf(xPathExpr, len, "value = %d", value);

    return 0;
}
