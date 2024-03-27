#include "include/dp_parser.hpp"

#include <regex>
#include <string>

#include "duckdb/parser/group_by_node.hpp"
#include "duckdb/parser/parser.hpp"
#include "duckdb/parser/query_node/select_node.hpp"
#include "duckdb/parser/statement/logical_plan_statement.hpp"

//#include "../compiler/include/compiler_extension.hpp"
//#include "../compiler/include/rdda/rdda_parse_table.hpp"
//#include "../compiler/include/rdda/rdda_parse_view.hpp"
//#include "duckdb/catalog/catalog_entry/duck_table_entry.hpp"
//#include "duckdb/common/serializer/binary_serializer.hpp"
//#include "duckdb/common/serializer/memory_stream.hpp"
//#include "duckdb/parser/group_by_node.hpp"
//#include "duckdb/parser/parser.hpp"
//#include "duckdb/parser/query_node/select_node.hpp"
//#include "duckdb/parser/statement/logical_plan_statement.hpp"
//#include "duckdb/planner/expression/bound_aggregate_expression.hpp"
//#include "duckdb/planner/expression_iterator.hpp"
//#include "duckdb/planner/operator/logical_aggregate.hpp"
//#include "duckdb/planner/operator/logical_projection.hpp"
//#include "duckdb/planner/planner.hpp"

namespace duckdb {
    ParserExtensionParseResult DPParserExtension::DPParseFunction(ParserExtensionInfo *info, const string &query) {


        auto query_lower = StringUtil::Lower(StringUtil::Replace(query, "\n", " "));
        StringUtil::Trim(query_lower);

        // printf("%s\n", query_lower.c_str());

        // todo: include more checks, make more robust
        // Check if query is part of extension syntax
        std::regex syntax_pattern(R"(^create\s+view\s+\w+\s+options\s\(+.+\)\sas)");
        if (std::regex_search(query_lower, syntax_pattern)) {
            std::cout << "Pattern matched: CREATE VIEW followed by OPTIONS" << std::endl;

            // todo: include more checks, make more robust
            // Capture options
            std::regex options_pattern(R"(options\s*\(([^]*)\))");
            std::string cleaned_query = std::regex_replace(query_lower, options_pattern, "");

            string options_part;
            std::smatch match;
            if (std::regex_search(query_lower, match, options_pattern)) {
                options_part = match[1]; // Extract the content within the parentheses
                StringUtil::Trim(options_part);
                printf("%s\n\n",options_part.c_str());
            }

//            printf("%s\n %s\n", cleaned_query.c_str(), query_lower.c_str());

        } else {
            std::cout << "Pattern not matched" << std::endl;
        }

        return ParserExtensionParseResult();

    }

    ParserExtensionPlanResult DPParserExtension::DPPlanFunction(ParserExtensionInfo *info, ClientContext &context,
                                                                unique_ptr<ParserExtensionParseData> parse_data) {
        printf("peter\n");
        return ParserExtensionPlanResult();
//        return result;
    }

    BoundStatement
    DPBind(ClientContext &context, Binder &binder, OperatorExtensionInfo *info, SQLStatement &statement) {
        printf("In IVM Bind function\n");
        return BoundStatement();
    }
}