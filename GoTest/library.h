#ifndef LIBRARY_H
#define LIBRARY_H

#ifdef __cplusplus
extern "C" {
#endif

// C interface functions for Go
char* get_records();
void add_record(const char* name, const char* value);
int init_database(const char* host, const char* user, const char* password, const char* database);
void close_database();

// User management functions
char* user_login_c(const char* password, const char* account);
int user_register_c(const char* password, const char* account, const char* username);
char* user_musicpath_list_c(const char* username);
int insert_music_c(const char* username, const char* music_path);

#ifdef __cplusplus
}
#endif

#endif // LIBRARY_H