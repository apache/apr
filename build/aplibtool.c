/* ====================================================================
 * The Apache Software License, Version 1.1
 *
 * Copyright (c) 2000-2001 The Apache Software Foundation.  All rights
 * reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * 3. The end-user documentation included with the redistribution,
 *    if any, must include the following acknowledgment:
 *       "This product includes software developed by the
 *        Apache Software Foundation (http://www.apache.org/)."
 *    Alternately, this acknowledgment may appear in the software itself,
 *    if and wherever such third-party acknowledgments normally appear.
 *
 * 4. The names "Apache" and "Apache Software Foundation" must
 *    not be used to endorse or promote products derived from this
 *    software without prior written permission. For written
 *    permission, please contact apache@apache.org.
 *
 * 5. Products derived from this software may not be called "Apache",
 *    nor may "Apache" appear in their name, without prior written
 *    permission of the Apache Software Foundation.
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESSED OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE APACHE SOFTWARE FOUNDATION OR
 * ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 * ====================================================================
 *
 * This software consists of voluntary contributions made by many
 * individuals on behalf of the Apache Software Foundation.  For more
 * information on the Apache Software Foundation, please see
 * <http://www.apache.org/>.
 */

#include <stdio.h>
#include <process.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <dirent.h>

char silent = 0;
char shared = 0;
enum mode_t { mCompile, mLink };
enum output_type_t { otGeneral, otObject, otProgram, otStaticLibrary, otDynamicLibrary };

typedef char bool;
#define false 0
#define true (!false)

#ifdef __EMX__
#  define SHELL_CMD  "sh"
#  define CC         "gcc"
#  define SHARE_SW   "-Zdll -Zmtd"
#  define USE_OMF true
#  define TRUNCATE_DLL_NAME
#  define DYNAMIC_LIB_EXT "dll"
#  define EXE_EXT ".exe"

#  if USE_OMF
     /* OMF is the native format under OS/2 */
#    define STATIC_LIB_EXT "lib"
#    define OBJECT_EXT     "obj"
#    define LIBRARIAN      "emxomfar"
#  else
     /* but the alternative, a.out, can fork() which is sometimes necessary */
#    define STATIC_LIB_EXT "a"
#    define OBJECT_EXT     "o"
#    define LIBRARIAN      "ar"
#  endif
#endif


typedef struct {
    char *arglist[1024];
    int num_args;
    enum mode_t mode;
    enum output_type_t output_type;
    char *stub_name;
    char *tmp_dirs[1024];
    int num_tmp_dirs;
} cmd_data_t;

void parse_args(int argc, char *argv[], cmd_data_t *cmd_data);
bool parse_long_opt(char *arg, cmd_data_t *cmd_data);
bool parse_short_opt(char *arg, cmd_data_t *cmd_data);
bool parse_input_file_name(char *arg, cmd_data_t *cmd_data);
bool parse_output_file_name(char *arg, cmd_data_t *cmd_data);
void post_parse_fixup(cmd_data_t *cmd_data);
bool explode_static_lib(char *lib, cmd_data_t *cmd_data);
int execute_command(cmd_data_t *cmd_data);
char *shell_esc(const char *str);
void cleanup_tmp_dirs(cmd_data_t *cmd_data);


int main(int argc, char *argv[])
{
    int rc;
    cmd_data_t cmd_data;

    memset(&cmd_data, 0, sizeof(cmd_data));
    cmd_data.mode = mCompile;
    cmd_data.output_type = otGeneral;

    parse_args(argc, argv, &cmd_data);
    rc = execute_command(&cmd_data);

    if (rc == 0 && cmd_data.stub_name) {
        fopen(cmd_data.stub_name, "w");
    }

    cleanup_tmp_dirs(&cmd_data);
    return rc;
}



