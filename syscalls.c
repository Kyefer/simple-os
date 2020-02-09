#include "syscalls.h"
#include "filesystem.h"

void _fs_touch(char *path) {
    if (path == NULL || STRLEN(path) == 0) {
        PRINT("Filename cannot be empty\n");
        return;
    }

    uint8_t clstr[CLUSTER_SIZE];

    char dir[512];
    char filename[MAX_FILENAME];
    _fs_split_path(path, dir, filename);

    uint32_t loaded_clstr = _fs_load_dir(clstr, dir);
    if (loaded_clstr == CLSTR_ERROR) {
        PRINTF("Unable to find directory '%s'\n", dir);
        return;
    }

    dir_t *existing = _fs_find_by_name(clstr, filename);
    if (existing != NULL) {
        PRINTF("File '%s' already exists\n", filename);
    }

    dir_t *new_file = NULL;
    for (int entry = 0; entry < MAX_ENTRIES; ++entry) {
        dir_t *dir = DIR(clstr, entry);

        if (dir->filename[0] == ENT_FREE || dir->filename[0] == ENT_REMOVED) {
            new_file = dir;
            break;
        }
    }

    if (new_file == NULL) {
        // cluster out of space
        PRINT("Directory out of space\n");
        return;
    }

    STRCPY(new_file->filename, filename);
    new_file->size = 0;
    new_file->cluster = CLSTR_END;
    new_file->attributes = 0;
    new_file->create_time = _system_time;
    new_file->modified_time = _system_time;

    _fs_set_cluster(loaded_clstr, clstr);
}

void _fs_mkdir(char *path) {
    uint8_t dir_clstr[CLUSTER_SIZE];
    char parent[512];
    char dirname[MAX_FILENAME];
    _fs_split_path(path, parent, dirname);

    uint32_t loaded_clstr = _fs_load_dir(dir_clstr, parent);
    if (loaded_clstr == CLSTR_ERROR) {
        PRINTF("Directory not found '%s'\n", path);
        return;
    }

    uint8_t entry;
    for (entry = 0; entry < MAX_ENTRIES; ++entry) {
        dir_t *next = DIR(dir_clstr, entry);

        if (next->filename[0] == 0 || next->filename[0] == ENT_REMOVED) {
            break;
        }
    }

    dir_t *new_dir = DIR(dir_clstr, entry);
    uint32_t free_clstr = _fs_get_free_clstr();

    STRCPY(new_dir->filename, dirname);
    new_dir->cluster = free_clstr;
    new_dir->attributes = ATTR_DIR;
    new_dir->size = CLUSTER_SIZE;
    new_dir->create_time = _system_time;
    new_dir->modified_time = _system_time;

    FAT_table[free_clstr] = CLSTR_END;
    _fs_set_cluster(loaded_clstr, dir_clstr);

    _fs_load_cluster(free_clstr, dir_clstr);

    dir_t *this = DIR(dir_clstr, 0);
    STRCPY(this->filename, DIR_THIS);
    this->cluster = free_clstr;
    this->attributes = ATTR_DIR;
    this->size = CLUSTER_SIZE;
    this->create_time = _system_time;
    this->modified_time = _system_time;

    dir_t *super = DIR(dir_clstr, 1);
    STRCPY(super->filename, DIR_PARENT);
    super->cluster = current_dir;
    super->attributes = ATTR_DIR;
    super->size = CLUSTER_SIZE;
    super->create_time = _system_time;
    super->modified_time = _system_time;

    // Mark next directory as free
    dir_t *next = DIR(dir_clstr, 2);
    next->filename[0] = 0;

    _fs_set_cluster(free_clstr, dir_clstr);
}

void _fs_ls(char *path) {
    uint8_t dir_clstr[CLUSTER_SIZE];
    uint32_t loaded_clstr = _fs_load_dir(dir_clstr, path);
    if (loaded_clstr == CLSTR_ERROR) {
        PRINTF("Directory not found '%s'\n", path);
        return;
    }
    
    dir_t *dir;
    for (int entry = 0; entry < MAX_ENTRIES; ++entry) {
        dir = DIR(dir_clstr, entry);

        if (dir->filename[0] == ENT_FREE) break;
        if (dir->filename[0] == ENT_REMOVED) continue;

        char name[15];
        STRCPY(name, dir->filename);
        if (dir->attributes & ATTR_DIR && STRCMP(name, DIR_THIS) && STRCMP(name, DIR_PARENT)) {
            sprintf(name, "%s/", name);
        }
        PRINTF("%-*s    %4x    %8x    %x", MAX_FILENAME + 1, name, dir->size, dir->modified_time, dir->cluster);
        if (!(dir->attributes & ATTR_DIR)) {
            uint32_t clstr = dir->cluster;
            while (clstr != CLSTR_END) {
                PRINTF("->%x", FAT_table[clstr]);
                clstr = FAT_table[clstr];
            }
        }

        PRINT("\n");
    }
}

