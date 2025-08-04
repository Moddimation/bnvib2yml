#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cyaml/cyaml.h>

typedef struct __attribute__((packed)) {
    short rate;
    unsigned int size;
} VibDataN;
typedef struct __attribute__((packed)) {
    short rate;
    int loop_start;
    int loop_end;
    unsigned int size;
} VibDataL;
typedef struct __attribute__((packed)) {
    short rate;
    int loop_start;
    int loop_end;
    int loop_wait;
    unsigned int size;
} VibDataW;

typedef struct {
    char type;
    const char* typeName;
} VibMeta;

static const char* vibTypeNames[] = {"Once", "Loop", "LoopWait"};

static const cyaml_schema_field_t fields_meta[] = {
    CYAML_FIELD_INT("type_id",        CYAML_FLAG_DEFAULT, VibMeta, type),
    CYAML_FIELD_STRING_PTR("type_name", CYAML_FLAG_DEFAULT, VibMeta, typeName, 0, CYAML_UNLIMITED),
};
static const cyaml_schema_value_t schema_meta = {
    CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER, VibMeta, fields_meta),
};

static const cyaml_schema_field_t fields_n[] = {
    CYAML_FIELD_INT("vib_rate", CYAML_FLAG_DEFAULT, VibDataN, rate),
    CYAML_FIELD_UINT("vib_size", CYAML_FLAG_DEFAULT, VibDataN, size),
};
static const cyaml_schema_value_t schema_n = {
    CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER, VibDataN, fields_n),
};

static const cyaml_schema_field_t fields_l[] = {
    CYAML_FIELD_INT("vib_rate",    CYAML_FLAG_DEFAULT, VibDataL, rate),
    CYAML_FIELD_INT("loop_start",  CYAML_FLAG_DEFAULT, VibDataL, loop_start),
    CYAML_FIELD_INT("loop_end",    CYAML_FLAG_DEFAULT, VibDataL, loop_end),
    CYAML_FIELD_UINT("vib_size",   CYAML_FLAG_DEFAULT, VibDataL, size),
};
static const cyaml_schema_value_t schema_l = {
    CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER, VibDataL, fields_l),
};

static const cyaml_schema_field_t fields_w[] = {
    CYAML_FIELD_INT("vib_rate",    CYAML_FLAG_DEFAULT, VibDataW, rate),
    CYAML_FIELD_INT("loop_start",  CYAML_FLAG_DEFAULT, VibDataW, loop_start),
    CYAML_FIELD_INT("loop_end",    CYAML_FLAG_DEFAULT, VibDataW, loop_end),
    CYAML_FIELD_INT("loop_wait",   CYAML_FLAG_DEFAULT, VibDataW, loop_wait),
    CYAML_FIELD_UINT("vib_size",   CYAML_FLAG_DEFAULT, VibDataW, size),
};
static const cyaml_schema_value_t schema_w = {
    CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER, VibDataW, fields_w),
};

int main(int argc, char* argv[])
{
    //puts("NIN de vib for .bnvib");
    //puts("Only extract metadata");

    if (argc <= 2)
    {
        fprintf(stderr, "Need 2 parameters: inpath and outpath!\n");
        return 1;
    }

    const char* out      = argv[2];
    char tmpname[512];
    snprintf(tmpname, sizeof(tmpname), "%s.1", out);

    FILE* f = fopen(argv[1], "rb");
    if (!f)
    {
        perror("Error opening input file");
        return 1;
    }

    char type;
    fread(&type, 1, 1, f);
    fseek(f, 0x6, 0);

    VibMeta meta = { type, NULL };
    void* data = NULL;
    const cyaml_schema_value_t* data_schema = NULL;

    switch (type)
    {
        case 0x4: {
            VibDataN* n = malloc(sizeof(*n));
            meta.typeName = vibTypeNames[0];
            fread(n, sizeof(*n), 1, f);
            data = n;
            data_schema = &schema_n;
            break;
        }
        case 0xC: {
            VibDataL* l = malloc(sizeof(*l));
            meta.typeName = vibTypeNames[1];
            fread(l, sizeof(*l), 1, f);
            data = l;
            data_schema = &schema_l;
            break;
        }
        case 0x10: {
            VibDataW* w = malloc(sizeof(*w));
            meta.typeName = vibTypeNames[2];
            fread(w, sizeof(*w), 1, f);
            data = w;
            data_schema = &schema_w;
            break;
        }
        default:
            fprintf(stderr, "Invalid vib type: %d\n", (int)type);
            fclose(f);
            return 1;
    }
    fclose(f);

    const cyaml_config_t config = {
        .log_level = CYAML_LOG_WARNING,
        .log_fn    = cyaml_log,
        .mem_fn    = cyaml_mem,
    };

    cyaml_err_t err;
    err = cyaml_save_file(out, &config, &schema_meta, &meta, 0);
    if (err != CYAML_OK)
    {
        fprintf(stderr, "CYAML error writing metadata: (%d) %s\n",
                err, cyaml_strerror(err));
        free(data);
        return 1;
    }

    err = cyaml_save_file(tmpname, &config, data_schema, data, 0);
    if (err != CYAML_OK)
    {
        fprintf(stderr, "CYAML error writing data: (%d) %s\n",
                err, cyaml_strerror(err));
        free(data);
        return 1;
    }

    FILE* in1 = fopen(tmpname, "r");
    FILE* outp = fopen(out, "a");
    char buf[4096];
    while (fgets(buf, sizeof(buf), in1))
        fputs(buf, outp);
    fclose(in1);
    fclose(outp);
    remove(tmpname);

    printf("OK: %s\n", argv[2]);
    free(data);
    return 0;
}

