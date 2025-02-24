#include "duckdp_state.hpp"

 namespace duckdb {



 void DuckDPState::RegisterPrivateTable(string &catalog_name, string &schema_name, string &table_name) {
	 duckdp_state->private_tables[table_name] = {catalog_name, schema_name, {}};
 }

 void DuckDPState::RegisterPrivateColumn(string &table_name, string &column_name, double lower_bound, double upper_bound) {
 	duckdp_state->private_tables[table_name].private_columns[column_name] = {lower_bound, upper_bound};
 }

 bool DuckDPState::TableIsPrivate(string &table_name) {
 	if (duckdp_state->private_tables.find(table_name) != duckdp_state->private_tables.end()) {
 		return true;
 	}
 	return false;
 }



 shared_ptr<DuckDPState> GetDuckDPState(ClientContext &context) {
 	auto lookup = context.registered_state->Get<DuckDPState>("duckdp");
 	if (!lookup) {
 		throw Exception(ExceptionType::INVALID,
						 "Registered DuckDP state not found");
 	}
 	return lookup;
 }



 } // namespace duckdb