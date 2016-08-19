#include <stdlib.h>
#include <stdio.h>
#include "headers.h"
#include "patch.h"
#include "firm.h"
#include "fcram.h"

#define check(condition, message, ...) if (!(condition)) { fprintf(stderr, message "\n", ##__VA_ARGS__); goto error; }

void *load_file(char *path, size_t *size)
{
    if (!size) {
        size_t local_size;
        size = &local_size;
    }

    void *mem = NULL;

    FILE *fp = fopen(path, "r");
    if (!fp) return NULL;

    if (fseek(fp, 0, SEEK_END) != 0) goto error;
    long tmp_size = ftell(fp);
    if (tmp_size == -1) return NULL;
    *size = (size_t)tmp_size;
    if (fseek(fp, 0, SEEK_SET) != 0) goto error;

    mem = malloc(*size);
    if (!mem) goto error;

    if (fread(mem, *size, 1, fp) != 1) goto error;
    if (fclose(fp) != 0) goto error;

    return mem;

error:
    if (fp) fclose(fp);
    if (mem) free(mem);

    return NULL;
}

int write_file(char *path, void *mem, size_t size)
{
    FILE *fp = fopen(path, "w");
    if (!fp) return 1;
    if (fwrite(mem, size, 1, fp) != 1) return 1;
    if (fclose(fp) != 0) return 1;
    return 0;
}

int main(int argc, char *argv[])
{
    int rc = 0;
    void *cake = NULL;
    size_t cake_size, firm_size, twl_firm_size, agb_firm_size;

    memory_loc = NULL;
    firm_loc = NULL;
    twl_firm_loc = NULL;
    agb_firm_loc = NULL;

    check(argc > 3, "Usage: %s <cake file> <memory file> <NATIVE_FIRM> [TWL_FIRM] [AGB_FIRM]", argv[0]);

    cake = load_file(argv[1], &cake_size);
    check(cake, "Failed to load cake: %s", argv[1]);

    memory_loc = malloc(FCRAM_SPACING);
    check(memory_loc, "Failed to allocate memory");

    firm_loc = load_file(argv[3], &firm_size);
    check(firm_loc, "Failed to load NATIVE_FIRM: %s", argv[3]);
    current_firm = get_firm_info(firm_loc, firm_signatures);
    check(current_firm, "Unsupported NATIVE_FIRM: %s", argv[3]);

    if (argc > 4) {
        twl_firm_loc = load_file(argv[4], &twl_firm_size);
        check(twl_firm_loc, "Failed to load TWL_FIRM: %s", argv[5]);
        current_twl_firm = get_firm_info(twl_firm_loc, twl_firm_signatures);
        check(current_twl_firm, "Unsupported TWL_FIRM: %s", argv[5]);

        if (argc > 5) {
            agb_firm_loc = load_file(argv[5], &agb_firm_size);
            check(agb_firm_loc, "Failed to load AGB_FIRM: %s", argv[5]);
            current_agb_firm = get_firm_info(agb_firm_loc, agb_firm_signatures);
            check(current_agb_firm, "Unsupported AGB_FIRM: %s", argv[5]);
        }
    }

    patch_reset();
    check(patch_firm(cake, cake_size) == 0, "Failed to apply cake");

    check(write_file(argv[2], memory_loc, *memory_loc) == 0, "Failed to write memory file: %s", argv[2]);

    check(write_file(argv[3], firm_loc, firm_size) == 0, "Failed to write NATIVE_FIRM: %s", argv[3]);

    if (argc > 4) {
        check(write_file(argv[4], twl_firm_loc, twl_firm_size) == 0, "Failed to write TWL_FIRM: %s", argv[4]);

        if (argc > 5) {
            check(write_file(argv[5], agb_firm_loc, agb_firm_size) == 0, "Failed to write AGB_FIRM: %s", argv[5]);
        }
    }

    goto cleanup;

error:
    rc = 1;

cleanup:
    if (cake) free(cake);
    if (memory_loc) free(memory_loc);
    if (firm_loc) free(firm_loc);
    if (twl_firm_loc) free(twl_firm_loc);
    if (agb_firm_loc) free(agb_firm_loc);

    return rc;
}
