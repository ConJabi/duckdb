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
// todo: organize more into seperate files

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


	void CustomVisitOperatorChildren(const unique_ptr<LogicalOperator> &op) {
		for (auto &child : op->children) {
			CustomVisitOperator(child);
		}
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

	void RedirectBindings(unique_ptr<LogicalProjection> &projection, unique_ptr<LogicalOperator> &op) {
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

	unique_ptr<BoundFunctionExpression> CreateNoiseExpression(Expression *expression, ColumnBinding expression_binding,
	                                                          double scale) {
		auto column_ref_expression = make_uniq<BoundColumnRefExpression>(expression->return_type, expression_binding);
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

	void RewriteSumExpression(BoundAggregateExpression * bound_aggregate_expression) {
		// BoundColumnRefExpression* ColRefExpr = bound_aggregate_expression->children[0].get()->Cast<BoundColumnRefExpression*>();
		// string table_name = duckdp_state.

					double lower_bound = 2;
					double upper_bound = 4;

					add_upper_bound_to_children(context, bound_aggregate_expression, upper_bound);
					add_lower_bound_to_children(context, bound_aggregate_expression, lower_bound);
		//
		// 			LogicalType logic_type= aggr->function.arguments[0];
		//
		// 			aggr->function = SumFun().GetFunctions().GetFunctionByArguments(context,vector<LogicalType>{ logic_type}); ;
		// 			aggr->function.name="sum";
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
					unique_ptr<BoundFunctionExpression> function_expression = CreateNoiseExpression(expression, op->GetColumnBindings()[i], 0.06);
					projection_expressions.emplace_back(std::move(function_expression));
					continue;
				}
			}
			throw Exception(ExceptionType::PERMISSION, "Table is private");
		}

		// Replace all bindings to op with the new projection bindings
		unique_ptr<LogicalProjection> projection = CreateProjectionOnTop(op, projection_expressions);
		RedirectBindings(projection, op);
	}

	// todo check for (table) filters. Maybe in parser already because: where < max_column_value will not produce filter
	void LogicalGetHandler(unique_ptr<LogicalOperator> &op) {
		auto &get_operator = op->Cast<LogicalGet>();
		auto table_catalog = get_operator.GetTable();

		if (!table_catalog) {
			return;
		}
		auto table_name = table_catalog.get()->name;

		if (!duckdp_state->TableIsPrivate(table_name)) {
			return;
		}
		// table is private

		const idx_t table_index = get_operator.table_index;
		duckdp_state->RegisterAccessedTable(table_name, table_index);
		duckdp_state->SetPrivateChildOperator(true);

		auto &column_list = get_operator.GetTable()->GetColumns();

		for (auto &column : column_list.Logical()) {
			string column_name = column.GetName();
			const idx_t column_index = column.Oid();
			duckdp_state->RegisterAccessedColumn(table_index, column_name, column_index );
		}
	}

// todo disable statistics_propagation void DisabledOptimizersSetting::SetGlobal(DatabaseInstance *db, DBConfig &config, const Value &input) {

	void CustomVisitOperator(unique_ptr<LogicalOperator> &op) {
		// check if operator children and/or expressions accesses private table
		CustomVisitOperatorChildren(op);
		VisitOperatorExpressions(*op);

		if (duckdp_state->HasPrivateChildOperator()) {
			if (duckdp_state->HasPrivateChildExpression()) {

				PrivateOperatorHandler(op);
				duckdp_state->SetPrivateChildExpression(false);
				duckdp_state->SetPrivateChildOperator(false);
			}else {
				throw Exception(ExceptionType::PERMISSION, "Table is private");
			}
		}
		// if operator expression has private bindings

		// check if private table is accessed, register the bindings
		if (op->type == LogicalOperatorType::LOGICAL_GET) {
			LogicalGetHandler(op);
		}

		// todo check if safe end of query, otherwise maybe use EndQuery callback?
		// Remove the private bindings from duckdp  state
		if (op && op.get() ==  &root) {
			duckdp_state->ResetQueryState();
		}
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



	// 	if (expr->GetExpressionClass() == ExpressionClass::BOUND_AGGREGATE) {
	// 		auto aggr = static_cast<BoundAggregateExpression*>(expr.get());
	//
	// 		if (aggr->function.name == "dp_sum") {
	// 			// todo make sure dp_sum children are correct, check upper > lower, check for constant (this is done in add_bounds)
	// 			// todo discuss  where to get this value and type from
	// 			double lower_bound = static_cast<BoundConstantExpression*>(aggr->children[1].get())->value.GetValue<double>();
	// 			double upper_bound = static_cast<BoundConstantExpression*>(aggr->children[2].get())->value.GetValue<double>();
	//
	// 			add_upper_bound_to_children(context, aggr, upper_bound);
	// 			add_lower_bound_to_children(context, aggr, lower_bound);
	//
	// 			LogicalType logic_type= aggr->function.arguments[0];
	//
	// 			aggr->function = SumFun().GetFunctions().GetFunctionByArguments(context,vector<LogicalType>{ logic_type}); ;
	// 			aggr->function.name="sum";
	// 		}
	// 	} else {
	// 		VisitExpressionChildren(**expression);
	// 	}
	}
};


void dp_optimizer_function(OptimizerExtensionInput &input, unique_ptr<LogicalOperator> &plan_in_out) {
	DPAggrReplacer replacer(input.context, input.optimizer, *plan_in_out.get());
	replacer.CustomVisitOperator(plan_in_out);
}


void CoreFunctions::RegisterSumOptimizerFunction(
	DatabaseInstance &db) {

	// register dummy aggregate function dp_sum
	auto aggregate_function = AggregateFunction("dp_sum",{LogicalType::DOUBLE,LogicalType::DOUBLE,LogicalType::DOUBLE},LogicalType::DOUBLE,nullptr,nullptr,nullptr,nullptr,nullptr,nullptr);
	ExtensionUtil::RegisterFunction(db,aggregate_function );

	auto &db_config = DBConfig::GetConfig(db);

	OptimizerExtension dp_optimizer;
	dp_optimizer.optimize_function = dp_optimizer_function;
	db_config.optimizer_extensions.push_back(dp_optimizer);
}
} // name space duckdb