// #include "types.h"
#include "platform.h"

#define VOLUME_SIZE (1 << 20)  // MB
#define CLUSTER_SIZE 512       // 4KB
#define FAT_SIZE (VOLUME_SIZE / CLUSTER_SIZE)
#define MAX_ENTRIES (CLUSTER_SIZE / sizeof(dir_t))

#define CLSTR_EMPTY 0
#define CLSTR_END 0xFFF8
#define CLSTR_ERROR 0xFFFF

#define ATTR_DIR 0x10
#define ENT_FREE 0
#define ENT_REMOVED 0xE5

#define DIR_THIS "."
#define DIR_PARENT ".."

#define DIR(clstr, offset) (dir_t *)((clstr) + (offset) * sizeof(dir_t))
#define IS_DOT_DIR(dir) (STRCMP((dir)->filename, DIR_THIS) == 0 || STRCMP((dir)->filename, DIR_PARENT) == 0)
#define IS_FILE(entry) (!((entry)->attributes & ATTR_DIR))

#define MAX_FILENAME 15

uint32_t _system_time;

uint16_t *FAT_table;
uint8_t *volume;
uint32_t current_dir;

typedef struct dir {
    uint8_t filename[MAX_FILENAME];
    uint8_t attributes;
    uint32_t create_time;
    uint32_t modified_time;
    uint32_t cluster;
    uint32_t size;
} dir_t;

void _fs_init();
void _fs_load_cluster(uint32_t, uint8_t *);
void _fs_set_cluster(uint32_t, uint8_t *);
uint32_t _fs_get_free_clstr();
dir_t *_fs_find_by_name(uint8_t *, char *);
void _fs_cleanup_table(uint32_t);

uint32_t _fs_load_dir(uint8_t *, char *);
void _fs_split_path(char *, char *, char *);

void loop();