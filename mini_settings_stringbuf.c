#include "mini_settings_stringbuf.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void mini_settings_stringbuf_init(struct mini_settings_stringbuf *buf,
                                  size_t capacity) {
  buf->data = malloc(capacity * sizeof(char));
  buf->size = 0;
  buf->capacity = capacity;
}

static void ensure_capacity(struct mini_settings_stringbuf *buf,
                            size_t capacity) {
  if (buf->capacity >= capacity) return;
  size_t new_capacity = (2 * buf->capacity + 1) * sizeof(char);
  if (new_capacity < capacity) new_capacity = capacity;
  char *new_buf = realloc(buf->data, new_capacity);
  if (new_buf == NULL) {
    perror("mini_settings_stringbuf_append");
    exit(1);
  }
  buf->data = new_buf;
  buf->capacity = new_capacity;
}

void mini_settings_stringbuf_append(struct mini_settings_stringbuf *buf,
                                    const char *str, size_t size) {
  ensure_capacity(buf, buf->size + size);
  memcpy(buf->data + buf->size, str, size * sizeof(char));
  buf->size += size;
}

void mini_settings_stringbuf_append_line(struct mini_settings_stringbuf *buf,
                                         const char *str, size_t size) {
  ensure_capacity(buf, buf->size + size + 1);
  memcpy(buf->data + buf->size, str, size * sizeof(char));
  buf->data[buf->size + size] = '\n';
  buf->size += size + 1;
}

void mini_settings_stringbuf_append_char(struct mini_settings_stringbuf *buf,
                                         char c) {
  ensure_capacity(buf, buf->size + 1);
  buf->data[buf->size++] = c;
}
