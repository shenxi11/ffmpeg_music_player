#include "library.h"
#include <cstring>
#include <mysql_driver.h>
#include <mysql_connection.h>
#include <cppconn/statement.h>
#include <cppconn/prepared_statement.h>
#include <cppconn/resultset.h>
#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <queue>
#include <mutex>

// Connection Pool Class
class ConnectionPool {
private:
    std::string url, username, password;
    std::queue<std::shared_ptr<sql::Connection>> pool;
    std::mutex poolMutex;
    sql::mysql::MySQL_Driver* driver;

public:
    ConnectionPool(const std::string& dbUrl, const std::string& dbUser, const std::string& dbPassword, const std::string& dbName)
        : url(dbUrl), username(dbUser), password(dbPassword) {
        driver = sql::mysql::get_mysql_driver_instance();
        for (int i = 0; i < 10; ++i) {
            try {
                std::shared_ptr<sql::Connection> con(driver->connect(url, username, password));
                con->setSchema(dbName);
                pool.push(con);
            } catch (sql::SQLException& e) {
                std::cerr << "Connection error: " << e.what() << std::endl;
            }
        }
    }

    std::shared_ptr<sql::Connection> getConnection() {
        std::lock_guard<std::mutex> lock(poolMutex);
        if (pool.empty()) {
            try {
                std::shared_ptr<sql::Connection> con(driver->connect(url, username, password));
                return con;
            } catch (sql::SQLException& e) {
                std::cerr << "New connection error: " << e.what() << std::endl;
                return nullptr;
            }
        }
        
        auto con = pool.front();
        pool.pop();
        return con;
    }

    void releaseConnection(std::shared_ptr<sql::Connection> con) {
        if (con && con->isValid()) {
            std::lock_guard<std::mutex> lock(poolMutex);
            pool.push(con);
        }
    }
};
ConnectionPool pool("tcp://127.0.0.1:3306", "root", "shen2003125", "music_users");

std::shared_ptr<sql::Connection> sql_init() {
    try {
        std::shared_ptr<sql::Connection> con = pool.getConnection();
        if (con) {
            return con;
        } else {
            std::cerr << "No available connections in pool" << std::endl;
        }
    } catch (sql::SQLException& e) {
        std::cerr << "MySQL Error: " << e.what() << std::endl;
    }

    return nullptr;
}
bool user_login(std::shared_ptr<sql::Connection> con, const std::string password, const std::string account, std::string& username) {
    std::unique_ptr<sql::PreparedStatement> stmt(con->prepareStatement("SELECT username FROM users WHERE account = ? AND password = ?"));
    stmt->setString(1, account);
    stmt->setString(2, password);
    std::unique_ptr<sql::ResultSet> res(stmt->executeQuery());
    if(res->next()) {
        username = res->getString("username");
        return true;
    }
    return false;
}
bool user_register(std::shared_ptr<sql::Connection> con, const std::string password,  const std::string account, const std::string username) {
    // 首先检查是否已经存在该账户
    std::unique_ptr<sql::PreparedStatement> checkStmt(con->prepareStatement("SELECT * FROM users WHERE account = ?"));
    checkStmt->setString(1, account);
    std::unique_ptr<sql::ResultSet> res(checkStmt->executeQuery());

    if (res->next()) {
        return false;
    }

    std::unique_ptr<sql::PreparedStatement> insertStmt(con->prepareStatement("INSERT INTO users (account, password, username) VALUES (?, ?, ?)"));
    insertStmt->setString(1, account);
    insertStmt->setString(2, password);
    insertStmt->setString(3, username);

    insertStmt->executeUpdate();

    return true;
}
std::vector<std::string> user_musicpath_list(std::shared_ptr<sql::Connection> con, const std::string  username) {
    std::unique_ptr<sql::PreparedStatement> checkStmt(con->prepareStatement("SELECT music_path FROM user_path WHERE username = ?"));
    checkStmt->setString(1, username);
    std::unique_ptr<sql::ResultSet> res(checkStmt->executeQuery());
    std::vector<std::string> musicPaths;

    while (res->next()) {
        musicPaths.push_back(res->getString("music_path"));
    }
    return musicPaths;
}
bool insert_music(std::shared_ptr<sql::Connection> con, const std::string username, const std::string music_path) {
    std::unique_ptr<sql::PreparedStatement> checkStmt(con->prepareStatement("SELECT * FROM user_path WHERE username = ? and music_path = ?"));
    checkStmt->setString(1, username);
    checkStmt->setString(2, music_path);
    std::unique_ptr<sql::ResultSet> res(checkStmt->executeQuery());

    if(res->next()) {
        return false;
    }
    std::unique_ptr<sql::PreparedStatement> insertStmt(con->prepareStatement("INSERT INTO user_path (username, music_path) VALUES (?, ?)"));
    insertStmt->setString(1, username);
    insertStmt->setString(2, music_path);

    insertStmt->executeUpdate();

    return true;
}

