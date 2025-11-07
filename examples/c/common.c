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
#include <io.h>
#else
#include <limits.h>
#include <unistd.h>
#endif

static int yyaml_example_is_sep(int ch) {
#if defined(_WIN32)
    return ch == '/' || ch == '\\';
#else
    return ch == '/';
#endif
}

static void yyaml_example_trim_trailing_sep(char *path) {
    size_t len;
    if (!path) return;
    len = strlen(path);
    while (len > 0 && yyaml_example_is_sep((unsigned char)path[len - 1])) {
#if defined(_WIN32)
        if (len == 3 && path[1] == ':' && yyaml_example_is_sep((unsigned char)path[2])) {
            return;
        }
#endif
        if (len == 1) {
            return;
        }
        path[--len] = '\0';
    }
}

static bool yyaml_example_path_pop(char *path) {
    size_t len;
    if (!path) return false;
    len = strlen(path);
    if (len == 0) return false;
    yyaml_example_trim_trailing_sep(path);
    len = strlen(path);
    while (len > 0 && !yyaml_example_is_sep((unsigned char)path[len - 1])) {
        path[--len] = '\0';
    }
    if (len == 0) {
        return true;
    }
    if (len == 1 && path[0] == '/') {
        path[1] = '\0';
        return true;
    }
#if defined(_WIN32)
    if (len == 3 && path[1] == ':' && yyaml_example_is_sep((unsigned char)path[2])) {
        path[3] = '\0';
        return true;
    }
#endif
    path[len - 1] = '\0';
    return true;
}

static bool yyaml_example_path_append(char *path,
                                      size_t path_size,
                                      const char *component,
                                      size_t component_len) {
    size_t len;
    if (!path || !component) return false;
    if (component_len == 0) return true;
    len = strlen(path);
    if (len == 0) return false;
    if (!yyaml_example_is_sep((unsigned char)path[len - 1])) {
        if (len + 1 >= path_size) return false;
#if defined(_WIN32)
        path[len++] = '\\';
#else
        path[len++] = '/';
#endif
        path[len] = '\0';
    }
    if (len + component_len >= path_size) return false;
    memcpy(path + len, component, component_len);
    path[len + component_len] = '\0';
    return true;
}

bool yyaml_example_build_data_path(const char *source_file,
                                   const char *suffix,
                                   char *buffer,
                                   size_t buffer_size) {
    char file_path[1024];
    char dir_path[1024];
    char result[2048];
    const char *cursor;

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

    strncpy(dir_path, file_path, sizeof(dir_path));
    dir_path[sizeof(dir_path) - 1] = '\0';
    if (!yyaml_example_path_pop(dir_path) || dir_path[0] == '\0') {
        return false;
    }

    strncpy(result, dir_path, sizeof(result));
    result[sizeof(result) - 1] = '\0';

    cursor = suffix;
    while (*cursor) {
        const char *start;
        size_t len;
        while (*cursor && yyaml_example_is_sep((unsigned char)*cursor)) {
            cursor++;
        }
        if (*cursor == '\0') break;
        start = cursor;
        while (*cursor && !yyaml_example_is_sep((unsigned char)*cursor)) {
            cursor++;
        }
        len = (size_t)(cursor - start);
        if (len == 0) continue;
        if (len == 1 && start[0] == '.') {
            continue;
        }
        if (len == 2 && start[0] == '.' && start[1] == '.') {
            if (!yyaml_example_path_pop(result)) {
                return false;
            }
            continue;
        }
        if (!yyaml_example_path_append(result, sizeof(result), start, len)) {
            return false;
        }
    }

    if (strlen(result) + 1 > buffer_size) {
        return false;
    }
    memcpy(buffer, result, strlen(result) + 1);
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

bool yyaml_example_write_file(const char *path,
                              const char *data,
                              size_t data_len) {
    FILE *fp;
    if (!path || !data) {
        return false;
    }
    fp = fopen(path, "wb");
    if (!fp) {
        return false;
    }
    if (data_len > 0) {
        size_t written = fwrite(data, 1, data_len, fp);
        if (written != data_len) {
            fclose(fp);
            return false;
        }
    }
    fclose(fp);
    return true;
}

bool yyaml_example_create_temp_yaml(char *buffer, size_t buffer_size) {
    if (!buffer || buffer_size == 0) {
        return false;
    }

#if defined(_WIN32)
    {
        char tmp_dir[MAX_PATH];
        char template_buf[MAX_PATH];
        if (GetTempPathA((DWORD)sizeof(tmp_dir), tmp_dir) == 0) {
            return false;
        }
        if (snprintf(template_buf, sizeof(template_buf), "%syyaml-sampleXXXXXX.yaml", tmp_dir) < 0) {
            return false;
        }
        if (_mktemp_s(template_buf, sizeof(template_buf)) != 0) {
            return false;
        }
        if (!yyaml_example_write_file(template_buf, "", 0)) {
            return false;
        }
        if (strlen(template_buf) + 1 > buffer_size) {
            return false;
        }
        memcpy(buffer, template_buf, strlen(template_buf) + 1);
        return true;
    }
#else
    {
        const char *suffix = ".yaml";
        const char *tmp_dir = getenv("TMPDIR");
        char template_buf[4096];
        int fd;
        if (!tmp_dir || tmp_dir[0] == '\0') {
            tmp_dir = "/tmp";
        }
        if (snprintf(template_buf, sizeof(template_buf), "%s/yyaml-sampleXXXXXX%s", tmp_dir, suffix) < 0) {
            return false;
        }
#if defined(__APPLE__) || defined(__linux__) || defined(__unix__)
        fd = mkstemps(template_buf, (int)strlen(suffix));
#else
        {
            size_t suffix_len = strlen(suffix);
            template_buf[strlen(template_buf) - suffix_len] = '\0';
            fd = mkstemp(template_buf);
            strcat(template_buf, suffix);
        }
#endif
        if (fd < 0) {
            return false;
        }
        close(fd);
        if (strlen(template_buf) + 1 > buffer_size) {
            return false;
        }
        memcpy(buffer, template_buf, strlen(template_buf) + 1);
        return true;
    }
#endif
}
