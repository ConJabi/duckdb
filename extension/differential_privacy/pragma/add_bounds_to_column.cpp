#include "duckdb/function/pragma_function.hpp"
#include "duckdb/main/extension_util.hpp"
#include "duckdp_state.hpp"
#include "pragma.hpp"

#include <duckdb/catalog/catalog_entry/table_catalog_entry.hpp>
#include <duckdb/main/client_data.hpp>
#include <memory>

namespace duckdb {

// todo warn when called for same column multiple times? Allow overwriting?
static void PragmaAddBoundsToColumn(ClientContext &context,
									   const FunctionParameters &parameters) {

	string table_name = parameters.values[0].GetValue<std::string>();
	string column_name = parameters.values[1].GetValue<std::string>();
	double lower_bound = parameters.values[2].GetValue<double>();
	double upper_bound = parameters.values[3].GetValue<double>();
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

	if (upper_bound < lower_bound) {
		throw std::runtime_error("Lower bound is greater than upper bound: " + std::to_string(lower_bound) + ">" + std::to_string(upper_bound));
	}

	// todo add bounds
	// duckdp_state->RegisterPrivateColumn(table_name, column_name, lower_bound, upper_bound);
}

void CorePragma::RegisterAddBoundsToColumn(DatabaseInstance &instance) {
	// Define the pragma function
	auto pragma_func = PragmaFunction::PragmaCall("add_bounds", PragmaAddBoundsToColumn, {LogicalType::VARCHAR, LogicalType::VARCHAR, LogicalType::DOUBLE, LogicalType::DOUBLE});

	// Register the pragma function
	ExtensionUtil::RegisterFunction(instance, pragma_func);
}
//
} // namespace core
