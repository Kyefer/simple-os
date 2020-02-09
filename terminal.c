#include "filesystem.h"
#include "syscalls.h"

void init() {
    char test_file[] = "test_file";
    char touch[] = "dir/test";
    char test_dir[] = "dir";

    _fs_touch(test_file);
    _fs_mkdir(test_dir);
    // _fs_touch(touch);

    char data[] = "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Eu scelerisque felis imperdiet proin. Tristique risus nec feugiat in fermentum. Neque laoreet suspendisse interdum consectetur libero id faucibus. Non curabitur gravida arcu ac tortor dignissim convallis aenean et. Massa tincidunt dui ut ornare lectus sit amet. Rhoncus dolor purus non enim praesent elementum. Facilisis volutpat est velit egestas dui. Elementum pulvinar etiam non quam lacus suspendisse faucibus. Ipsum a arcu cursus vitae congue mauris rhoncus. Id nibh tortor id aliquet lectus proin nibh. Sodales ut eu sem integer. Sit amet justo donec enim diam vulputate ut pharetra sit. Semper feugiat nibh sed pulvinar proin gravida. Non quam lacus suspendisse faucibus interdum posuere lorem ipsum. Tristique sollicitudin nibh sit amet commodo nulla facilisi nullam. Nisi quis eleifend quam adipiscing vitae proin sagittis nisl rhoncus. Scelerisque eleifend donec pretium vulputate. Donec massa sapien faucibus et molestie ac feugiat sed lectus. In hendrerit gravida rutrum quisque non tellus. Magna ac placerat vestibulum lectus. Sed libero enim sed faucibus. Massa sapien faucibus et molestie ac feugiat sed lectus. Cras sed felis eget velit aliquet sagittis id. Donec ultrices tincidunt arcu non sodales neque sodales. Morbi tincidunt ornare massa eget egestas purus viverra. Et molestie ac feugiat sed. Odio euismod lacinia at quis risus sed. Donec adipiscing tristique risus nec feugiat in fermentum posuere. Est velit egestas dui id ornare arcu odio ut. Magna fermentum iaculis eu non diam phasellus. Quam elementum pulvinar etiam non. Amet porttitor eget dolor morbi non arcu risus. Purus in massa tempor nec feugiat nisl pretium fusce id. Ac felis donec et odio pellentesque diam. At augue eget arcu dictum varius duis at consectetur. Massa tempor nec feugiat nisl pretium. Id volutpat lacus laoreet non curabitur. Sed faucibus turpis in eu mi bibendum neque egestas. Vitae suscipit tellus mauris a. Ut sem viverra aliquet eget sit amet tellus cras adipiscing. Scelerisque viverra mauris in aliquam. Ut aliquam purus sit amet luctus venenatis. Nec ullamcorper sit amet risus nullam eget felis eget nunc. At ultrices mi tempus imperdiet nulla malesuada pellentesque. Sollicitudin nibh sit amet commodo nulla facilisi.";
    // char out[4096];
    _fs_write(data, STRLEN(data) + 1, test_file);

    // print_fat();
    // // _fs_ls(0);
    // // printf("%s\n", out);
    // char data2[] = "test post please ignore";
    // _fs_write(data2, STRLEN(data2), test_file);
    // _fs_read(out, STRLEN(data2), test_file);
    // print_fat();
    // printf("%s\n", out);
    // _fs_ls(0);
}

void loop() {
    init();

    char *line = MALLOC(32);
    size_t bufsize = 32;

    int done = 0;
    while (!done) {
        PRINT("> ");
        getline(&line, &bufsize, stdin);

        line = strtok(line, "\n");
        char *cmd = strtok(line, " ");

        if (cmd == NULL) continue;

        if (STRCMP(cmd, "e") == 0 || STRCMP(cmd, "exit") == 0) {
            done = 1;
        } else if (STRCMP(cmd, "l") == 0 || STRCMP(cmd, "ls") == 0) {
            _fs_ls(strtok(NULL, " "));
        } else if (STRCMP(cmd, "t") == 0 || STRCMP(cmd, "touch") == 0) {
            char *filename = strtok(NULL, " ");
            // if (filename != NULL)
            _fs_touch(filename);
        } else if (STRCMP(cmd, "d") == 0 || STRCMP(cmd, "mkdir") == 0) {
            char *dirname = strtok(NULL, " ");
            if (dirname != NULL)
                _fs_mkdir(dirname);
        } else if (STRCMP(cmd, "c") == 0 || STRCMP(cmd, "cd") == 0) {
            char *dirname = strtok(NULL, " ");
            if (dirname != NULL)
                _fs_cd(dirname);
        } else if (STRCMP(cmd, "read") == 0 || STRCMP(cmd, "cat") == 0) {
            char *filename = strtok(NULL, " ");
            if (filename != NULL) {
                int len = _fs_size(filename);
                if (len > 0) {
                    char *buf = MALLOC(len);
                    _fs_read(buf, len + 1, filename);
                    PRINTF("%s\n", buf);
                    FREE(buf);
                }
            }
        } else if (STRCMP(cmd, "rm") == 0) {
            char *filename = strtok(NULL, " ");
            if (filename != NULL) {
                _fs_rm(filename);
            }
        } else if (STRCMP(cmd, "wr") == 0) {
            char *filename = strtok(NULL, " ");
            if (filename != NULL) {
                char *text = strtok(NULL, "");
                _fs_write(text, STRLEN(text) + 1, filename);
            }
        } else if (STRCMP(cmd, "cp") == 0) {
            // char *src = strtok(NULL, " ");
            // char *dst = strtok(NULL, " ");

            // uint8_t src_clstr[CLUSTER_SIZE];
            // uint8_t dst_clstr[CLUSTER_SIZE];
            // dir_t *entry = _fs_get_entry(src_clstr, src);
            // if (entry == NULL || entry->attributes & ATTR_DIR) {
            //     PRINT("src cannot be a directory\n");
            //     return;
            // }

        } else if (STRCMP(cmd, "p") == 0) {
            char *path = strtok(NULL, " ");
            char *dir = malloc(512);
            char *filename = malloc(16);
            _fs_split_path(path, dir, filename);
            PRINTF("%s\n", dir);
            PRINTF("%s\n", filename);
            // if (path != NULL) {
            // uint8_t clstr[CLUSTER_SIZE];
            // dir_t *entry = _fs_get_entry(clstr, path);
            // PRINTF("%s\n", entry->filename);
            // }
        } else {
            PRINTF("Unknown command '%s'\n", cmd);
        }
    }
    FREE(line);
}