extern "C" {
    char* get_records() {
        try {
            std::shared_ptr<sql::Connection> con = sql_init();
            if (!con) {
                return strdup("{\"error\":\"Database connection failed\"}");
            }
            
            std::unique_ptr<sql::PreparedStatement> stmt(con->prepareStatement("SELECT * FROM users LIMIT 10"));
            std::unique_ptr<sql::ResultSet> res(stmt->executeQuery());
            
            std::string json = "[";
            bool first = true;
            while (res->next()) {
                if (!first) json += ",";
                json += "{\"id\":" + std::to_string(res->getInt("id")) + 
                       ",\"name\":\"" + res->getString("username") + 
                       "\",\"value\":\"" + res->getString("account") + "\"}";
                first = false;
            }
            json += "]";
            
            pool.releaseConnection(con);
            return strdup(json.c_str());
        } catch (const std::exception& e) {
            return strdup("{\"error\":\"Query failed\"}");
        }
    }
    
    void add_record(const char* name, const char* value) {
        try {
            std::shared_ptr<sql::Connection> con = sql_init();
            if (con) {
                user_register(con, "default_password", value, name);
                pool.releaseConnection(con);
            }
        } catch (const std::exception& e) {
            // Log error silently
        }
    }
    
    char* user_login_c(const char* password, const char* account) {
        try {
            std::shared_ptr<sql::Connection> con = sql_init();
            if (!con) {
                return strdup("{\"error\":\"Database connection failed\"}");
            }
            
            std::string username;
            bool success = user_login(con, password, account, username);
            
            std::string json;
            if (success) {
                std::vector<std::string> songs = user_musicpath_list(con, username);
                json = "{\"success\":\"" + username + "\",\"song_path_list\":[";
                for (size_t i = 0; i < songs.size(); ++i) {
                    if (i > 0) json += ",";
                    json += "\"" + songs[i] + "\"";
                }
                json += "]}";
            } else {
                json = "{\"error\":\"Invalid credentials\"}";
            }
            
            pool.releaseConnection(con);
            return strdup(json.c_str());
        } catch (const std::exception& e) {
            return strdup("{\"error\":\"Login failed\"}");
        }
    }
    
    int user_register_c(const char* password, const char* account, const char* username) {
        try {
            std::shared_ptr<sql::Connection> con = sql_init();
            if (!con) {
                return 0;
            }
            
            bool success = user_register(con, password, account, username);
            pool.releaseConnection(con);
            return success ? 1 : 0;
        } catch (const std::exception& e) {
            return 0;
        }
    }
    
    char* user_musicpath_list_c(const char* username) {
        try {
            std::shared_ptr<sql::Connection> con = sql_init();
            if (!con) {
                return strdup("{\"error\":\"Database connection failed\"}");
            }
            
            std::vector<std::string> songs = user_musicpath_list(con, username);
            std::string json = "[";
            for (size_t i = 0; i < songs.size(); ++i) {
                if (i > 0) json += ",";
                json += "\"" + songs[i] + "\"";
            }
            json += "]";
            
            pool.releaseConnection(con);
            return strdup(json.c_str());
        } catch (const std::exception& e) {
            return strdup("{\"error\":\"Query failed\"}");
        }
    }
    
    int insert_music_c(const char* username, const char* music_path) {
        try {
            std::shared_ptr<sql::Connection> con = sql_init();
            if (!con) {
                return 0;
            }
            
            bool success = insert_music(con, username, music_path);
            pool.releaseConnection(con);
            return success ? 1 : 0;
        } catch (const std::exception& e) {
            return 0;
        }
    }
}