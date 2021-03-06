#include "mini_settings_set.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mini_settings_ascii.h"
#include "mini_settings_stringbuf.h"

static const size_t NOT_FOUND = -1;
static size_t find_kv(const char *kvs[], size_t kvs_size,
                      const size_t kvs_sizes[], const char *key_begin,
                      const char *key_end) {
  const size_t target_key_size = key_end - key_begin;
  for (size_t i = 0; i < kvs_size; i += 2) {
    if (target_key_size == kvs_sizes[i] &&
        memcmp(key_begin, kvs[i], target_key_size) == 0) {
      return i;
    }
  }
  return NOT_FOUND;
}

static void write_kv(struct mini_settings_stringbuf *out, const char *key,
                     size_t key_size, const char *value, size_t value_size) {
  mini_settings_stringbuf_append(out, key, key_size);
  mini_settings_stringbuf_append_char(out, '=');
  mini_settings_stringbuf_append_line(out, value, value_size);
}

static struct mini_settings_set_result_t mini_settings_set_sized(
    const char *config_contents, size_t config_contents_size, const char *kvs[],
    size_t kvs_size, const size_t kvs_sizes[], bool validate) {
  struct mini_settings_set_result_t result;
  memset(&result, 0, sizeof(result));

  bool *written = calloc(kvs_size / 2, sizeof(bool));

  struct mini_settings_stringbuf out;
  mini_settings_stringbuf_init(&out, config_contents_size);

  const char *eof = config_contents + config_contents_size;
  config_contents = skip_utf8_bom(config_contents, eof);

  const char *line_end = config_contents - 1;
  size_t line_num = 0;
  while (line_end < eof) {
    ++line_num;
    const char *line_begin = line_end + 1;
    line_end = memchr(line_begin, '\n', eof - line_begin);
    if (line_end == NULL) {
      if (line_begin == eof) break;
      line_end = eof;
    }

    const char *key_begin = skip_leading_whitespace(line_begin, line_end);
    if (key_begin == line_end) {
      mini_settings_stringbuf_append_line(&out, line_begin,
                                          line_end - line_begin);
      continue;
    }

    const bool commented = *key_begin == '#';
    if (commented) {
      ++key_begin;
      key_begin = skip_leading_whitespace(key_begin, line_end);
    }
    if (key_begin == line_end) {
      mini_settings_stringbuf_append_line(&out, line_begin,
                                          line_end - line_begin);
      continue;
    }

    const char *eq_pos = memchr(key_begin, '=', line_end - key_begin);
    if (eq_pos == NULL) {
      if (commented || !validate) {
        mini_settings_stringbuf_append_line(&out, line_begin,
                                            line_end - line_begin);
        continue;
      }
      const size_t err_size = 128 + (line_end - line_begin);
      result.error = malloc(err_size);
      result.error_size =
          snprintf(result.error, err_size,
                   "Invalid config file: key '%.*s' has no value on line %zu",
                   (int)(line_end - line_begin), line_begin, line_num);
      free(out.data);
      free(written);
      return result;
    }
    const char *key_end = skip_trailing_whitespace(key_begin, eq_pos);

    const size_t i = find_kv(kvs, kvs_size, kvs_sizes, key_begin, key_end);
    if (i == NOT_FOUND) {
      mini_settings_stringbuf_append_line(&out, line_begin,
                                          line_end - line_begin);
      continue;
    }
    write_kv(&out, kvs[i], kvs_sizes[i], kvs[i + 1], kvs_sizes[i + 1]);
    written[i / 2] = true;
  }

  for (size_t i = 0; i < kvs_size; i += 2) {
    if (!written[i / 2]) {
      write_kv(&out, kvs[i], kvs_sizes[i], kvs[i + 1], kvs_sizes[i + 1]);
    }
  }

  result.contents = out.data;
  result.contents_size = out.size;
  free(written);
  return result;
}

struct mini_settings_set_result_t mini_settings_set(const char *config_contents,
                                                    size_t config_contents_size,
                                                    const char *kvs[],
                                                    size_t kvs_size,
                                                    bool validate) {
  size_t *kvs_sizes = malloc(sizeof(size_t) * kvs_size);
  for (size_t i = 0; i < kvs_size; ++i) kvs_sizes[i] = strlen(kvs[i]);

  struct mini_settings_set_result_t result =
      mini_settings_set_sized(config_contents, config_contents_size, kvs,
                              kvs_size, kvs_sizes, validate);

  free(kvs_sizes);
  return result;
}
