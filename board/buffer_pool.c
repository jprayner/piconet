#include "buffer_pool.h"

#include <stdlib.h>
#include "hardware/sync.h"

static bool _pool_buffer_alloc(pool_t *p, uint id);
static void _pool_buffer_free(pool_t *p, uint id);

bool pool_init(pool_t *p, size_t buffer_size, uint buffer_count) {
  p->buffer_size = 0;
  p->buffer_count = 0;

  p->buffers = calloc(buffer_count, sizeof(buffer_t));
  if (p->buffers == NULL) {
    return false;
  }
  p->buffer_size = buffer_size;

  mutex_init(&p->pool_mutex);

  for (uint i = 0; i < buffer_count; i++) {
    if (!_pool_buffer_alloc(p, i)) {
      pool_destroy(p);
      return false;
    }

    p->buffer_count++;
  }
}

void pool_destroy(pool_t *p) {
  if (p == NULL) {
    return;
  }

  mutex_enter_blocking(&p->pool_mutex);

  for (uint i = 0; i < p->buffer_count; i++) {
    _pool_buffer_free(p, i);
  }
  p->buffer_count = 0;

  if (p->buffers != NULL) {
    free(p->buffers);
    p->buffers = NULL;
  }

  mutex_exit(&p->pool_mutex);
}

buffer_t* pool_buffer_claim(pool_t *p) {
  mutex_enter_blocking(&p->pool_mutex);

  buffer_t *buffer = NULL;
  for (uint i = 0; i < p->buffer_count; i++) {
    if (!p->buffers[i].in_use) {
      p->buffers[i].in_use = true;
      buffer = &p->buffers[i];
      break;
    }
  }

  mutex_exit(&p->pool_mutex);
  return buffer;
}

void pool_buffer_release(pool_t *p, uint buffer_id) {
  mutex_enter_blocking(&p->pool_mutex);

  for (uint i = 0; i < p->buffer_count; i++) {
    if (p->buffers[i].id == buffer_id) {
      p->buffers[i].in_use = false;
      break;
    }
  }

  mutex_exit(&p->pool_mutex);
}

buffer_t* pool_buffer_get(pool_t *p, uint buffer_id) {
  mutex_enter_blocking(&p->pool_mutex);

  buffer_t *buffer = NULL;
  for (uint i = 0; i < p->buffer_count; i++) {
    if (p->buffers[i].in_use) {
      buffer = &p->buffers[i];
      break;
    }
  }

  mutex_exit(&p->pool_mutex);
  return buffer;
}

static bool _pool_buffer_alloc(pool_t *p, uint id) {
  if (id >= p->buffer_count) {
    return false;
  }

  p->buffers[id].id = id;
  p->buffers[id].in_use = false;
  p->buffers[id].data = malloc(p->buffer_size);

  if (p->buffers[id].data == NULL) {
    return false;
  }

  return true;
}

static void _pool_buffer_free(pool_t *p, uint id) {
  if (id >= p->buffer_count) {
    return;
  }

  if (p->buffers[id].data != NULL) {
    free(p->buffers[id].data);
  }

  p->buffers[id].data = NULL;
}
