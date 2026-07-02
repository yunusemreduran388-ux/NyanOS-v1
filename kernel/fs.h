#ifndef FS_H
#define FS_H

#include "../include/types.h"

#define FS_MAX_FILES     16
#define FS_MAX_NAME      32
#define FS_MAX_FILE_SIZE 4096

typedef struct {
    char name[FS_MAX_NAME];
    char data[FS_MAX_FILE_SIZE];
    uint32_t size;
    int used;
} fs_file_t;

void  fs_init(void);
int   fs_write(const char *name, const char *data, uint32_t len); /* create or overwrite */
int   fs_read(const char *name, char *out_buf, uint32_t max_len);  /* returns byte count, -1 if not found */
int   fs_delete(const char *name);
int   fs_exists(const char *name);
/* Returns the number of files and fills 'out' with their names (up to max_count). */
int   fs_list(char out[][FS_MAX_NAME], int max_count);

#endif
