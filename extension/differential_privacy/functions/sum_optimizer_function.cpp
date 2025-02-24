#include "duckdb/core_functions/aggregate/distributive_functions.hpp"
#include "duckdb/core_functions/scalar/generic_functions.hpp"
#include "duckdb/main/extension_util.hpp"
#include "duckdb/optimizer/optimizer_extension.hpp"
#include "duckdb/planner/expression/bound_aggregate_expression.hpp"
#include "duckdb/planner/logical_operator.hpp"
#include "duckdb/planner/operator/logical_projection.hpp"
#include "functions.hpp"
#include "duckdp_state.hpp"

#include <duckdb/catalog/catalog_entry/table_catalog_entry.hpp>
#include <duckdb/main/database.hpp>
#include <duckdb/optimizer/column_binding_replacer.hpp>
#include <duckdb/optimizer/optimizer.hpp>
#include <duckdb/planner/binder.hpp>
#include <duckdb/planner/expression/bound_constant_expression.hpp>
#include <duckdb/planner/expression/bound_function_expression.hpp>
#include <duckdb/planner/operator/logical_aggregate.hpp>
#include <duckdb/planner/operator/logical_get.hpp>

namespace duckdb {
template <class T> struct SumState {};

template <typename T>
vector<unique_ptr<Expression>> add_constant_to_expression(unique_ptr<Expression> expression, T value) {
	vector<unique_ptr<Expression>> result;
	auto child = expression->Copy();
	result.emplace_back(std::move(child));

	Value value_upper_bound = Value(value);
	auto least_value_expression = make_uniq<BoundConstantExpression>(value_upper_bound);
	result.emplace_back(std::move(least_value_expression));

	return result;
}


template <typename T>
void add_upper_bound_to_children(ClientContext &context, BoundAggregateExpression* expression, T upper_bound) {
	LogicalType logic_type= expression->function.arguments[0];
	vector<unique_ptr<Expression>> least_child = add_constant_to_expression(expression->children[0]->Copy(), upper_bound);

	auto leastfunct = LeastFun::GetFunctions().GetFunctionByArguments(context,vector<LogicalType>{logic_type, logic_type});
	leastfunct.bind(context, leastfunct, least_child);
	leastfunct.name = "least";

	unique_ptr<Expression> final_expression = make_uniq<BoundFunctionExpression>(logic_type, leastfunct,
															  std::move(least_child), nullptr, false);
	vector<unique_ptr<Expression>> final_expression_vec;
	final_expression_vec.emplace_back(std::move(final_expression));
	expression->children = std::move(final_expression_vec);
}


template <typename T>
void add_lower_bound_to_children(ClientContext &context, BoundAggregateExpression* expression, T lower_bound) {
	LogicalType logic_type= expression->function.arguments[0];

	vector<unique_ptr<Expression>> greatest_child = add_constant_to_expression(expression->children[0]->Copy(), lower_bound);

	auto greatestfunct = GreatestFun::GetFunctions().GetFunctionByArguments(context,vector<LogicalType>{logic_type, logic_type});
	greatestfunct.bind(context, greatestfunct, greatest_child);
	greatestfunct.name = "greatest";

	unique_ptr<Expression> final_expression = make_uniq<BoundFunctionExpression>(logic_type, greatestfunct,
															  std::move(greatest_child), nullptr, false);
	vector<unique_ptr<Expression>> final_expression_vec;
	final_expression_vec.emplace_back(std::move(final_expression));
	expression->children = std::move(final_expression_vec);
}


class DPAggrReplacer : LogicalOperatorVisitor {
public:
	DPAggrReplacer(ClientContext &ctx, Optimizer &optm, LogicalOperator &rt) : context(ctx) , optimizer(optm), root(rt) {};

	ClientContext &context;
	Optimizer &optimizer;
	LogicalOperator &root;
	shared_ptr<DuckDPState> duckdp_state = GetDuckDPState(context);

	void CustomVisitOperatorChildren(unique_ptr<LogicalOperator> &op) {
		for (auto &child : op->children) {
			CustomVisitOperator(child);
		}
	}

