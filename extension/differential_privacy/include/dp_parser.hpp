#include "duckdb.hpp"
#include "duckdb/function/pragma_function.hpp"
#include "duckdb/main/extension_util.hpp"
#

#include <utility>

#ifndef DUCKDB_DP_PARSER_HPP
#define DUCKDB_DP_PARSER_HPP

namespace duckdb {

    class DPParserExtension : public ParserExtension {
    public:
        explicit DPParserExtension() {
            parse_function = DPParseFunction;
            plan_function = DPPlanFunction;
        }

        static ParserExtensionParseResult DPParseFunction(ParserExtensionInfo *info, const string &query);
        static ParserExtensionPlanResult DPPlanFunction(ParserExtensionInfo *info, ClientContext &context,
                                                         unique_ptr<ParserExtensionParseData> parse_data);
    };

    BoundStatement DPBind(ClientContext &context, Binder &binder, OperatorExtensionInfo *info, SQLStatement &statement);

    struct DPOperatorExtension : public OperatorExtension {
        DPOperatorExtension() : OperatorExtension() {
            Bind = DPBind;
        }

        std::string GetName() override {
            return "DP";
        }
    };

    struct DPParseData : ParserExtensionParseData {
        DPParseData() {
        }

        unique_ptr<SQLStatement> statement;

        unique_ptr<ParserExtensionParseData> Copy() const override {
            return make_uniq_base<ParserExtensionParseData, DPParseData>(statement->Copy());
        }

        explicit DPParseData(unique_ptr<SQLStatement> statement) : statement(std::move(statement)) {
        }
    };

    class IVMState : public ClientContextState {
    public:
        explicit IVMState(unique_ptr<ParserExtensionParseData> parse_data) : parse_data(std::move(parse_data)) {
        }

        void QueryEnd() override {
            parse_data.reset();
        }

        unique_ptr<ParserExtensionParseData> parse_data;
    };

    class DPFunction : public TableFunction {
    public:
        DPFunction() {
            name = "DP function";
            arguments.push_back(LogicalType::BOOLEAN); // parsing successful
            bind = DPBind;
            init_global = DPInit;
            function = DPFunc;
        }

        struct DPBindData : public TableFunctionData {

            explicit DPBindData(bool result) : result(result) {
            }

            bool result;
        };

        struct DPGlobalData : public GlobalTableFunctionState {
            DPGlobalData() : offset(0) {
            }

            idx_t offset;
        };

        static duckdb::unique_ptr<FunctionData> DPBind(ClientContext &context, TableFunctionBindInput &input,
                                                        vector<LogicalType> &return_types, vector<string> &names) {


            names.emplace_back("MATERIALIZED VIEW CREATION");
            return_types.emplace_back(LogicalType::BOOLEAN);
            bool result = false;
            if (IntegerValue::Get(input.inputs[0]) == 1) {
                result = true; // explict creation of the result since the input is an integer value for some reason
            }
            return make_uniq<DPBindData>(result);
        }

        static duckdb::unique_ptr<GlobalTableFunctionState> DPInit(ClientContext &context, TableFunctionInitInput &input) {
            return make_uniq<DPGlobalData>();
        }

        static void DPFunc(ClientContext &context, TableFunctionInput &data_p, DataChunk &output) {
            // placeholder (this needs to return something)
             printf("Inside IVMFunc of Table function class\n");

        }
    };

} // namespace duckdb

#endif // DUCKDB_IVM_PARSER_HPP