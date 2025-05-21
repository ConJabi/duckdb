#include "duckdb/function/pragma_function.hpp"
#include "duckdb/main/extension_util.hpp"
#include "duckdp_state.hpp"
#include "pragma.hpp"

#include <duckdb/catalog/catalog_entry/table_catalog_entry.hpp>
#include <duckdb/main/client_data.hpp>


namespace duckdb {

// todo warn when called for same column multiple times? Allow overwriting?
// todo add checks to state function, reuse in add_bounds
static void PragmaAddNullReplacement(ClientContext &context,
									   const FunctionParameters &parameters) {

	string table_name = parameters.values[0].GetValue<std::string>();
	string column_name = parameters.values[1].GetValue<std::string>();
	double replacement_value = parameters.values[2].GetValue<double>();

	auto duckdp_state = GetDuckDPState(context);

	if ( !duckdp_state->TableIsPrivate(table_name)){
		throw std::runtime_error("Table " + table_name + " is not private!");
	}

	auto duckdp_private_tables = duckdp_state.get()->duckdp_state->private_tables;
	auto private_table = duckdp_private_tables.find(table_name);

	// get table from catalog
	auto catalog = Catalog::GetCatalogEntry(context,private_table->second.catalog_name);
	auto table_catalog = &catalog.get()->GetEntry(context, CatalogType::TABLE_ENTRY,private_table->second.schema_name, private_table->first).Cast<TableCatalogEntry>();

	if (!table_catalog->ColumnExists(column_name)) {
		throw std::runtime_error("Column " + column_name + " does not exist in table " + table_name);
	}

	duckdp_state->AddNullReplacement(table_name, column_name, replacement_value);
}

void CorePragma::RegisterAddNullReplacement(DatabaseInstance &instance) {
	// Define the pragma function
	const auto pragma_func = PragmaFunction::PragmaCall("add_replacement", PragmaAddNullReplacement, {LogicalType::VARCHAR, LogicalType::VARCHAR, LogicalType::DOUBLE});

	// Register the pragma function
	ExtensionUtil::RegisterFunction(instance, pragma_func);
}
//
} // namespace core
