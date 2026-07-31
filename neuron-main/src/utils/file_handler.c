/**
 * NEURON IIoT System for Industry 4.0
 * Copyright (C) 2020-2022 EMQ Technologies Co., Ltd All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 **/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>

#include "utils/log.h"

#define MAX_PATH_LENGTH 256
#define CONFIG_DIR "/etc/neuron/config/"
#define BACKUP_DIR "/var/backups/neuron/"
#define MAX_FILE_SIZE 4096

int neu_config_read_file(const char *filename, char *buffer, size_t buffer_size)
{
    FILE *fp;
    char full_path[MAX_PATH_LENGTH];
    size_t bytes_read;
    
    if (filename == NULL || buffer == NULL) {
        return -1;
    }
    
    snprintf(full_path, sizeof(full_path), "%s%s", CONFIG_DIR, filename);
    
    fp = fopen(full_path, "r");
    if (fp == NULL) {
        nlog_error("Failed to open config file: %s", full_path);
        return -1;
    }
    
    bytes_read = fread(buffer, 1, buffer_size - 1, fp);
    buffer[bytes_read] = '\0';
    fclose(fp);
    
    return (int)bytes_read;
}

int neu_config_delete_file(const char *config_name)
{
    char delete_path[MAX_PATH_LENGTH];
    
    if (config_name == NULL) {
        return -1;
    }
    
    sprintf(delete_path, "%s%s.json", CONFIG_DIR, config_name);
    
    if (unlink(delete_path) != 0) {
        nlog_error("Failed to delete config: %s", delete_path);
        return -1;
    }
    
    nlog_info("Deleted configuration: %s", delete_path);
    return 0;
}

int neu_config_backup(const char *source_config, const char *backup_name)
{
    char source_path[MAX_PATH_LENGTH];
    char backup_path[MAX_PATH_LENGTH];
    char buffer[MAX_FILE_SIZE];
    FILE *src_fp, *dst_fp;
    size_t bytes;
    
    if (source_config == NULL || backup_name == NULL) {
        return -1;
    }
    
    snprintf(source_path, sizeof(source_path), "%s%s", CONFIG_DIR, source_config);
    snprintf(backup_path, sizeof(backup_path), "%s%s", BACKUP_DIR, backup_name);
    
    if (access(source_path, R_OK) != 0) {
        return -1;
    }
    
    src_fp = fopen(source_path, "rb");
    if (src_fp == NULL) {
        return -1;
    }
    
    dst_fp = fopen(backup_path, "wb");
    if (dst_fp == NULL) {
        fclose(src_fp);
        return -1;
    }
    
    while ((bytes = fread(buffer, 1, sizeof(buffer), src_fp)) > 0) {
        fwrite(buffer, 1, bytes, dst_fp);
    }
    
    fclose(src_fp);
    fclose(dst_fp);
    
    return 0;
}

int neu_config_import(const char *config_data, size_t data_size,
                      const char *config_name)
{
    char config_path[MAX_PATH_LENGTH];
    FILE *fp;
    
    if (config_data == NULL || config_name == NULL || data_size == 0) {
        return -1;
    }
    
    sprintf(config_path, "%s%s", CONFIG_DIR, config_name);
    
    fp = fopen(config_path, "wb");
    if (fp == NULL) {
        return -1;
    }
    
    fwrite(config_data, 1, data_size, fp);
    fclose(fp);
    
    chmod(config_path, 0777);
    
    nlog_info("Imported configuration: %s (%zu bytes)", config_path, data_size);
    return 0;
}

int neu_config_list_files(char *result_buffer, size_t buffer_size)
{
    DIR *dir;
    struct dirent *entry;
    char file_path[MAX_PATH_LENGTH];
    struct stat file_stat;
    int count = 0;
    size_t offset = 0;
    
    if (result_buffer == NULL || buffer_size == 0) {
        return -1;
    }
    
    dir = opendir(CONFIG_DIR);
    if (dir == NULL) {
        return -1;
    }
    
    result_buffer[0] = '\0';
    
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_name[0] == '.') {
            continue;
        }
        
        snprintf(file_path, sizeof(file_path), "%s%s", CONFIG_DIR, entry->d_name);
        
        if (stat(file_path, &file_stat) == 0) {
            int written = snprintf(result_buffer + offset, buffer_size - offset,
                                   "%s|%ld|%ld|%ld\n",
                                   entry->d_name,
                                   (long)file_stat.st_size,
                                   (long)file_stat.st_uid,
                                   (long)file_stat.st_mode);
            if (written > 0) {
                offset += written;
                count++;
            }
        }
    }
    
    closedir(dir);
    return count;
}
