#define DUCKDB_EXTENSION_MAIN


#include "differential_privacy_extension.hpp"
#include "duckdb.hpp"
#include "duckdb/common/exception.hpp"
#include "duckdb/common/string_util.hpp"
#include "duckdb/function/scalar_function.hpp"
#include "duckdb/main/extension_util.hpp"
#include <duckdb/parser/parsed_data/create_scalar_function_info.hpp>
#include "dp_parser.hpp"
#include "opendp.h"

template<typename T>
T handleFfiResult(const FfiResult<T> &result) {
	if (result.tag == FfiResult<T>::Tag::Ok) {
		return result.ok._0;
	} else {
		throw std::runtime_error(result.err._0->message);
	}
}


extern  "C"{
double printMessage(double number, char *type, float bounds[2]) {
	char *MO = "ZeroConcentratedDivergence<f64>";
	char *c_T = type;
	bool c_nullable = false;
	AnyDomain *domain;

	if (bounds == NULL) {
		domain = handleFfiResult(opendp_domains__atom_domain(NULL, c_nullable, c_T));
	} else {
		char *bound_type = "(f64, f64)";
		float bounds2[2] = {1.0, 2.0};
		float *prr = bounds2;
		const FfiSlice bounds_slice = {prr, 2};
		AnyObject *bounds_object = handleFfiResult(opendp_data__slice_as_object(&bounds_slice, bound_type));

		domain = handleFfiResult(opendp_domains__atom_domain(bounds_object, c_nullable, c_T));
	}


	AnyMetric *metric = handleFfiResult(opendp_metrics__absolute_distance(type));
	double scale = 1;

	AnyMeasurement *anymeasurement = handleFfiResult(
	    opendp_measurements__make_gaussian(domain, metric, &scale, NULL, MO));


	const FfiSlice slice = {&number, 1};

	AnyObject *a = handleFfiResult(opendp_data__slice_as_object(&slice, type));
	const AnyObject *ab = handleFfiResult(opendp_core__measurement_invoke(anymeasurement, a));

	const void *aaa = handleFfiResult(opendp_data__object_as_slice(ab))->ptr;
	double number1 = *(const double *) aaa;


	return number1;

}
}

namespace duckdb {
    // TODO remove (kept for some debugging)
    inline void DifferentialPrivacyScalarFun(DataChunk &args, ExpressionState &state, Vector &result) {
        auto &name_vector = args.data[0];
        UnaryExecutor::Execute<string_t, string_t>(
                name_vector, result, args.size(),
                [&](string_t name) {
                    return StringVector::AddString(result, "Quack "+name.GetString());;
                });
    }



    double test (double test){
	    return printMessage(test,"f64", NULL);
    }
    static void MyCustomFunction(DataChunk  &args, ExpressionState &state, Vector &result) {
	    auto &name_vector = args.data[0];
	   UnaryExecutor::Execute<double, double>(name_vector, result, args.size(),  test);
    }


    static void LoadInternal(DatabaseInstance &instance) {

        // TODO remove (kept for some debugging)
        // Register a scalar function
        auto differential_privacy_scalar_function = ScalarFunction("quack", {LogicalType::VARCHAR}, LogicalType::VARCHAR, DifferentialPrivacyScalarFun);
	    auto differential_privacy_scalar_function2 = ScalarFunction("quack2", {LogicalType::DOUBLE}, LogicalType::DOUBLE, MyCustomFunction);
        ExtensionUtil::RegisterFunction(instance, differential_privacy_scalar_function);
	    ExtensionUtil::RegisterFunction(instance, differential_privacy_scalar_function2);

        // add a parser extension
        auto &db_config = duckdb::DBConfig::GetConfig(instance);
        auto DP_parser = duckdb::DPParserExtension();
        db_config.parser_extensions.push_back(DP_parser);
    }

    void DifferentialPrivacyExtension::Load(DuckDB &db) {
        LoadInternal(*db.instance);
    }
    std::string DifferentialPrivacyExtension::Name() {
        return "differential_privacy";
    }

} // namespace duckdb

extern "C" {

DUCKDB_EXTENSION_API void differential_privacy_init(duckdb::DatabaseInstance &db) {
    duckdb::DuckDB db_wrapper(db);
    db_wrapper.LoadExtension<duckdb::DifferentialPrivacyExtension>();

}

DUCKDB_EXTENSION_API const char *differential_privacy_version() {
    return duckdb::DuckDB::LibraryVersion();
}
}

#ifndef DUCKDB_EXTENSION_MAIN
#error DUCKDB_EXTENSION_MAIN not defined
#endif
