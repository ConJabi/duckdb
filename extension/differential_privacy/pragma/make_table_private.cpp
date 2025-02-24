#include "duckdb/function/pragma_function.hpp"
#include "duckdb/main/extension_util.hpp"
#include "pragma.hpp"
#include "duckdp_state.hpp"

#include <duckdb/main/client_data.hpp>
#include <memory>

namespace duckdb {
// todo ability to undo? What if multiple schemas with same table name?
static void PragmaMakeTablePrivate(ClientContext &context,
									   const FunctionParameters &parameters) {
	auto table_name = parameters.values[0].GetValue<std::string>();
	auto duckdp_state = GetDuckDPState(context);
	auto duckdp_private_tables = duckdp_state.get()->duckdp_state->private_tables;

	if (duckdp_state->TableIsPrivate(table_name)) {
		throw std::runtime_error("Table " + table_name + " is already private!");
	}

	// check if table exists in any of the schemas
	auto all_schemas  = Catalog::GetAllSchemas(context);
	for (auto schema_reference : all_schemas) {
		auto schema = &schema_reference.get();

		// search table in schema
		auto table_entry = schema->GetEntry(schema->GetCatalogTransaction(context),CatalogType::TABLE_ENTRY,table_name );
		if (table_entry != nullptr) {
			// add table name to private tables
			string schema_name = schema_reference.get().name;
			string catalog_name = schema_reference.get().catalog.GetName();
			duckdp_state->RegisterPrivateTable( catalog_name, schema_name, table_name);
			return;
		}
	}

	throw std::runtime_error("Did not find table " + table_name + "!");
}


void CorePragma::RegisterMakeTablePrivate(DatabaseInstance &instance){
	// Define the pragma function
	auto pragma_func = PragmaFunction::PragmaCall("make_table_private", PragmaMakeTablePrivate, {LogicalType::VARCHAR});

	// Register the pragma function
	ExtensionUtil::RegisterFunction(instance, pragma_func);
}

} // namespace core
