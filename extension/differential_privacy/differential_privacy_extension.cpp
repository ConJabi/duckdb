#define DUCKDB_EXTENSION_MAIN

#include "differential_privacy_extension.hpp"
#include "duckdb.hpp"
#include "functions.hpp"
#include "pragma.hpp"
#include "duckdb/main/connection_manager.hpp"
#include "duckdp_state.hpp"
#include "duckdp_extension_callback.hpp"


namespace duckdb {

    static void LoadInternal(DatabaseInstance &instance) {
    	auto &config = DBConfig::GetConfig(instance);
    	config.extension_callbacks.push_back(make_uniq<DuckDPExtensionCallback>());

    	for (auto &connection :
			ConnectionManager::Get(instance).GetConnectionList()) {
    		connection->registered_state->Insert(
				"duckdp", make_shared_ptr<DuckDPState>());
		}

    	// add the scalar and optimizer functions
		CoreFunctions::Register(instance);
    	CorePragma::Register(instance);

        // add a parser extension
        auto &db_config = duckdb::DBConfig::GetConfig(instance);
        auto DP_parser = duckdb::DPParserExtension();
        db_config.parser_extensions.push_back(DP_parser);

    }

    void DifferentialPrivacyExtension::Load(DuckDB &db) {
        LoadInternal(*db.instance);
    }
    std::string DifferentialPrivacyExtension::Name() {
        return "differential_privacy";
    }

} // namespace duckdb

extern "C" {

DUCKDB_EXTENSION_API void differential_privacy_init(duckdb::DatabaseInstance &db) {
    duckdb::DuckDB db_wrapper(db);
    db_wrapper.LoadExtension<duckdb::DifferentialPrivacyExtension>();

}

DUCKDB_EXTENSION_API const char *differential_privacy_version() {
    return duckdb::DuckDB::LibraryVersion();
}
}

#ifndef DUCKDB_EXTENSION_MAIN
#error DUCKDB_EXTENSION_MAIN not defined
#endif
