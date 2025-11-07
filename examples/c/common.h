/*
 * This code is for Mohammad Raziei (https://github.com/mohammadraziei/yyaml).
 * Written on 2025-11-06 and released under the MIT license.
 * If you use it, please star the repository and report issues via GitHub.
 */

#ifndef YYAML_EXAMPLE_COMMON_H
#define YYAML_EXAMPLE_COMMON_H

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

bool yyaml_example_build_data_path(const char *source_file,
                                   const char *suffix,
                                   char *buffer,
                                   size_t buffer_size);

bool yyaml_example_read_file(const char *path,
                              char **out_data,
                              size_t *out_len);

bool yyaml_example_write_file(const char *path,
                              const char *data,
                              size_t data_len);

bool yyaml_example_create_temp_yaml(char *buffer, size_t buffer_size);

#ifdef __cplusplus
}
#endif

#endif /* YYAML_EXAMPLE_COMMON_H */
