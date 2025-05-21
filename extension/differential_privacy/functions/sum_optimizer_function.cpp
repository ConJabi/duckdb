#include "../../../third_party/catch/catch.hpp"
#include "duckdb/core_functions/aggregate/distributive_functions.hpp"
#include "duckdb/core_functions/scalar/generic_functions.hpp"
#include "duckdb/main/extension_util.hpp"
#include "duckdb/optimizer/optimizer_extension.hpp"
#include "duckdb/planner/expression/bound_aggregate_expression.hpp"
#include "duckdb/planner/logical_operator.hpp"
#include "duckdb/planner/operator/logical_projection.hpp"
#include "duckdp_state.hpp"
#include "functions.hpp"

#include <duckdb/catalog/catalog_entry/table_catalog_entry.hpp>
#include <duckdb/main/database.hpp>
#include <duckdb/optimizer/column_binding_replacer.hpp>
#include <duckdb/optimizer/optimizer.hpp>
#include <duckdb/planner/binder.hpp>
#include <duckdb/planner/expression/bound_constant_expression.hpp>
#include <duckdb/planner/expression/bound_function_expression.hpp>
#include <duckdb/planner/expression/bound_operator_expression.hpp>
#include <duckdb/planner/operator/logical_aggregate.hpp>
#include <duckdb/planner/operator/logical_get.hpp>

namespace duckdb {
// todo: organize more into seperate files
// todo: follow guideline https://github.com/duckdb/duckdb/blob/main/CONTRIBUTING.md

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

template <typename T>
void replace_null_of_children(ClientContext &context, BoundAggregateExpression* expression, T replacement_value) {
	LogicalType logic_type= expression->function.arguments[0];

	vector<unique_ptr<Expression>> fill_null = add_constant_to_expression(expression->children[0]->Copy(), replacement_value);
	unique_ptr<BoundOperatorExpression> coalesce_expression = make_uniq<BoundOperatorExpression>(ExpressionType::OPERATOR_COALESCE, logic_type);
	coalesce_expression->children = std::move(fill_null);

	vector<unique_ptr<Expression>> final_expression_vec;
	final_expression_vec.emplace_back(std::move(coalesce_expression));
	expression->children = std::move(final_expression_vec);
}


class DPAggrReplacer : LogicalOperatorVisitor {
public:
	DPAggrReplacer(ClientContext &ctx, Optimizer &optm, LogicalOperator &rt) : context(ctx) , optimizer(optm), root(rt) {};

	ClientContext &context;
	Optimizer &optimizer;
	LogicalOperator &root;
	shared_ptr<DuckDPState> duckdp_state = GetDuckDPState(context);


	// todo change name into something more descriptive
	bool CustomVisitOperatorChildren(const unique_ptr<LogicalOperator> &op) {
		bool private_operator = false;
		for (auto &child : op->children) {
			private_operator = private_operator || CustomVisitOperator(child);
		}
		return private_operator;
	}

	unique_ptr<LogicalProjection> CreateProjectionOnTop(unique_ptr<LogicalOperator> &op, vector<unique_ptr<Expression>> &projection_expressions) {
		idx_t table_idx = optimizer.binder.GenerateTableIndex();
		auto projection = make_uniq<LogicalProjection>(table_idx, std::move(projection_expressions));
		if (op->has_estimated_cardinality) {
			projection->SetEstimatedCardinality(op->estimated_cardinality);
		}
		// todo no copy?
		projection->AddChild(op->Copy(context));
		projection->ResolveOperatorTypes();
		return projection;
	}

	void RedirectBindings(unique_ptr<LogicalOperator> &projection, unique_ptr<LogicalOperator> &op) {
		ColumnBindingReplacer replacer;
		auto old_binding = op->GetColumnBindings();
		auto new_binding = projection->GetColumnBindings();

		for (size_t i = 0; i < old_binding.size(); i++) {
			replacer.replacement_bindings.emplace_back(old_binding[i], new_binding[i]);
		}
		op = std::move(projection);

		// Make sure we stop at the new projection when replacing bindings
		replacer.stop_operator = op.get();

		// Make the plan consistent again
		replacer.VisitOperator(root);
	}

