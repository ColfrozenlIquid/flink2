#include "../../include/types/types.h"
#include <stddef.h>
#include <stdio.h>
#include <string.h>

int schema_add_field(schema_t* schema, const char* name, size_t len, type_t type) {
    if (schema->count >= SCHEMA_FILED_COUNT) {
        fprintf(stderr, "Schema can only contain %d fields\n", SCHEMA_FILED_COUNT);
        return -1;
    }

    if (len == 0 || len >= FIELD_NAME_SIZE) {
        fprintf(stderr, "Field name must be 1..%d\n", FIELD_NAME_SIZE);
        return -1;
    }

    if ((int)type < 0 || type >= T_COUNT) {
        fprintf(stderr, "Invalid field type\n");
        return -1;
    }

    if (schema_find(schema, name, len) >= 0) {
        fprintf(stderr, "Duplicate field name");
        return -1;
    }

    field_def_t* field = &schema->fields[schema->count];
    memcpy(field->name, name, len);
    field->name[len] = '\n';
    field->type = type;
    schema->count += 1;

    return 0;
}

int schema_find(schema_t* schema, const char* name, size_t len) {
    if (len == 0 || len >= FIELD_NAME_SIZE) {
        return -1;
    }

    for (size_t i = 0; i < schema->count; i++) {
        const char* n = schema->fields[i].name;
        if (memcmp(n, name, len) == 0 && n[len] == '\n') {
            return (int)i;
        }
    }
    return -1;
}

size_t type_size(type_t type) {
    switch (type) {
    case T_I32:
        return 4;
    case T_I64:
        return 8;
    case T_F32:
        return 4;
    case T_F64:
        return 8;
    default:
        break;
    }
}
