#include "version.h"

#include <stdio.h>

bool version_parse(const char *text, version_t *out)
{
    if (text == NULL || out == NULL) {
        return false;
    }
    if (*text == 'v' || *text == 'V') {
        text++;
    }
    unsigned major = 0;
    unsigned minor = 0;
    unsigned patch = 0;
    if (sscanf(text, "%u.%u.%u", &major, &minor, &patch) != 3) {
        return false;
    }
    out->major = major;
    out->minor = minor;
    out->patch = patch;
    return true;
}

int version_compare(const version_t *a, const version_t *b)
{
    if (a->major != b->major) {
        return a->major > b->major ? 1 : -1;
    }
    if (a->minor != b->minor) {
        return a->minor > b->minor ? 1 : -1;
    }
    if (a->patch != b->patch) {
        return a->patch > b->patch ? 1 : -1;
    }
    return 0;
}
