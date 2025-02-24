#pragma once
#include "duckdb.hpp"

namespace duckdb {


class DuckDPState : public ClientContextState {

  public:
    void RegisterPrivateTable(string &catalog_name, string &schema_name, string &table_name);
    void RegisterPrivateColumn(string &table_name, string &column_name, double lower_bound, double upper_bound);
    bool TableIsPrivate(string &table_name);
    bool ColumnIsPrivate(string &table_name, string &column_name);
    void RegisterAccessedTable(string &table_name, ::idx_t table_id);
    void RegisterAccessedColumn(string &table_name, ::idx_t table_id, string &column_name, idx_t column_id);

  public:
  struct PrivateColumn {
    double lower_bound;
    double upper_bound;
  };

  struct PrivateTable {
    std::string catalog_name;
    std::string schema_name;
    std::unordered_map<std::string, PrivateColumn> private_columns;
  };

  struct AccessedColumn {
    idx_t column_index;
  };

  struct AccessedTable {
    idx_t table_index;
    std::unordered_map<std::string, AccessedColumn> accessed_columns;
  };

  struct State {
    std::unordered_map<string,PrivateTable> private_tables;
    std::unordered_map<string,AccessedTable> accessed_tables;
  };

    std::shared_ptr<State> duckdp_state = std::make_shared<State>();


};

shared_ptr<DuckDPState> GetDuckDPState(ClientContext &context);


} // namespace duckdb