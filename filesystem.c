#include "filesystem.h"

void main(void) {
#ifdef LOCAL
    _system_time = 0xdeadbeef;
#endif

    _fs_init();

    loop();
}

void _fs_init() {
    // Initialize FAT table to all zeros
    FAT_table = MALLOC(sizeof(uint16_t) * FAT_SIZE);
    MEMSET(FAT_table, 0, FAT_SIZE);

    // Initialize memory
    volume = MALLOC(VOLUME_SIZE);

    // Initialize root directory at 0
    current_dir = 0;

    uint8_t root_clstr[CLUSTER_SIZE];
    _fs_load_cluster(0, root_clstr);
    dir_t *root = DIR(root_clstr, 0);
    STRCPY(root->filename, DIR_THIS);
    root->attributes = ATTR_DIR;
    root->cluster = 0;
    root->size = CLUSTER_SIZE;
    root->create_time = _system_time;
    root->modified_time = _system_time;
    FAT_table[0] = CLSTR_END;

    // Mark the next directory location as free
    dir_t *next = DIR(root_clstr, 1);
    next->filename[0] = ENT_FREE;

    _fs_set_cluster(0, root_clstr);
}

void _fs_load_cluster(uint32_t cluster, uint8_t *buf) {
    if (cluster == CLSTR_END) {
        PRINT("Trying to load CLSTR_END\n");
    }
    MEMCPY(buf, (void *)(volume + cluster * CLUSTER_SIZE), CLUSTER_SIZE);
}

void _fs_set_cluster(uint32_t cluster, uint8_t *buf) {
    MEMCPY((void *)(volume + cluster * CLUSTER_SIZE), buf, CLUSTER_SIZE);
}

uint32_t _fs_get_free_clstr() {
    for (uint32_t i = 0; i < FAT_SIZE; ++i) {
        if (FAT_table[i] == CLSTR_EMPTY) {
            return i;
        }
    }
    // error!!
}

dir_t *_fs_find_by_name(uint8_t *dir_clstr, char *name) {
    dir_t *dir;
    for (int entry = 0; entry < MAX_ENTRIES; ++entry) {
        dir = DIR(dir_clstr, entry);

        if (dir->filename[0] == ENT_FREE) {
            break;
        } else if (dir->filename[0] == ENT_REMOVED) {
            continue;
        } else if (STRCMP(dir->filename, name) == 0) {
            return dir;
        }
    }

    return NULL;
}

void _fs_cleanup_table(uint32_t clstr_end) {
    while (FAT_table[clstr_end] != CLSTR_END) {
        int tmp;
        tmp = FAT_table[clstr_end];
        FAT_table[clstr_end] = CLSTR_EMPTY;
        clstr_end = tmp;
    }

    FAT_table[clstr_end] = CLSTR_EMPTY;
}

uint32_t _fs_load_dir(uint8_t *clstr, char *_path) {
    char path[512];
    STRCPY(path, _path);

    uint32_t working_clstr;
    working_clstr = (path != NULL && path[0] == '/') ? 0 : current_dir;

    _fs_load_cluster(working_clstr, clstr);

    char *curr = strtok(path, "/");

    dir_t *entry = DIR(clstr, 0);
    while (curr != NULL) {
        // curr is a directory
        entry = _fs_find_by_name(clstr, curr);

        if (entry == NULL) {
            return CLSTR_ERROR;
        } else if (IS_FILE(entry)) {
            return CLSTR_ERROR;
        }

        working_clstr = entry->cluster;
        curr = strtok(NULL, "/");

        _fs_load_cluster(working_clstr, clstr);
    }

    return working_clstr;
}

void _fs_split_path(char *path, char *dir, char *filename) {
    int last = -1;

    for (int i = 0; i < STRLEN(path); ++i) {
        if (path[i] == '/') last = i;
    }
    if (last < 0) {
        STRCPY(filename, path);
    } else {
        STRCPY(filename, path + last + 1);
        STRCPY(dir, path);
        dir[last] = '\0';
    }
}