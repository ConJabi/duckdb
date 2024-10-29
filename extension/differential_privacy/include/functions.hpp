#pragma once
#include <duckdb/function/function_set.hpp>
namespace duckdb {

struct CoreFunctions {
  static void Register(DatabaseInstance &db) {
    RegisterNoiseScalarFunction(db);
    RegisterSumOptimizerFunction(db);
  }
  static ScalarFunctionSet GetNoiseFunction();

private:
  static void RegisterNoiseScalarFunction(DatabaseInstance &db);
  static void RegisterSumOptimizerFunction(DatabaseInstance &db);
};

} // namespace duckb