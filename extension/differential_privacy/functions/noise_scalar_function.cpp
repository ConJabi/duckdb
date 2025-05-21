#include "../opendp/rust/opendp.h"

#include "duckdb/main/extension_util.hpp"
#include "functions.hpp"

template <typename T>
T handleFfiResult(const FfiResult<T> &result) {
	if (result.tag == FfiResult<T>::Tag::Ok) {
		return result.ok._0;
	}
	throw std::runtime_error(result.err._0->message);
}

// double make_gaussian(double number, double scale, char *type) {
// 	char *MO = "ZeroConcentratedDivergence<f64>";
// 	char *c_T = type;
// 	bool c_nullable = false;
//
// 	AnyDomain *atom_domain = handleFfiResult(opendp_domains__atom_domain(NULL, c_nullable, c_T));
// 	AnyMetric *metric = handleFfiResult(opendp_metrics__absolute_distance(type));
// 	AnyMeasurement *measurement = handleFfiResult(
// 		opendp_measurements__make_gaussian(atom_domain, metric, &scale, NULL, MO));
//
//
// 	const FfiSlice number_slice = {&number, 1};
//
// 	AnyObject *number_anyobject = handleFfiResult(opendp_data__slice_as_object(&number_slice, type));
// 	const AnyObject *private_anyobject = handleFfiResult(opendp_core__measurement_invoke(measurement, number_anyobject));
//
// 	const void *private_number_ptr = handleFfiResult(opendp_data__object_as_slice(private_anyobject))->ptr;
// 	return *(const double *) private_number_ptr;
// }

// double test (double test){
// 	return make_gaussian(test,0.05, "f64");
// }

template<typename T>
inline void make_gaussian_vec(T *number, T scale, uint32_t size, T *result) {
	const char * type;
	const char * vec_type;
	if (std::is_same<T, float>::value) {
		type = "f32";
		vec_type = "Vec<f32>";

	} else {
		type = "f64";
		vec_type = "Vec<f64>";
	}
	T;
	AnyMeasure *measure = handleFfiResult(opendp_measures__zero_concentrated_divergence());
	char *measure_type = handleFfiResult(opendp_measures__measure_type(measure));

	bool contains_null = false;

	const FfiSlice bool_slice = {&contains_null, 1};
	const AnyObject *bool_anyobject = handleFfiResult(opendp_data__slice_as_object(&bool_slice, "bool"));


	AnyDomain *atom_domain = handleFfiResult(opendp_domains__atom_domain(NULL, bool_anyobject, type));
	const FfiSlice size_slice = {&size, 1};
	const AnyObject *size_anyobject = handleFfiResult(opendp_data__slice_as_object(&size_slice, "i32"));
	AnyDomain *vector_domain = handleFfiResult(opendp_domains__vector_domain(atom_domain, size_anyobject));

	AnyMetric *metric = handleFfiResult(opendp_metrics__l2_distance(type));

	AnyMeasurement *measurement = handleFfiResult(
		opendp_measurements__make_gaussian(vector_domain, metric, scale, NULL, measure_type));


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
template<typename T>
static void NoiseFunction(DataChunk  &args, ExpressionState &state, Vector &result) {
	auto &input_vector = args.data[0];
	auto &scale_vector = args.data[1];

	auto result_data = FlatVector::GetData<T>(result);
	auto input_data = FlatVector::GetData<T>(input_vector);
	auto scale_data = FlatVector::GetData<T>(scale_vector);

	make_gaussian_vec<T>(input_data, scale_data[0], args.size(), result_data);

	// UnaryExecutor::Execute<double, double>(name_vector, result, args.size(),  test); //todo for evaluation purposes
}

ScalarFunctionSet CoreFunctions::GetNoiseFunction() {
	ScalarFunctionSet set("noise");
	set.AddFunction(ScalarFunction( {LogicalType::FLOAT}, LogicalType::FLOAT, NoiseFunction<float>));
	set.AddFunction(ScalarFunction( {LogicalType::DOUBLE, LogicalType::DOUBLE}, LogicalType::DOUBLE, NoiseFunction<double>));
	return set;
}

void CoreFunctions::RegisterNoiseScalarFunction(
	DatabaseInstance &db) {
	ExtensionUtil::RegisterFunction(db, GetNoiseFunction());
}

} // namespace duckdb