#pragma once
#include "duckdb.hpp"

namespace duckdb {


class DuckDPState : public ClientContextState {

  // todo make state suitable for integer
  struct PrivateColumn {
    double lower_bound;
    double upper_bound;
    double null_replacement;
  };

  struct PrivateTable {
    string catalog_name;
    string schema_name;
    unordered_map<std::string, PrivateColumn> private_columns;
  };

  struct AccessedColumn {
    PrivateColumn* private_column;
  };

  struct AccessedTable {
    PrivateTable* private_table;
    unordered_map<idx_t, AccessedColumn> accessed_columns;
  };

  struct State {
    unordered_map<string,PrivateTable> private_tables;
    unordered_map<idx_t,AccessedTable> accessed_tables;
    bool private_child_expression = false;
    bool private_child_operation = false;
  };


  public:
    void RegisterPrivateTable(const string &catalog_name, const string &schema_name, const string &table_name);
    void RegisterPrivateColumn(const string &table_name, const string &column_name);
    void AddBoundsToColumn(const string &table_name, const string &column_name, double lower_bound, double upper_bound);
    void AddNullReplacement(const string &table_name, const string &column_name, double null_replacement);


    bool TableIsPrivate(const string &table_name);
    bool ColumnIsPrivate(const string &table_name, string &column_name);

    void RegisterAccessedTable(const string &table_name, idx_t table_id);
    void RegisterAccessedColumn(idx_t table_id, const string &column_name, idx_t column_id);
    bool PrivateTableIsAccessed();
    bool BindingIsPrivate(ColumnBinding binding);
    void ResetQueryState();

    void SetPrivateChildExpression(bool is_private);
    bool HasPrivateChildExpression();

    void SetPrivateChildOperator(bool is_private);
    bool HasPrivateChildOperator();

    void TransactionRollback(MetaTransaction &transaction, ClientContext &context) override;

    PrivateColumn* GetPrivateColumn(ColumnBinding binding);


    shared_ptr<State> duckdp_state = make_shared_ptr<State>();

};

shared_ptr<DuckDPState> GetDuckDPState(ClientContext &context);


} // namespace duckdb