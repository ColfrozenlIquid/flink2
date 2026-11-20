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

int batch_allocate(schema_t* schema, batch_t* out);

void* allocate_column(size_t rows, size_t element_size);

void batch_free(batch_t* in);

void batch_fill_random(batch_t* in, size_t n, uint64_t* rng_state);

void batch_destroy(batch_t* batch);
