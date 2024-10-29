#include "duckdb/core_functions/aggregate/distributive_functions.hpp"
#include "duckdb/core_functions/scalar/generic_functions.hpp"
#include "duckdb/main/extension_util.hpp"
#include "duckdb/optimizer/optimizer_extension.hpp"
#include "duckdb/planner/expression/bound_aggregate_expression.hpp"
#include "duckdb/planner/logical_operator.hpp"
#include "functions.hpp"

#include <duckdb/planner/expression/bound_constant_expression.hpp>
#include <duckdb/planner/expression/bound_function_expression.hpp>
#include <duckdb/planner/operator/logical_aggregate.hpp>

namespace duckdb {
template <class T> struct SumState {};

struct DPSumOperation {
	template <class STATE> static void Initialize(STATE &state) {
		throw NotImplementedException("DPSumOperation::Initialize");
	}

	template <class A_TYPE, class STATE, class OP>
	static void Operation(STATE &state, const A_TYPE &x_data, AggregateUnaryInput &idata) {
		throw NotImplementedException("DPSumOperation::Operation");
	};


	template <class STATE, class OP>
	static void Combine(const STATE &source, STATE &target,
						AggregateInputData &aggr_input_data) {

		throw NotImplementedException("DPSumOperation::Combine");
	};

	template <class INPUT_TYPE, class STATE, class OP>
	static void ConstantOperation(STATE &state, const INPUT_TYPE &input, AggregateUnaryInput &, idx_t count) {
		throw NotImplementedException("DPSumOperation::ConstantOperation");
	}

	template <class T, class STATE>
	static void Finalize(STATE &state, T &target, AggregateFinalizeData &finalize_data) {
		throw NotImplementedException("DPSumOperation::Finalize");
	}

	static bool IgnoreNull() { return true; }


};


AggregateFunctionSet DP_sum() {\
	AggregateFunctionSet set("dp_sum");
	set.AddFunction(AggregateFunction::UnaryAggregate<SumState<double>, double,double, DPSumOperation>(LogicalType::DOUBLE,LogicalType::DOUBLE));
	set.AddFunction(AggregateFunction::UnaryAggregate<SumState<float>, float,float, DPSumOperation>(LogicalType::FLOAT,LogicalType::FLOAT));
	return set;
};

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
	DPAggrReplacer(ClientContext &ctx) : context(ctx) {};

public:
	//! Update each operator of the plan
	void VisitOperator(LogicalOperator &op) {
		if (op.type == LogicalOperatorType::LOGICAL_AGGREGATE_AND_GROUP_BY) {
			// // auto &aggr = op.Cast<LogicalAggregate>().expressions[0];
			// if (aggr->GetExpressionClass() == ExpressionClass::BOUND_AGGREGATE) {
			// 	auto expression = static_cast<BoundAggregateExpression*>(aggr.get());
			// 	if (expression->function.name == "dp_sum") {
			// 		op.type = LogicalOperatorType::LOGICAL_PROJECTION;
			// 		VisitOperatorExpressions(op);
			// 		// LogicalOperator op_child = op.Copy(context);
			// 	}
			// }
			VisitOperatorExpressions(op);


		}
		VisitOperatorChildren(op);
	}

	//! Visit an expression and update its column bindings
	void VisitExpression(unique_ptr<Expression> *expression) {
		auto &expr = *expression;
		if (expr->GetExpressionClass() == ExpressionClass::BOUND_AGGREGATE) {
			auto aggr = static_cast<BoundAggregateExpression*>(expr.get());

			if (aggr->function.name == "dp_sum") {
				// todo make sure dp_sum children are correct

				// todo discuss  where to get this value and type from also discuss lower/upper bound functions
				double upper_bound = 2.0;
				double lower_bound = 1.0;
				// add_upper_bound_to_children(context, aggr, upper_bound);
				// add_lower_bound_to_children(context, aggr, lower_bound);

				// todo discuss float implementation of sum , line 217 of sum.cpp
				LogicalType logic_type= aggr->function.arguments[0];
				// aggr->function = CoreFunctions::GetNoiseFunction().GetFunctionByArguments(context,vector<LogicalType>{logic_type});
				unique_ptr<Expression> resultt;
				BoundFunctionExpression* result = static_cast<BoundFunctionExpression*>(resultt.get());

				auto child = aggr->Copy();


				// result->children.emplace_back(std::move(child));



				aggr->function = SumFun().GetFunctions().GetFunctionByArguments(context,vector<LogicalType>{ logic_type}); ;
				aggr->function.name="sum";

			}
		} else {
			VisitExpressionChildren(**expression);
		}
	}
	ClientContext &context;
};

void dp_optimizer_function(OptimizerExtensionInput &input, unique_ptr<LogicalOperator> &plan_in_out) {
	DPAggrReplacer replacer(input.context);
	replacer.VisitOperator(*plan_in_out.get());
}

void CoreFunctions::RegisterSumOptimizerFunction(
	DatabaseInstance &db) {

	// register dummy aggregate function dp_sum
	ExtensionUtil::RegisterFunction(db, DP_sum());

	auto &db_config = DBConfig::GetConfig(db);

	OptimizerExtension dp_optimizer;
	dp_optimizer.optimize_function = dp_optimizer_function;
	db_config.optimizer_extensions.push_back(dp_optimizer);
}
} // name space duckdb