	// todo duplicate names?
	void CustomVisitOperator(unique_ptr<LogicalOperator> &op) {
		if (op->type == LogicalOperatorType::LOGICAL_GET) {
			auto table_catalog = op->Cast<LogicalGet>().GetTable();
			string table_name = table_catalog.get()->name;

			// quit if table not private
			if (!duckdp_state->TableIsPrivate(table_name)) {
				return;
			}

			auto &column_list = table_catalog->GetColumns();
			// auto column_ids = op->Cast<LogicalGet>().GetColumnIds();
			//
			// for (auto column_id : column_ids) {
			// 	LogicalIndex column_idx = LogicalIndex(column_id);
			// 	auto column_name = column_list.GetColumn(column_idx).Name();
			// 	int ab =5;
			//
			// }

			for (auto &column : column_list.Logical()) {
				column.GetName();
				int b =4;
			}
			// column_list.Get
			int a =5;
		}

		// if (op->type == LogicalOperatorType::LOGICAL_AGGREGATE_AND_GROUP_BY) {
		// 	// todo loop
		// 	auto &aggr = op->Cast<LogicalAggregate>().expressions[0];
		//
		// 	//  todo: make suitable for integers
		// 	if (aggr->GetExpressionClass() == ExpressionClass::BOUND_AGGREGATE) {
		// 		auto expression = static_cast<BoundAggregateExpression*>(aggr.get());
		// 		if (expression->function.name == "dp_sum") {
		// 			VisitOperatorExpressions(*op);
		//
		// 			// todo: where to get scale
		// 			// First create column reference expression to the private column and scale
		// 			auto column_ref_expression = make_uniq<BoundColumnRefExpression>(aggr->return_type, op->GetColumnBindings()[0] );
		// 			auto standard_deviation = make_uniq<BoundConstantExpression>(Value(0.06));
		//
		// 			vector<unique_ptr<Expression>> function_expressions_children;
		// 			function_expressions_children.emplace_back(std::move(column_ref_expression));
		// 			function_expressions_children.emplace_back(std::move(standard_deviation));
		//
		// 			// Create noise expression with column reference expression as child
		// 			vector<unique_ptr<Expression>> projection_expressions;
		// 			auto noise_funct = CoreFunctions().GetNoiseFunction().GetFunctionByArguments(context,vector<LogicalType>{ LogicalType::DOUBLE, LogicalType::DOUBLE});
		// 			noise_funct.name = "noise";
		// 			unique_ptr<Expression> function_expression = make_uniq<BoundFunctionExpression>(LogicalType::DOUBLE, noise_funct, std::move(function_expressions_children), nullptr, false);
		// 			projection_expressions.emplace_back(std::move(function_expression));
		//
		// 			// Create projection with the noise expressions and the aggregation as child
		// 			idx_t table_idx = optimizer.binder.GenerateTableIndex();
		// 			auto projection = make_uniq<LogicalProjection>(table_idx, std::move(projection_expressions));
		//
		// 			if (op->has_estimated_cardinality) {
		// 				projection->SetEstimatedCardinality(op->estimated_cardinality);
		// 			}
		//
		// 			projection->AddChild(op->Copy(context));
		// 			projection->ResolveOperatorTypes();
		//
		// 			// todo: combine multiple sums over different bindings, search for unsafe bindings
		// 			// Initialize a ColumnBindingReplacer with the new bindings
		// 			ColumnBindingReplacer replacer;
		// 			auto old_binding = op->GetColumnBindings();
		// 			auto new_binding = projection->GetColumnBindings();
		// 			replacer.replacement_bindings.emplace_back(old_binding[0], new_binding[0]);
		// 			op = std::move(projection);
		//
		// 			// Make sure we stop at the new projection when replacing bindings
		// 			replacer.stop_operator = op.get();
		//
		// 			// Make the plan consistent again
		// 			replacer.VisitOperator(root);
		// 		}
		// 	}
		// }
		CustomVisitOperatorChildren(op);
	}

	//! Visit an expression and update its column bindings
	void VisitExpression(unique_ptr<Expression> *expression) {
		auto &expr = *expression;
		if (expr->GetExpressionClass() == ExpressionClass::BOUND_AGGREGATE) {
			auto aggr = static_cast<BoundAggregateExpression*>(expr.get());

			if (aggr->function.name == "dp_sum") {
				// todo make sure dp_sum children are correct, check upper > lower, check for constant (this is done in add_bounds)
				// todo discuss  where to get this value and type from
				double lower_bound = static_cast<BoundConstantExpression*>(aggr->children[1].get())->value.GetValue<double>();
				double upper_bound = static_cast<BoundConstantExpression*>(aggr->children[2].get())->value.GetValue<double>();

				add_upper_bound_to_children(context, aggr, upper_bound);
				add_lower_bound_to_children(context, aggr, lower_bound);

				LogicalType logic_type= aggr->function.arguments[0];

				aggr->function = SumFun().GetFunctions().GetFunctionByArguments(context,vector<LogicalType>{ logic_type}); ;
				aggr->function.name="sum";
			}
		} else {
			VisitExpressionChildren(**expression);
		}
	}
};


void dp_optimizer_function(OptimizerExtensionInput &input, unique_ptr<LogicalOperator> &plan_in_out) {
	DPAggrReplacer replacer(input.context, input.optimizer, *plan_in_out.get());
	replacer.CustomVisitOperator(plan_in_out);
}


void CoreFunctions::RegisterSumOptimizerFunction(
	DatabaseInstance &db) {

	// register dummy aggregate function dp_sum
	ExtensionUtil::RegisterFunction(db, AggregateFunction("dp_sum",{LogicalType::DOUBLE,LogicalType::DOUBLE,LogicalType::DOUBLE},LogicalType::DOUBLE,NULL,NULL,NULL,NULL,NULL,NULL));

	auto &db_config = DBConfig::GetConfig(db);

	OptimizerExtension dp_optimizer;
	dp_optimizer.optimize_function = dp_optimizer_function;
	db_config.optimizer_extensions.push_back(dp_optimizer);
}
} // name space duckdb