	unique_ptr<BoundFunctionExpression> CreateNoiseExpression(ColumnBinding child_binding,
	                                                          double scale) {
		auto column_ref_expression = make_uniq<BoundColumnRefExpression>(LogicalType::DOUBLE, child_binding);
		auto standard_deviation = make_uniq<BoundConstantExpression>(Value(scale));

		vector<unique_ptr<Expression>> function_expressions_children;
		function_expressions_children.emplace_back(std::move(column_ref_expression));
		function_expressions_children.emplace_back(std::move(standard_deviation));

		// Create noise expression with column reference expression as child

		auto noise_funct = CoreFunctions().GetNoiseFunction().GetFunctionByArguments(
		    context, vector<LogicalType> {LogicalType::DOUBLE, LogicalType::DOUBLE});
		noise_funct.name = "noise";
		unique_ptr<BoundFunctionExpression> function_expression = make_uniq<BoundFunctionExpression>(
		    LogicalType::DOUBLE, noise_funct, std::move(function_expressions_children), nullptr, false);
		return function_expression;
	}

	// todo support integer, add checks
	void RewriteDPSumExpression(BoundAggregateExpression *bound_aggregate_expression) {
		BoundConstantExpression* lower_bound_expr = &bound_aggregate_expression->children[1]->Cast<BoundConstantExpression>();
		auto lower_bound = lower_bound_expr->value.GetValue<double>();
		BoundConstantExpression* upper_bound_expr = &bound_aggregate_expression->children[2]->Cast<BoundConstantExpression>();
		auto upper_bound = upper_bound_expr->value.GetValue<double>();
		BoundConstantExpression* fill_null_expr = &bound_aggregate_expression->children[3]->Cast<BoundConstantExpression>();
		auto fill_null = fill_null_expr->value.GetValue<double>();

		replace_null_of_children(context, bound_aggregate_expression, fill_null);
		add_upper_bound_to_children(context, bound_aggregate_expression, upper_bound);
		add_lower_bound_to_children(context, bound_aggregate_expression, lower_bound);

		bound_aggregate_expression->function = SumFun().GetFunctions().GetFunctionByArguments(context,vector<LogicalType>{ LogicalType::DOUBLE}); ;
		bound_aggregate_expression->function.name="sum";
	}

	void RewriteSumExpression(BoundAggregateExpression * bound_aggregate_expression) {
		BoundColumnRefExpression* ColRefExpr = &bound_aggregate_expression->children[0].get()->Cast<BoundColumnRefExpression>();
		auto private_column = duckdp_state->GetPrivateColumn(ColRefExpr->binding);

		if (isnan(private_column->lower_bound) || isnan(private_column->upper_bound)) {
			throw Exception(ExceptionType::INVALID_INPUT, "Column misses upper bound or lower bound, use pragma add_bounds");
		}

		if (isnan(private_column->null_replacement)) {
			throw Exception(ExceptionType::INVALID_INPUT, "Column misses replacement value for null values, use pragma add_replacement");
		}

		double null_replacement = private_column->null_replacement;
		replace_null_of_children(context, bound_aggregate_expression, null_replacement);

		double lower_bound = private_column->lower_bound;
		double upper_bound = private_column->upper_bound;
		add_upper_bound_to_children(context, bound_aggregate_expression, upper_bound);
		add_lower_bound_to_children(context, bound_aggregate_expression, lower_bound);

	}


	void PrivateOperatorHandler(unique_ptr<LogicalOperator> &op) {
		if (op->type != LogicalOperatorType::LOGICAL_AGGREGATE_AND_GROUP_BY) {
			throw Exception(ExceptionType::PERMISSION, "Table is private, only aggregations allowed");
		}

		// Create projection with current operator as child
		vector<unique_ptr<Expression>> projection_expressions;

		vector<unique_ptr<Expression>> &expressions =  op->Cast<LogicalAggregate>().expressions;
		for (size_t i = 0; i <expressions.size(); ++i) {
			auto expression = expressions[i].get();
			if (expression->GetExpressionClass() == ExpressionClass::BOUND_AGGREGATE) {
				auto bound_aggregate_expression = static_cast<BoundAggregateExpression*>(expression);
				if (bound_aggregate_expression->function.name == "sum") {
					RewriteSumExpression(bound_aggregate_expression);
					// todo where to get scale
					unique_ptr<BoundFunctionExpression> function_expression = CreateNoiseExpression(op->GetColumnBindings()[i], 0.06);
					projection_expressions.emplace_back(std::move(function_expression));
					continue;
				} if (bound_aggregate_expression->function.name == "dp_sum") {
					RewriteDPSumExpression(bound_aggregate_expression);
					unique_ptr<BoundFunctionExpression> function_expression = CreateNoiseExpression(op->GetColumnBindings()[i], 0.0);
					projection_expressions.emplace_back(std::move(function_expression));
					continue;
				}
			}
			throw Exception(ExceptionType::PERMISSION, "Table is private");
		}

		// Replace all bindings to op with the new projection bindings
		// todo what if no expressions, can that happen?
		unique_ptr<LogicalOperator> projection = CreateProjectionOnTop(op, projection_expressions);
		RedirectBindings(projection, op);
	}