void parse_args(int argc, char *argv[], cmd_data_t *cmd_data)
{
    int a;
    char *arg;
    bool argused;

    for (a=1; a < argc; a++) {
        arg = argv[a];
        argused = false;

        if (arg[0] == '-') {
            if (arg[1] == '-') {
                argused = parse_long_opt(arg + 2, cmd_data);
            } else if (arg[1] == 'o' && a+1 < argc) {
                cmd_data->arglist[cmd_data->num_args++] = arg;
                arg = argv[++a];
                argused = parse_output_file_name(arg, cmd_data);
            } else {
                argused = parse_short_opt(arg + 1, cmd_data);
            }
        } else {
            argused = parse_input_file_name(arg, cmd_data);
        }

        if (!argused) {
            cmd_data->arglist[cmd_data->num_args++] = arg;
        }
    }

    post_parse_fixup(cmd_data);
}



bool parse_long_opt(char *arg, cmd_data_t *cmd_data)
{
    char *equal_pos = strchr(arg, '=');
    char var[50];
    char value[500];

    if (equal_pos) {
        strncpy(var, arg, equal_pos - arg);
        var[equal_pos - arg] = 0;
        strcpy(value, equal_pos + 1);
    } else {
        strcpy(var, arg);
    }

    if (strcmp(var, "silent") == 0) {
        silent = true;
    } else if (strcmp(var, "mode") == 0) {
        if (strcmp(value, "compile") == 0) {
            cmd_data->mode = mCompile;
            cmd_data->output_type = otObject;
        }
        if (strcmp(value, "link") == 0) {
            cmd_data->mode = mLink;
        }
    } else if (strcmp(var, "shared") == 0) {
        shared = true;
    } else {
        return false;
    }

    return true;
}



bool parse_short_opt(char *arg, cmd_data_t *cmd_data)
{
    if (strcmp(arg, "export-dynamic") == 0) {
        return true;
    }

    if (strcmp(arg, "module") == 0) {
        return true;
    }

    if (strcmp(arg, "Zexe") == 0) {
        return true;
    }

    if (strcmp(arg, "avoid-version") == 0) {
        return true;
    }

    return false;
}



bool parse_input_file_name(char *arg, cmd_data_t *cmd_data)
{
    char *ext = strrchr(arg, '.');
    char *name = strrchr(arg, '/');
    int pathlen;
    char *newarg;

    if (!ext) {
        return false;
    }

    ext++;

    if (name == NULL) {
        name = strrchr(arg, '\\');

        if (name == NULL) {
            name = arg;
        } else {
            name++;
        }
    } else {
        name++;
    }

    pathlen = name - arg;

    if (strcmp(ext, "lo") == 0) {
        newarg = (char *)malloc(strlen(arg) + 10);
        strcpy(newarg, arg);
        strcpy(newarg + (ext - arg), OBJECT_EXT);
        cmd_data->arglist[cmd_data->num_args++] = newarg;
        return true;
    }

    if (strcmp(ext, "la") == 0) {
        newarg = (char *)malloc(strlen(arg) + 10);
        strcpy(newarg, arg);
        newarg[pathlen] = 0;
        strcat(newarg, ".libs/");

        if (strncmp(name, "lib", 3) == 0) {
            name += 3;
        }

        strcat(newarg, name);
        ext = strrchr(newarg, '.') + 1;
        strcpy(ext, STATIC_LIB_EXT);
        cmd_data->arglist[cmd_data->num_args++] = newarg;
        return true;
    }

    if (strcmp(ext, "c") == 0) {
        if (cmd_data->stub_name == NULL) {
            cmd_data->stub_name = (char *)malloc(strlen(arg) + 4);
            strcpy(cmd_data->stub_name, arg);
            strcpy(strrchr(cmd_data->stub_name, '.') + 1, shared ? "slo" : "lo");
        }
    }

    if (strcmp(name, CC) == 0 || strcmp(name, CC EXE_EXT) == 0) {
        if (cmd_data->output_type == otGeneral) {
            cmd_data->output_type = otObject;
        }
    }

    return false;
}



