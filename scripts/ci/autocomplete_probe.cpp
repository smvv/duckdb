#include "duckdb.h"
#include "duckdb_static_extension.h"
#include <cstdio>
#include <string>

extern "C" int32_t duckdb_extension_core_functions_describe(duckdb_extension_descriptor *);
extern "C" int32_t duckdb_extension_loadable_extensions_describe(duckdb_extension_descriptor *);

int main(int argc, char **argv) {
    if (argc != 2 || duckdb_register_static_extension(duckdb_extension_core_functions_describe) ||
        duckdb_register_static_extension(duckdb_extension_loadable_extensions_describe)) {
        return 2;
    }
    duckdb_config config;
    duckdb_create_config(&config);
    duckdb_set_config(config, "allow_unsigned_extensions", "true");
    duckdb_set_config(config, "autoinstall_known_extensions", "true");
    duckdb_set_config(config, "autoload_known_extensions", "true");
    duckdb_set_config(config, "allow_extension_repositories", "allowed");
    duckdb_set_config(config, "threads", "1");
    duckdb_database db;
    char *error = nullptr;
    if (duckdb_open_ext(nullptr, &db, config, &error) != DuckDBSuccess) {
        fprintf(stderr, "open failed: %s\n", error ? error : "unknown");
        return 3;
    }
    duckdb_connection con;
    duckdb_connect(db, &con);
    std::string repository(argv[1]);
    for (int i = 0; i < 3; i++) {
        repository.resize(repository.rfind('/'));
    }
    const std::string queries[] = {
        "SET autoinstall_extension_repository='" + repository + "';",
        "INSTALL autocomplete FROM '" + repository + "';",
        "SELECT suggestion FROM sql_auto_complete('COPY tbl FROM ''file.csv'' HEAD');",
        "SELECT suggestion FROM sql_auto_complete('SHOW therm');",
        "CALL check_peg_parser('SHOW TABLES FROM db.main');",
        "CALL check_peg_parser('SELECT ARRAY(SELECT 42)');"
    };
    for (auto &sql : queries) {
        fprintf(stderr, "QUERY: %s\n", sql.c_str());
        fflush(stderr);
        duckdb_result result;
        if (duckdb_query(con, sql.c_str(), &result) != DuckDBSuccess) {
            fprintf(stderr, "query failed: %s\n", duckdb_result_error(&result));
            return 4;
        }
        fprintf(stderr, "ok: %llu rows\n", (unsigned long long)duckdb_row_count(&result));
        duckdb_destroy_result(&result);
    }
    duckdb_disconnect(&con);
    duckdb_close(&db);
    duckdb_destroy_config(&config);
    return 0;
}
