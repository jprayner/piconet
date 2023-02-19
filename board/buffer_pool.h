#ifndef _PICONET_BUFFER_POOL_ADLC_H_
#define _PICONET_BUFFER_POOL_H_

#include "pico.h"
#include "pico/mutex.h"

typedef struct {
  uint      id;
  bool      in_use;
  uint8_t*  data;
} buffer_t;

typedef struct {
  mutex_t   pool_mutex;
  size_t    buffer_size;
  size_t    buffer_count;
  buffer_t* buffers;
} pool_t;

bool      pool_init(pool_t *p, size_t buffer_size, uint buffer_count);
void      pool_destroy(pool_t *p);

buffer_t* pool_buffer_claim(pool_t *p);
void      pool_buffer_release(pool_t *p, uint buffer_id);
buffer_t* pool_buffer_get(pool_t *p, uint buffer_id);

#endif