bool parse_output_file_name(char *arg, cmd_data_t *cmd_data)
{
    char *name = strrchr(arg, '/');
    char *ext = strrchr(arg, '.');
    char *newarg = NULL, *newext;
    int pathlen;

    if (name == NULL) {
        name = strrchr(arg, '\\');

        if (name == NULL) {
            name = arg;
        } else {
            name++;
        }
    } else {
        name++;
    }

    if (!ext) {
        cmd_data->stub_name = arg;
        cmd_data->output_type = otProgram;
        newarg = (char *)malloc(strlen(arg) + 5);
        strcpy(newarg, arg);
        strcat(newarg, EXE_EXT);
        cmd_data->arglist[cmd_data->num_args++] = newarg;
        return true;
    }

    ext++;
    pathlen = name - arg;

    if (strcmp(ext, "la") == 0) {
        cmd_data->stub_name = arg;
        cmd_data->output_type = shared ? otDynamicLibrary : otStaticLibrary;
        newarg = (char *)malloc(strlen(arg) + 10);
        mkdir(".libs", 0);
        strcpy(newarg, ".libs/");

        if (strncmp(arg, "lib", 3) == 0) {
            arg += 3;
        }

        strcat(newarg, arg);
        newext = strrchr(newarg, '.') + 1;
        strcpy(newext, shared ? DYNAMIC_LIB_EXT : STATIC_LIB_EXT);
        cmd_data->arglist[cmd_data->num_args++] = newarg;

#ifdef TRUNCATE_DLL_NAME
        if (shared) {
            /* Cut DLL name down to 8 characters after removing any mod_ prefix */
            int len = ext - name - 1;
            char *newname = strrchr(newarg, '/') + 1;

            if (strncmp(newname, "mod_", 4) == 0) {
                strcpy(newname, newname + 4);
                len -= 4;
            }

            if (len > 8) {
                strcpy(newname + 8, strchr(newname, '.'));
            }
        }
#endif

        return true;
    }

    if (strcmp(ext, "lo") == 0) {
        cmd_data->stub_name = arg;
        cmd_data->output_type = otObject;
        newarg = (char *)malloc(strlen(newarg) + 2);
        strcpy(newarg, arg);
        ext = strrchr(newarg, '.') + 1;
        strcpy(ext, OBJECT_EXT);
        cmd_data->arglist[cmd_data->num_args++] = newarg;
        return true;
    }

    return false;
}



void post_parse_fixup(cmd_data_t *cmd_data)
{
    int a;
    char *arg;
    char *ext;

    if (cmd_data->output_type == otStaticLibrary && cmd_data->mode == mLink) {
        /* We do a real hatchet job on the args when making a static library
         * removing all compiler switches & any other cruft that ar won't like
         * We also need to explode any libraries listed
         */

        for (a=0; a < cmd_data->num_args; a++) {
            arg = cmd_data->arglist[a];

            if (arg) {
                ext = strrchr(arg, '.');

                if (ext) {
                    ext++;
                }

                if (arg[0] == '-') {
                    cmd_data->arglist[a] = NULL;

                    if (strcmp(arg, "-rpath") == 0 && a+1 < cmd_data->num_args) {
                        cmd_data->arglist[a+1] = NULL;
                    }

                    if (strcmp(arg, "-R") == 0 && a+1 < cmd_data->num_args) {
                        cmd_data->arglist[a+1] = NULL;
                    }

                    if (strcmp(arg, "-version-info") == 0 && a+1 < cmd_data->num_args) {
                        cmd_data->arglist[a+1] = NULL;
                    }

                    if (strcmp(arg, "-o") == 0) {
                        a++;
                    }
                }

                if (strcmp(arg, CC) == 0 || strcmp(arg, CC EXE_EXT) == 0) {
                    cmd_data->arglist[a] = LIBRARIAN " cr";
                }

                if (ext) {
                    if (strcmp(ext, "h") == 0 || strcmp(ext, "c") == 0) {
                      /* ignore source files, they don't belong in a library */
                        cmd_data->arglist[a] = NULL;
                    }

                    if (strcmp(ext, STATIC_LIB_EXT) == 0) {
                        cmd_data->arglist[a] = NULL;
                        explode_static_lib(arg, cmd_data);
                    }
                }
            }
        }
    }

    if (cmd_data->output_type == otDynamicLibrary) {
        for (a=0; a < cmd_data->num_args; a++) {
            arg = cmd_data->arglist[a];

            if (arg) {
                if (strcmp(arg, "-rpath") == 0 && a+1 < cmd_data->num_args) {
                    cmd_data->arglist[a] = NULL;
                    cmd_data->arglist[a+1] = NULL;
                }
            }
        }
    }

#if USE_OMF
    if (cmd_data->output_type == otObject ||
        cmd_data->output_type == otProgram ||
        cmd_data->output_type == otDynamicLibrary) {
        cmd_data->arglist[cmd_data->num_args++] = "-Zomf";
    }
#endif

    if (shared) {
        cmd_data->arglist[cmd_data->num_args++] = SHARE_SW;
    }
}



