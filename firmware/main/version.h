#pragma once

#include <stdbool.h>

typedef struct {
    unsigned major;
    unsigned minor;
    unsigned patch;
} version_t;

bool version_parse(const char *text, version_t *out);
int version_compare(const version_t *a, const version_t *b);
