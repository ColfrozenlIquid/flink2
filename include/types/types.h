#pragma once
#include <stddef.h>
#include <stdint.h>

typedef enum { T_I32, T_I64, T_F32, T_F64, T_COUNT } type_t;

#define FIELD_NAME_SIZE 32

typedef struct {
    char name[FIELD_NAME_SIZE];
    type_t type;
} field_def_t;

#define SCHEMA_FILED_COUNT 16

typedef struct {
    field_def_t fields[SCHEMA_FILED_COUNT];
    size_t count;
} schema_t;

void schema_init(schema_t* schema);

int schema_add_field(schema_t* schema, const char* name, size_t len, type_t type);

int schema_find(schema_t* schema, const char* name, size_t len);

size_t type_size(type_t type);

typedef enum { CMP_GT, CMP_LT, CMP_EQ, CMP_COUNT } cmp_t;

typedef enum { AGG_SUM, AGG_COUNT, AGG_COUNT_MAX } agg_t;

typedef union {
    int64_t i;
    double f;
} value_t;

typedef struct {
    int col;
    cmp_t cmp;
    value_t rhs;
} filter_t;

typedef struct {
    filter_t filters[4];
    size_t filters_count;
    agg_t agg;
    int agg_col;
} plan_t;

#define BATCH_SIZE 4096

typedef struct {
    const schema_t* schema;
    void* cols[16];
    size_t rows_count; // rows currently valid
    size_t capacity;   // rows allocated
} batch_t;

void batch_init(schema_t* schema, uint8_t* base, batch_t* out, const size_t* col_offset);

size_t batch_layout(const schema_t* schema, size_t col_offset[16]);

typedef struct {
    schema_t schema;       // defines what schema the pool is using
    size_t batch_bytes;    // bytes per batch
    size_t col_offset[16]; // byte offset of each column within a batch
    uint8_t* slab;         // aligned alloc for the entire pool
    batch_t* batches;      // array of pool size batches
    batch_t** free_list;   // stack
    size_t free_count;
    size_t pool_size;
} batch_pool_t;

int pool_init(batch_pool_t* pool, const schema_t* schema, size_t pool_size);

void pool_destroy(batch_pool_t* pool);

batch_t* pool_acquire(batch_pool_t* pool);

void pool_release(batch_pool_t* pool, batch_t* batch);