int execute_command(cmd_data_t *cmd_data)
{
    int target = 0;
    char *command;
    int a, total_len = 0;
    char *args[4];

    for (a=0; a < cmd_data->num_args; a++) {
        if (cmd_data->arglist[a]) {
            total_len += strlen(cmd_data->arglist[a]) + 1;
        }
    }

    command = (char *)malloc( total_len );
    command[0] = 0;

    for (a=0; a < cmd_data->num_args; a++) {
        if (cmd_data->arglist[a]) {
            strcat(command, cmd_data->arglist[a]);
            strcat(command, " ");
        }
    }

    command[strlen(command)-1] = 0;

    if (!silent) {
        puts(command);
    }

    cmd_data->num_args = target;
    cmd_data->arglist[cmd_data->num_args] = NULL;
    command = shell_esc(command);

    args[0] = SHELL_CMD;
    args[1] = "-c";
    args[2] = command;
    args[3] = NULL;
    return spawnvp(P_WAIT, args[0], args);
}



char *shell_esc(const char *str)
{
    char *cmd;
    unsigned char *d;
    const unsigned char *s;

    cmd = (char *)malloc(2 * strlen(str) + 1);
    d = (unsigned char *)cmd;
    s = (const unsigned char *)str;

    for (; *s; ++s) {
        if (*s == '"' || *s == '\\') {
	    *d++ = '\\';
	}
	*d++ = *s;
    }

    *d = '\0';
    return cmd;
}



bool explode_static_lib(char *lib, cmd_data_t *cmd_data)
{
    char tmpdir[1024];
    char savewd[1024];
    char cmd[1024];
    char *name;
    DIR *dir;
    struct dirent *entry;

    strcpy(tmpdir, lib);
    strcat(tmpdir, ".exploded");

    mkdir(tmpdir, 0);
    cmd_data->tmp_dirs[cmd_data->num_tmp_dirs++] = strdup(tmpdir);
    getcwd(savewd, sizeof(savewd));

    if (chdir(tmpdir) != 0)
        return false;

    strcpy(cmd, LIBRARIAN " x ");
    name = strrchr(lib, '/');

    if (name) {
        name++;
    } else {
        name = lib;
    }

    strcat(cmd, "../");
    strcat(cmd, name);
    system(cmd);
    chdir(savewd);
    dir = opendir(tmpdir);

    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_name[0] != '.') {
            strcpy(cmd, tmpdir);
            strcat(cmd, "/");
            strcat(cmd, entry->d_name);
            cmd_data->arglist[cmd_data->num_args++] = strdup(cmd);
        }
    }

    closedir(dir);
    return true;
}



void cleanup_tmp_dir(char *dirname)
{
    DIR *dir;
    struct dirent *entry;
    char fullname[1024];

    dir = opendir(dirname);

    if (dir == NULL)
        return;

    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_name[0] != '.') {
            strcpy(fullname, dirname);
            strcat(fullname, "/");
            strcat(fullname, entry->d_name);
            remove(fullname);
        }
    }

    rmdir(dirname);
}



void cleanup_tmp_dirs(cmd_data_t *cmd_data)
{
    int d;

    for (d=0; d < cmd_data->num_tmp_dirs; d++) {
        cleanup_tmp_dir(cmd_data->tmp_dirs[d]);
    }
}
