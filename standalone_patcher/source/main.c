#include <stdlib.h>
#include <stdio.h>
#include "headers.h"
#include "patch.h"
#include "firm.h"

#define check(condition, message, ...) if (!(condition)) { fprintf(stderr, message "\n", ##__VA_ARGS__); goto error; }

firm_h *firm_loc = NULL;
struct firm_signature *current_firm = NULL;
firm_h *agb_firm_loc = NULL;
struct firm_signature *current_agb_firm = NULL;

void *load_file(char *path, long *size)
{
    if (!size) {
        long local_size;
        size = &local_size;
    }

    void *mem = NULL;

    FILE *fp = fopen(path, "r");
    if (!fp) return NULL;

    if (fseek(fp, 0, SEEK_END) != 0) goto error;
    *size = ftell(fp);
    if (*size == -1) return NULL;
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

int write_file(char *path, void *mem, long size)
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
    long firm_size, agb_firm_size;

    check(argc > 2, "Usage: %s <cake file> <NATIVE_FIRM> [AGB_FIRM] [TWL_FIRM]", argv[0]);

    cake = load_file(argv[1], NULL);
    check(cake, "Failed to load cake: %s", argv[1]);

    firm_loc = load_file(argv[2], &firm_size);
    check(firm_loc, "Failed to load NATIVE_FIRM: %s", argv[2]);
    current_firm = get_firm_info(firm_loc, firm_signatures);
    check(current_firm, "Unsupported NATIVE_FIRM: %s", argv[2]);

    if (argc > 3) {
        agb_firm_loc = load_file(argv[3], &agb_firm_size);
        check(agb_firm_loc, "Failed to load AGB_FIRM: %s", argv[3]);
        current_agb_firm = get_firm_info(agb_firm_loc, agb_firm_signatures);
        check(current_agb_firm, "Unsupported AGB_FIRM: %s", argv[3]);
    }

    check(patch_firm(cake) == 0, "Failed to apply cake");

    check(write_file(argv[2], firm_loc, firm_size) == 0, "Failed to write NATIVE_FIRM: %s", argv[2]);

    if (argc > 3) {
        check(write_file(argv[3], agb_firm_loc, agb_firm_size) == 0, "Failed to write AGB_FIRM: %s", argv[3]);
    }

    goto cleanup;

error:
    rc = 1;

cleanup:
    if (cake) free(cake);
    if (firm_loc) free(firm_loc);
    if (agb_firm_loc) free(agb_firm_loc);

    return rc;
}