	// todo check for (table) filters. Maybe in parser already because: where < max_col_value + 1 will not produce filter
	// Also where column=value will not produce filter if whole column is value
	// check for logical_empty result, where > max_column_value
	bool LogicalGetHandler(unique_ptr<LogicalOperator> &op) {
		auto &get_operator = op->Cast<LogicalGet>();
		auto table_catalog = get_operator.GetTable();


		// todo are there cases with no catalog? what to do then
		// if (!table_catalog) {
		// 	return;
		// }

		auto table_name = table_catalog.get()->name;

		if (!duckdp_state->TableIsPrivate(table_name)) {
			return false;
		}

		const idx_t table_index = get_operator.table_index;
		duckdp_state->RegisterAccessedTable(table_name, table_index);

		auto &column_list = get_operator.GetTable()->GetColumns();
		auto &column_ids = get_operator.GetColumnIds();




		for (int binding_index = 0; binding_index < column_ids.size(); binding_index++) {
			auto column_id = column_ids[binding_index];
			auto &column = column_list.GetColumn(LogicalIndex(column_id));
			string column_name = column.GetName();

			duckdp_state->RegisterAccessedColumn(table_index, column_name, binding_index );
		}

		return true;
	}

	// todo change name into something more descriptive
	bool CustomVisitOperator(unique_ptr<LogicalOperator> &op) {
		// check if operator children and/or expressions access private tables
		bool has_private_child_op = CustomVisitOperatorChildren(op);
		VisitOperatorExpressions(*op);

		if (duckdp_state->HasPrivateChildExpression()) {
			PrivateOperatorHandler(op);
			duckdp_state->SetPrivateChildExpression(false);
		}else if (has_private_child_op) {
			throw Exception(ExceptionType::PERMISSION, "Table is private");
		}

		// check if private table is accessed, register the bindings
		if (op->type == LogicalOperatorType::LOGICAL_GET) {
			return LogicalGetHandler(op);
		}

		// todo check if safe end of query, otherwise maybe use EndQuery callback?
		// Remove the private bindings from duckdp  state
		if (op && op.get() ==  &root) {
			duckdp_state->ResetQueryState();
		}

		return false;
	}


	void VisitExpression(unique_ptr<Expression> *expression) override {
		VisitExpressionChildren(**expression);
		auto &expr = *expression;

		// check if column references are bound to private table
		if (duckdp_state->PrivateTableIsAccessed() && expr->GetExpressionClass() == ExpressionClass::BOUND_COLUMN_REF) {
			auto &column_ref = expr->Cast<BoundColumnRefExpression>();

			if (duckdp_state->BindingIsPrivate(column_ref.binding)) {
				duckdp_state->SetPrivateChildExpression(true);
			}
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
	auto aggregate_function = AggregateFunction("dp_sum",{LogicalType::DOUBLE,LogicalType::DOUBLE,LogicalType::DOUBLE,LogicalType::DOUBLE},LogicalType::DOUBLE,nullptr,nullptr,nullptr,nullptr,nullptr,nullptr);
	ExtensionUtil::RegisterFunction(db,aggregate_function );

	auto &db_config = DBConfig::GetConfig(db);

	OptimizerExtension dp_optimizer;
	dp_optimizer.optimize_function = dp_optimizer_function;
	db_config.optimizer_extensions.push_back(dp_optimizer);
}
} // name space duckdb