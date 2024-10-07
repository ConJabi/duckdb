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
#include "duckdb/function/function_set.hpp"
#include "duckdb/parser/parsed_data/create_aggregate_function_info.hpp"

template<typename T>
T handleFfiResult(const FfiResult<T> &result) {
	if (result.tag == FfiResult<T>::Tag::Ok) {
		return result.ok._0;
	} else {
		throw std::runtime_error(result.err._0->message);
	}
}



double make_gaussian(double number, double scale, char *type) {
	char *MO = "ZeroConcentratedDivergence<f64>";
	char *c_T = type;
	bool c_nullable = false;

	AnyDomain *atom_domain = handleFfiResult(opendp_domains__atom_domain(NULL, c_nullable, c_T));
	AnyMetric *metric = handleFfiResult(opendp_metrics__absolute_distance(type));
	AnyMeasurement *measurement = handleFfiResult(
	    opendp_measurements__make_gaussian(atom_domain, metric, &scale, NULL, MO));


	const FfiSlice number_slice = {&number, 1};

	AnyObject *number_anyobject = handleFfiResult(opendp_data__slice_as_object(&number_slice, type));
	const AnyObject *private_anyobject = handleFfiResult(opendp_core__measurement_invoke(measurement, number_anyobject));

	const void *private_number_ptr = handleFfiResult(opendp_data__object_as_slice(private_anyobject))->ptr;
	return *(const double *) private_number_ptr;
}

template<typename T>
inline void make_gaussian_vec(T *number, T scale, uint32_t size, T *result) {
	char * type;
	char * vec_type;
	if (std::is_same<T, float>::value) {
		type = "f32";
		vec_type = "Vec<f32>";

	} else {
		type = "f64";
		vec_type = "Vec<f64>";
	}

	AnyMeasure *measure = handleFfiResult(opendp_measures__zero_concentrated_divergence(type));
	char *measure_type = handleFfiResult(opendp_measures__measure_type(measure));

	AnyDomain *atom_domain = handleFfiResult(opendp_domains__atom_domain(NULL, false, type));
	const FfiSlice size_slice = {&size, 1};
	const AnyObject *size_anyobject = handleFfiResult(opendp_data__slice_as_object(&size_slice, "i32"));
	AnyDomain *vector_domain = handleFfiResult(opendp_domains__vector_domain(atom_domain, size_anyobject));

	AnyMetric *metric = handleFfiResult(opendp_metrics__l2_distance(type));

	AnyMeasurement *measurement = handleFfiResult(
	    opendp_measurements__make_gaussian(vector_domain, metric, &scale, NULL, measure_type));


	const FfiSlice number_slice = {number, size};
	AnyObject *number_anyobject = handleFfiResult(opendp_data__slice_as_object(&number_slice, vec_type));

	AnyObject *private_anyobject = handleFfiResult(opendp_core__measurement_invoke(measurement, number_anyobject));
	const FfiSlice *private_void_ptr = handleFfiResult(opendp_data__object_as_slice(private_anyobject));

	T *private_number_ptr = (T *) private_void_ptr->ptr;

	for (size_t i = 0; i < size; i++) {
		result[i] = private_number_ptr[i];
	}
}


namespace duckdb {

	template <class T> struct SumState {
		T sum;
	};

	struct DPSumOperation {
	    template <class STATE> static void Initialize(STATE &state) {
		    state.sum = 0;
	    }

	    template <class A_TYPE, class STATE, class OP>
	    static void Operation(STATE &state, const A_TYPE &x_data, AggregateUnaryInput &idata) {
			    state.sum +=x_data;

	    };


	    template <class STATE, class OP>
	    static void Combine(const STATE &source, STATE &target,
	                        AggregateInputData &aggr_input_data) {

		 target.sum += source.sum;
	    };

	    template <class INPUT_TYPE, class STATE, class OP>
	    static void ConstantOperation(STATE &state, const INPUT_TYPE &input, AggregateUnaryInput &, idx_t count) {
		   state.sum +=input*count;
	    }

	    template <class T, class STATE>
	    static void Finalize(STATE &state, T &target, AggregateFinalizeData &finalize_data) {
		    target = state.sum;
	    }

	    static bool IgnoreNull() { return true; }


    };


    AggregateFunctionSet DP_sum() {
	    AggregateFunctionSet set("dp_sum");
	    set.AddFunction(AggregateFunction::UnaryAggregate<SumState<double>, double,double, DPSumOperation>(LogicalType::DOUBLE,LogicalType::DOUBLE));
	    return set;
	};




    double test (double test){
	    return make_gaussian(test,0.05, "f64");
    }

    template<typename T>
    static void NoiseFunction(DataChunk  &args, ExpressionState &state, Vector &result) {
	    auto &name_vector = args.data[0];
	    auto result_data = FlatVector::GetData<T>(result);
	    auto input_data = FlatVector::GetData<T>(name_vector);
	    make_gaussian_vec<T>(input_data, 0.05, args.size(), result_data);

//	    UnaryExecutor::Execute<double, double>(name_vector, result, args.size(),  test);
    }

    ScalarFunctionSet GetNoiseFunction() {
	    ScalarFunctionSet set("noise");
	    set.AddFunction(ScalarFunction( {LogicalType::FLOAT}, LogicalType::FLOAT, NoiseFunction<float>));
	    set.AddFunction(ScalarFunction( {LogicalType::DOUBLE}, LogicalType::DOUBLE, NoiseFunction<double>));
	    return set;
    }


    static void LoadInternal(DatabaseInstance &instance) {
        // Register a scalar function
	    ExtensionUtil::RegisterFunction(instance, GetNoiseFunction());

	    ExtensionUtil::RegisterFunction(instance, DP_sum());


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
