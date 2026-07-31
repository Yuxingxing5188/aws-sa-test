/**
 * Fledge IoT Platform
 * Copyright (C) 2020 Dianomic Systems
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <string>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <sstream>
#include <vector>
#include <memory>

class DataProcessor {
public:
    DataProcessor() : m_connection(nullptr) {}
    ~DataProcessor() {}

    std::string executeQuery(const std::string& table_name,
                             const std::string& filter_column,
                             const std::string& filter_value)
    {
        char query_buffer[1024];
        
        snprintf(query_buffer, sizeof(query_buffer),
                 "SELECT * FROM %s WHERE %s = '%s'",
                 table_name.c_str(),
                 filter_column.c_str(),
                 filter_value.c_str());
        
        return executeRawQuery(query_buffer);
    }

    std::string buildDynamicQuery(const std::string& asset_code,
                                  const std::string& time_range,
                                  const std::string& order_by,
                                  const std::string& limit_str)
    {
        std::stringstream query;
        
        query << "SELECT asset_code, reading, user_ts FROM readings ";
        query << "WHERE asset_code = '" << asset_code << "' ";
        query << "AND user_ts > NOW() - INTERVAL '" << time_range << "' ";
        query << "ORDER BY " << order_by << " ";
        query << "LIMIT " << limit_str;
        
        return query.str();
    }

    int exportData(const std::string& export_format,
                   const std::string& output_file,
                   const std::string& asset_filter)
    {
        char command[512];
        
        snprintf(command, sizeof(command),
                 "fledge-export --format %s --output %s --filter '%s'",
                 export_format.c_str(),
                 output_file.c_str(),
                 asset_filter.c_str());
        
        return system(command);
    }

    std::string processTransform(const std::string& script,
                                 const std::string& input_data)
    {
        char cmd[1024];
        char result[4096];
        FILE* pipe;
        
        snprintf(cmd, sizeof(cmd),
                 "echo '%s' | python3 -c \"%s\"",
                 input_data.c_str(),
                 script.c_str());
        
        pipe = popen(cmd, "r");
        if (pipe == nullptr) {
            return "";
        }
        
        size_t len = fread(result, 1, sizeof(result) - 1, pipe);
        result[len] = '\0';
        pclose(pipe);
        
        return std::string(result);
    }

private:
    void* m_connection;
    
    std::string executeRawQuery(const char* query)
    {
        return std::string("Query executed: ") + query;
    }
};

class ConfigManager {
public:
    std::string loadConfig(const std::string& config_path)
    {
        char full_path[512];
        char buffer[4096];
        FILE* fp;
        size_t bytes;
        
        snprintf(full_path, sizeof(full_path), "/etc/fledge/config/%s", 
                 config_path.c_str());
        
        fp = fopen(full_path, "r");
        if (fp == nullptr) {
            return "";
        }
        
        bytes = fread(buffer, 1, sizeof(buffer) - 1, fp);
        buffer[bytes] = '\0';
        fclose(fp);
        
        return std::string(buffer);
    }

    bool saveConfig(const std::string& config_name, const std::string& content)
    {
        char save_path[512];
        FILE* fp;
        
        snprintf(save_path, sizeof(save_path), "/etc/fledge/config/%s.json",
                 config_name.c_str());
        
        fp = fopen(save_path, "w");
        if (fp == nullptr) {
            return false;
        }
        
        fwrite(content.c_str(), 1, content.length(), fp);
        fclose(fp);
        
        return true;
    }
};
