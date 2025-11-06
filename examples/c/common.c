/*
 * This code is for Mohammad Raziei (https://github.com/mohammadraziei/yyaml).
 * Written on 2025-11-06 and released under the MIT license.
 * If you use it, please star the repository and report issues via GitHub.
 */

#include "common.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(_WIN32)
#include <windows.h>
#else
#include <limits.h>
#include <unistd.h>
#endif

bool yyaml_example_build_data_path(const char *source_file,
                                   const char *suffix,
                                   char *buffer,
                                   size_t buffer_size) {
    char file_path[1024];
    const char *sep;
    size_t dir_len;
    int written;

    if (!buffer || buffer_size == 0 || !source_file || !suffix) {
        return false;
    }

#if defined(_WIN32)
    if (!_fullpath(file_path, source_file, sizeof(file_path))) {
        return false;
    }
#else
    if (!realpath(source_file, file_path)) {
        return false;
    }
#endif

    sep = strrchr(file_path, '/');
#if defined(_WIN32)
    {
        const char *back = strrchr(file_path, '\\');
        if (!sep || (back && back > sep)) {
            sep = back;
        }
    }
#endif

    dir_len = sep ? (size_t)(sep - file_path) : 0U;
    if (dir_len >= sizeof(file_path)) {
        return false;
    }

    written = snprintf(buffer, buffer_size, "%.*s%s", (int)dir_len, file_path, suffix);
    if (written < 0 || (size_t)written >= buffer_size) {
        return false;
    }

#if defined(_WIN32)
    {
        char resolved[1024];
        size_t len;
        if (!_fullpath(resolved, buffer, sizeof(resolved))) {
            return false;
        }
        len = strlen(resolved);
        if (len + 1 > buffer_size) {
            return false;
        }
        memcpy(buffer, resolved, len + 1);
    }
#else
    {
        char resolved[1024];
        size_t len;
        if (!realpath(buffer, resolved)) {
            return false;
        }
        len = strlen(resolved);
        if (len + 1 > buffer_size) {
            return false;
        }
        memcpy(buffer, resolved, len + 1);
    }
#endif

    return true;
}

bool yyaml_example_read_file(const char *path,
                             char **out_data,
                             size_t *out_len) {
    FILE *fp;
    long size;
    char *buf;

    if (!path || !out_data || !out_len) {
        return false;
    }

    *out_data = NULL;
    *out_len = 0;

    fp = fopen(path, "rb");
    if (!fp) {
        return false;
    }

    if (fseek(fp, 0, SEEK_END) != 0) {
        fclose(fp);
        return false;
    }
    size = ftell(fp);
    if (size < 0) {
        fclose(fp);
        return false;
    }
    if (fseek(fp, 0, SEEK_SET) != 0) {
        fclose(fp);
        return false;
    }

    buf = (char *)malloc((size_t)size + 1);
    if (!buf) {
        fclose(fp);
        return false;
    }

    if (size > 0) {
        size_t read_len = fread(buf, 1, (size_t)size, fp);
        if (read_len != (size_t)size) {
            free(buf);
            fclose(fp);
            return false;
        }
    }
    buf[size] = '\0';

    fclose(fp);
    *out_data = buf;
    *out_len = (size_t)size;
    return true;
}
