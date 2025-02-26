#include "duckdp_state.hpp"

 namespace duckdb {



 void DuckDPState::RegisterPrivateTable(const string &catalog_name, const string &schema_name, const string &table_name) {
	 duckdp_state->private_tables[table_name] = {catalog_name, schema_name, {}};
 }

 void DuckDPState::RegisterPrivateColumn(const string &table_name, const string &column_name, double lower_bound, double upper_bound) {
 	duckdp_state->private_tables[table_name].private_columns[column_name] = {lower_bound, upper_bound};
 }

 // todo duplicate names?
 bool DuckDPState::TableIsPrivate(const string &table_name) {
 	if (duckdp_state->private_tables.find(table_name) != duckdp_state->private_tables.end()) {
 		return true;
 	}
 	return false;
 }

 void DuckDPState::RegisterAccessedTable(const string &table_name, idx_t table_id) {
	 duckdp_state->accessed_tables[table_id].table_name = table_name;
 }

 void DuckDPState::RegisterAccessedColumn(idx_t table_id, const string &column_name, idx_t column_id) {
 	duckdp_state->accessed_tables[table_id].accessed_columns[column_id].column_name = column_name;
 }

 bool DuckDPState::BindingIsPrivate(ColumnBinding binding) {
 	auto table_it = duckdp_state->accessed_tables.find(binding.table_index);
 	if (table_it == duckdp_state->accessed_tables.end()) {
 		return false;
 	}

 	auto column_it = table_it->second.accessed_columns.find(binding.table_index);
 	if (column_it == table_it->second.accessed_columns.end()) {
 		return false;
 	}

 	return true;
 }

 bool DuckDPState::PrivateTableIsAccessed() {
 	return !duckdp_state->accessed_tables.empty();
 }

 void DuckDPState::ResetQueryState() {
 	duckdp_state->accessed_tables.clear();
 	SetPrivateChildExpression(false);
 }

void DuckDPState::SetPrivateChildExpression(bool is_private) {
	 duckdp_state->private_child_expression = is_private;
 }

 bool DuckDPState::hasPrivateChildExpression() {
	 return duckdp_state->private_child_expression;
 }

void DuckDPState::TransactionRollback(MetaTransaction &transaction, ClientContext &context) {
	 ResetQueryState();
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