void _fs_write(char *buf, int n, char *filename) {
    uint8_t clstr[CLUSTER_SIZE];
    _fs_load_cluster(current_dir, clstr);

    dir_t *file = _fs_find_by_name(clstr, filename);

    if (file == NULL) {
        PRINTF("File not found \"%s\"\n", filename);
        return;
    }

    // Allocate first cluster if needed
    if (file->cluster == CLSTR_END) {
        uint32_t new = _fs_get_free_clstr();
        // Check new
        file->cluster = new;
        FAT_table[new] = CLSTR_END;
    }

    file->size = n;
    file->modified_time = _system_time;
    // Update file
    _fs_set_cluster(current_dir, clstr);

    uint32_t file_clstr = file->cluster;
    for (int i = 0; n > 0; ++i) {
        _fs_load_cluster(file_clstr, clstr);
        MEMCPY(clstr, buf + i * CLUSTER_SIZE, n > CLUSTER_SIZE ? CLUSTER_SIZE : n);
        _fs_set_cluster(file_clstr, clstr);

        n -= CLUSTER_SIZE;

        if (n > 0 && FAT_table[file_clstr] == CLSTR_END) {
            uint32_t new = _fs_get_free_clstr();
            FAT_table[new] = CLSTR_END;
            FAT_table[file_clstr] = new;
        }
        file_clstr = FAT_table[file_clstr];
    }

    // Clean up
    if (file_clstr != CLSTR_END) {
        uint32_t extra = FAT_table[file_clstr];
        FAT_table[file_clstr] = CLSTR_END;
        _fs_cleanup_table(extra);
    }
}

void _fs_read(char *buf, int n, char *filename) {
    uint8_t clstr[CLUSTER_SIZE];
    _fs_load_cluster(current_dir, clstr);

    dir_t *file = _fs_find_by_name(clstr, filename);
    if (file == NULL) {
        PRINTF("File not found '%s'\n", filename);
        return;
    }

    uint32_t file_clstr = file->cluster;
    for (int i = 0; n > 0; ++i, n -= CLUSTER_SIZE) {
        _fs_load_cluster(file_clstr, clstr);
        MEMCPY(buf + i * CLUSTER_SIZE, clstr, n > CLUSTER_SIZE ? CLUSTER_SIZE : n);
        file_clstr = FAT_table[file_clstr];
    }
}

void _fs_cd(char *path) {
    uint8_t clstr[CLUSTER_SIZE];
    uint32_t loaded_clstr = _fs_load_dir(clstr, path);

    if (loaded_clstr == CLSTR_ERROR) {
        PRINTF("Directory not found '%s'\n", path);
        return;
    }

    current_dir = loaded_clstr;
}

int _fs_size(char *filename) {
    uint8_t clstr[CLUSTER_SIZE];
    _fs_load_cluster(current_dir, clstr);

    dir_t *file = _fs_find_by_name(clstr, filename);

    if (file != NULL)
        return file->size;
    else
        return -1;
}

void _fs_rm(char *filename) {
    uint8_t clstr[CLUSTER_SIZE];
    _fs_load_cluster(current_dir, clstr);

    dir_t *entry = _fs_find_by_name(clstr, filename);
    if (entry == NULL) {
        PRINTF("File not found '%s'\n", filename);
        return;
    }

    if (entry->attributes & ATTR_DIR) {
        if (IS_DOT_DIR(entry)) {
            PRINTF("Cannot delete the directory '%s'\n", entry->filename);
            return;
        }
        uint8_t dir_clstr[CLUSTER_SIZE];
        _fs_load_cluster(entry->cluster, dir_clstr);

        for (int entry = 0; entry < MAX_ENTRIES; ++entry) {
            dir_t *child = DIR(dir_clstr, entry);

            if (child->filename[0] == ENT_FREE) break;
            if (child->filename[0] == ENT_REMOVED) continue;

            if (!IS_DOT_DIR(child)) {
                PRINTF("Directory '%s' is not empty\n", filename);
                return;
            }
        }
    }

    entry->filename[0] = ENT_REMOVED;
    _fs_set_cluster(current_dir, clstr);
    _fs_cleanup_table(entry->cluster);
}