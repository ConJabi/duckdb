#pragma once

#include <cstdarg>
#include <cstdint>
#include <cstdlib>
#include <ostream>
#include <new>

struct AnyDomain;

struct AnyMeasure;

struct AnyMetric;

/// A struct that can wrap any object.
struct AnyObject;

/// A mathematical function which maps values from an input [`Domain`] to an output [`Domain`].
template<typename TI = void, typename TO = void>
struct Function;

/// A randomized mechanism with certain privacy characteristics.
///
/// The trait bounds provided by the Rust type system guarantee that:
/// * `input_domain` and `output_domain` are valid domains
/// * `input_metric` is a valid metric
/// * `output_measure` is a valid measure
///
/// It is, however, left to constructor functions to prove that:
/// * `input_metric` is compatible with `input_domain`
/// * `privacy_map` is a mapping from the input metric to the output measure
template<typename DI = void, typename TO = void, typename MI = void, typename MO = void>
struct Measurement;

/// A data transformation with certain stability characteristics.
///
/// The trait bounds provided by the Rust type system guarantee that:
/// * `input_domain` and `output_domain` are valid domains
/// * `input_metric` and `output_metric` are valid metrics
///
/// It is, however, left to constructor functions to prove that:
/// * metrics are compatible with domains
/// * `function` is a mapping from the input domain to the output domain
/// * `stability_map` is a mapping from the input metric to the output metric
template<typename DI = void, typename DO = void, typename MI = void, typename MO = void>
struct Transformation;

struct FfiError {
    char *variant;
    char *message;
    char *backtrace;
};

template<typename T>
struct FfiResult {
    enum class Tag : uint32_t {
        Ok,
        Err,
    };

    struct Ok_Body {
        T _0;
    };

    struct Err_Body {
        FfiError *_0;
    };

    Tag tag;
    union {
        Ok_Body ok;
        Err_Body err;
    };
};

/// A Measurement with all generic types filled by Any types. This is the type of Measurements
/// passed back and forth over FFI.
using AnyMeasurement = Measurement<AnyDomain, AnyObject, AnyMetric, AnyMeasure>;

/// A Transformation with all generic types filled by Any types. This is the type of Transformation
/// passed back and forth over FFI.
using AnyTransformation = Transformation<AnyDomain, AnyDomain, AnyMetric, AnyMetric>;

using AnyFunction = Function<AnyObject, AnyObject>;

using c_bool = uint8_t;

using CallbackFn = FfiResult<AnyObject *> *(*)(const AnyObject *);

using TransitionFn = FfiResult<AnyObject *> *(*)(const AnyObject *, c_bool);

struct FfiSlice {
    const void *ptr;
    uintptr_t len;
};

using RefCountFn = bool (*)(const void *, bool);

struct ExtrinsicObject {
    const void *ptr;
    RefCountFn count;
};

extern "C" {

FfiResult<AnyObject *> opendp_accuracy__describe_polars_measurement_accuracy(const AnyMeasurement *measurement,
                                                                             const AnyObject *alpha);

FfiResult<AnyMeasurement *> opendp_combinators__make_population_amplification(const AnyMeasurement *measurement,
                                                                              unsigned int population_size);

FfiResult<AnyMeasurement *> opendp_combinators__make_chain_mt(const AnyMeasurement *measurement1,
                                                              const AnyTransformation *transformation0);

FfiResult<AnyTransformation *> opendp_combinators__make_chain_tt(const AnyTransformation *transformation1,
                                                                 const AnyTransformation *transformation0);

FfiResult<AnyMeasurement *> opendp_combinators__make_chain_pm(const AnyFunction *postprocess1,
                                                              const AnyMeasurement *measurement0);

FfiResult<AnyMeasurement *> opendp_combinators__make_basic_composition(const AnyObject *measurements);

FfiResult<AnyMeasurement *> opendp_combinators__make_sequential_composition(const AnyDomain *input_domain,
                                                                            const AnyMetric *input_metric,
                                                                            const AnyMeasure *output_measure,
                                                                            const AnyObject *d_in,
                                                                            const AnyObject *d_mids);

FfiResult<AnyMeasurement *> opendp_combinators__make_zCDP_to_approxDP(const AnyMeasurement *measurement);

FfiResult<AnyMeasurement *> opendp_combinators__make_pureDP_to_fixed_approxDP(const AnyMeasurement *measurement);

FfiResult<AnyMeasurement *> opendp_combinators__make_pureDP_to_zCDP(const AnyMeasurement *measurement);

FfiResult<AnyMeasurement *> opendp_combinators__make_fix_delta(const AnyMeasurement *measurement,
                                                               const AnyObject *delta);

/// Internal function. Free the memory associated with `error`.
///
/// # Returns
/// A boolean, where true indicates successful free
bool opendp_core___error_free(FfiError *this_);

/// Get the input domain from a `transformation`.
///
/// # Arguments
/// * `this` - The transformation to retrieve the value from.
FfiResult<AnyDomain *> opendp_core__transformation_input_domain(AnyTransformation *this_);

/// Get the output domain from a `transformation`.
///
/// # Arguments
/// * `this` - The transformation to retrieve the value from.
FfiResult<AnyDomain *> opendp_core__transformation_output_domain(AnyTransformation *this_);

/// Get the input domain from a `transformation`.
///
/// # Arguments
/// * `this` - The transformation to retrieve the value from.
FfiResult<AnyMetric *> opendp_core__transformation_input_metric(AnyTransformation *this_);

/// Get the output domain from a `transformation`.
///
/// # Arguments
/// * `this` - The transformation to retrieve the value from.
FfiResult<AnyMetric *> opendp_core__transformation_output_metric(AnyTransformation *this_);

/// Get the input domain from a `measurement`.
///
/// # Arguments
/// * `this` - The measurement to retrieve the value from.
FfiResult<AnyDomain *> opendp_core__measurement_input_domain(AnyMeasurement *this_);

/// Get the input domain from a `measurement`.
///
/// # Arguments
/// * `this` - The measurement to retrieve the value from.
FfiResult<AnyMetric *> opendp_core__measurement_input_metric(AnyMeasurement *this_);

/// Get the output domain from a `measurement`.
///
/// # Arguments
/// * `this` - The measurement to retrieve the value from.
FfiResult<AnyMeasure *> opendp_core__measurement_output_measure(AnyMeasurement *this_);

/// Get the function from a measurement.
///
/// # Arguments
/// * `this` - The measurement to retrieve the value from.
FfiResult<AnyFunction *> opendp_core__measurement_function(AnyMeasurement *this_);

/// Use the `transformation` to map a given `d_in` to `d_out`.
///
/// # Arguments
/// * `transformation` - Transformation to check the map distances with.
/// * `distance_in` - Distance in terms of the input metric.
FfiResult<AnyObject *> opendp_core__transformation_map(const AnyTransformation *transformation,
                                                       const AnyObject *distance_in);

/// Check the privacy relation of the `measurement` at the given `d_in`, `d_out`
///
/// # Arguments
/// * `measurement` - Measurement to check the privacy relation of.
/// * `d_in` - Distance in terms of the input metric.
/// * `d_out` - Distance in terms of the output metric.
///
/// # Returns
/// True indicates that the relation passed at the given distance.
FfiResult<c_bool *> opendp_core__transformation_check(const AnyTransformation *transformation,
                                                      const AnyObject *distance_in,
                                                      const AnyObject *distance_out);

/// Use the `measurement` to map a given `d_in` to `d_out`.
///
/// # Arguments
/// * `measurement` - Measurement to check the map distances with.
/// * `distance_in` - Distance in terms of the input metric.
FfiResult<AnyObject *> opendp_core__measurement_map(const AnyMeasurement *measurement,
                                                    const AnyObject *distance_in);

/// Check the privacy relation of the `measurement` at the given `d_in`, `d_out`
///
/// # Arguments
/// * `measurement` - Measurement to check the privacy relation of.
/// * `d_in` - Distance in terms of the input metric.
/// * `d_out` - Distance in terms of the output metric.
///
/// # Returns
/// True indicates that the relation passed at the given distance.
FfiResult<c_bool *> opendp_core__measurement_check(const AnyMeasurement *measurement,
                                                   const AnyObject *distance_in,
                                                   const AnyObject *distance_out);

/// Invoke the `measurement` with `arg`. Returns a differentially private release.
///
/// # Arguments
/// * `this` - Measurement to invoke.
/// * `arg` - Input data to supply to the measurement. A member of the measurement's input domain.
FfiResult<AnyObject *> opendp_core__measurement_invoke(const AnyMeasurement *this_,
                                                       const AnyObject *arg);

/// Internal function. Free the memory associated with `this`.
FfiResult<void *> opendp_core___measurement_free(AnyMeasurement *this_);

/// Invoke the `transformation` with `arg`. Returns a differentially private release.
///
/// # Arguments
/// * `this` - Transformation to invoke.
/// * `arg` - Input data to supply to the transformation. A member of the transformation's input domain.
FfiResult<AnyObject *> opendp_core__transformation_invoke(const AnyTransformation *this_,
                                                          const AnyObject *arg);

/// Get the function from a transformation.
///
/// # Arguments
/// * `this` - The transformation to retrieve the value from.
FfiResult<AnyFunction *> opendp_core__transformation_function(AnyTransformation *this_);

/// Internal function. Free the memory associated with `this`.
FfiResult<void *> opendp_core___transformation_free(AnyTransformation *this_);

/// Get the input (carrier) data type of `this`.
///
/// # Arguments
/// * `this` - The transformation to retrieve the type from.
FfiResult<char *> opendp_core__transformation_input_carrier_type(AnyTransformation *this_);

/// Get the input (carrier) data type of `this`.
///
/// # Arguments
/// * `this` - The measurement to retrieve the type from.
FfiResult<char *> opendp_core__measurement_input_carrier_type(AnyMeasurement *this_);

/// Get the input distance type of `transformation`.
///
/// # Arguments
/// * `this` - The transformation to retrieve the type from.
FfiResult<char *> opendp_core__transformation_input_distance_type(AnyTransformation *this_);

/// Get the output distance type of `transformation`.
///
/// # Arguments
/// * `this` - The transformation to retrieve the type from.
FfiResult<char *> opendp_core__transformation_output_distance_type(AnyTransformation *this_);

/// Get the input distance type of `measurement`.
///
/// # Arguments
/// * `this` - The measurement to retrieve the type from.
FfiResult<char *> opendp_core__measurement_input_distance_type(AnyMeasurement *this_);

/// Get the output distance type of `measurement`.
///
/// # Arguments
/// * `this` - The measurement to retrieve the type from.
FfiResult<char *> opendp_core__measurement_output_distance_type(AnyMeasurement *this_);

FfiResult<AnyFunction *> opendp_core__new_function(CallbackFn function, const char *TO);

/// Eval the `function` with `arg`.
///
/// # Arguments
/// * `this` - Function to invoke.
/// * `arg` - Input data to supply to the measurement. A member of the measurement's input domain.
/// * `TI` - Input Type.
FfiResult<AnyObject *> opendp_core__function_eval(const AnyFunction *this_,
                                                  const AnyObject *arg,
                                                  const char *TI);

/// Internal function. Free the memory associated with `this`.
FfiResult<void *> opendp_core___function_free(AnyFunction *this_);

/// Invoke the `queryable` with `query`. Returns a differentially private release.
///
/// # Arguments
/// * `queryable` - Queryable to eval.
/// * `query` - Input data to supply to the measurement. A member of the measurement's input domain.
FfiResult<AnyObject *> opendp_core__queryable_eval(AnyObject *queryable, const AnyObject *query);

/// Get the query type of `queryable`.
///
/// # Arguments
/// * `this` - The queryable to retrieve the query type from.
FfiResult<char *> opendp_core__queryable_query_type(AnyObject *this_);

FfiResult<AnyObject *> opendp_core__new_queryable(TransitionFn transition,
                                                  const char *Q,
                                                  const char *A);

/// Internal function. Load data from a `slice` into an AnyObject
///
/// # Arguments
/// * `raw` - A pointer to the slice with data.
/// * `T` - The type of the data in the slice.
///
/// # Returns
/// An AnyObject that contains the data in `slice`.
/// The AnyObject also captures rust type information.
FfiResult<AnyObject *> opendp_data__slice_as_object(const FfiSlice *raw, const char *T);

/// Internal function. Retrieve the type descriptor string of an AnyObject.
///
/// # Arguments
/// * `this` - A pointer to the AnyObject.
FfiResult<char *> opendp_data__object_type(AnyObject *this_);

/// Internal function. Unload data from an AnyObject into an FfiSlicePtr.
///
/// # Arguments
/// * `obj` - A pointer to the AnyObject to unpack.
///
/// # Returns
/// An FfiSlice that contains the data in FfiObject, but in a format readable in bindings languages.
FfiResult<FfiSlice *> opendp_data__object_as_slice(const AnyObject *obj);

/// Internal function. Converts an FfiSlice of AnyObjects to an FfiSlice of AnyObjectPtrs.
///
/// # Arguments
/// * `raw` - A pointer to the slice to free.
FfiResult<FfiSlice *> opendp_data__ffislice_of_anyobjectptrs(const FfiSlice *raw);

/// Internal function. Free the memory associated with `this`, an AnyObject.
///
/// # Arguments
/// * `this` - A pointer to the AnyObject to free.
FfiResult<void *> opendp_data__object_free(AnyObject *this_);

/// Internal function. Free the memory associated with `this`, an FfiSlicePtr.
/// Used to clean up after object_as_slice.
/// Frees the slice, but not what the slice references!
///
/// # Arguments
/// * `this` - A pointer to the FfiSlice to free.
FfiResult<void *> opendp_data__slice_free(FfiSlice *this_);

/// Internal function. Free the memory associated with `this`, a slice containing an Arrow array, schema, and name.
FfiResult<void *> opendp_data__arrow_array_free(void *this_);

/// Internal function. Free the memory associated with `this`, a string.
/// Used to clean up after the type getter functions.
///
/// # Arguments
/// * `this` - A pointer to the string to free.
FfiResult<void *> opendp_data__str_free(char *this_);

/// Internal function. Free the memory associated with `this`, a bool.
/// Used to clean up after the relation check.
///
/// # Arguments
/// * `this` - A pointer to the bool to free.
FfiResult<void *> opendp_data__bool_free(c_bool *this_);

/// Internal function. Free the memory associated with `this`, a string.
/// Used to clean up after the type getter functions.
FfiResult<void *> opendp_data__extrinsic_object_free(ExtrinsicObject *this_);

/// Internal function. Use an SMDCurve to find epsilon at a given `delta`.
///
/// # Arguments
/// * `curve` - The SMDCurve.
/// * `delta` - What to fix delta to compute epsilon.
///
/// # Returns
/// Epsilon at a given `delta`.
FfiResult<AnyObject *> opendp_data__smd_curve_epsilon(const AnyObject *curve,
                                                      const AnyObject *delta);

/// wrap an AnyObject in an FfiResult::Ok(this)
///
/// # Arguments
/// * `this` - The AnyObject to wrap.
const FfiResult<const AnyObject *> *ffiresult_ok(const AnyObject *this_);

/// construct an FfiResult::Err(e)
///
/// # Arguments
/// * `message` - The error message.
/// * `backtrace` - The error backtrace.
const FfiResult<const AnyObject *> *ffiresult_err(char *message, char *backtrace);

/// Internal function. Populate the buffer behind `ptr` with `len` random bytes
/// sampled from a cryptographically secure RNG.
bool opendp_data__fill_bytes(uint8_t *ptr, uintptr_t len);

/// Internal function. Collects a DataFrame from a OnceFrame, exhausting the OnceFrame.
///
/// # Arguments
/// * `onceframe` - The queryable holding a LazyFrame.
FfiResult<AnyObject *> opendp_data__onceframe_collect(AnyObject *onceframe);

/// Internal function. Extracts a LazyFrame from a OnceFrame,
/// circumventing protections against multiple evaluations.
///
/// Each collection consumes the entire allocated privacy budget.
/// To remain DP at the advertised privacy level, only collect the LazyFrame once.
///
/// # Features
/// * `honest-but-curious` - LazyFrames can be collected an unlimited number of times.
///
/// # Arguments
/// * `onceframe` - The queryable holding a LazyFrame.
FfiResult<AnyObject *> opendp_data__onceframe_lazy(AnyObject *onceframe);

/// Internal function. Free the memory associated with `this`.
FfiResult<void *> opendp_domains___domain_free(AnyDomain *this_);

/// Check membership in a `domain`.
///
/// # Arguments
/// * `this` - The domain to check membership in.
/// * `val` - A potential element of the domain.
FfiResult<c_bool *> opendp_domains__member(AnyDomain *this_, const AnyObject *val);

/// Debug a `domain`.
///
/// # Arguments
/// * `this` - The domain to debug (stringify).
FfiResult<char *> opendp_domains__domain_debug(AnyDomain *this_);

/// Get the type of a `domain`.
///
/// # Arguments
/// * `this` - The domain to retrieve the type from.
FfiResult<char *> opendp_domains__domain_type(AnyDomain *this_);

/// Get the carrier type of a `domain`.
///
/// # Arguments
/// * `this` - The domain to retrieve the carrier type from.
FfiResult<char *> opendp_domains__domain_carrier_type(AnyDomain *this_);

FfiResult<AnyDomain *> opendp_domains__atom_domain(const AnyObject *bounds,
                                                   c_bool nullable,
                                                   const char *T);

FfiResult<AnyDomain *> opendp_domains__option_domain(const AnyDomain *element_domain, const char *D);

/// Construct an instance of `VectorDomain`.
///
/// # Arguments
/// * `atom_domain` - The inner domain.
FfiResult<AnyDomain *> opendp_domains__vector_domain(const AnyDomain *atom_domain,
                                                     const AnyObject *size);

/// Construct an instance of `MapDomain`.
///
/// # Arguments
/// * `key_domain` - domain of keys in the hashmap
/// * `value_domain` - domain of values in the hashmap
FfiResult<AnyDomain *> opendp_domains__map_domain(const AnyDomain *key_domain,
                                                  const AnyDomain *value_domain);

/// Construct a new UserDomain.
/// Any two instances of an UserDomain are equal if their string descriptors are equal.
/// Contains a function used to check if any value is a member of the domain.
///
/// # Arguments
/// * `identifier` - A string description of the data domain.
/// * `member` - A function used to test if a value is a member of the data domain.
/// * `descriptor` - Additional constraints on the domain.
FfiResult<AnyDomain *> opendp_domains__user_domain(char *identifier,
                                                   CallbackFn member,
                                                   ExtrinsicObject *descriptor);

/// Retrieve the descriptor value stored in a user domain.
///
/// # Arguments
/// * `domain` - The UserDomain to extract the descriptor from
FfiResult<ExtrinsicObject *> opendp_domains___user_domain_descriptor(AnyDomain *domain);

/// Construct an instance of `LazyFrameDomain`.
///
/// # Arguments
/// * `series_domains` - Domain of each series in the lazyframe.
FfiResult<AnyDomain *> opendp_domains__lazyframe_domain(AnyObject *series_domains);

FfiResult<AnyObject *> opendp_domains___lazyframe_from_domain(AnyDomain *domain);

FfiResult<AnyDomain *> opendp_domains__with_margin(AnyDomain *frame_domain,
                                                   AnyObject *by,
                                                   AnyObject *max_partition_length,
                                                   AnyObject *max_num_partitions,
                                                   AnyObject *max_partition_contributions,
                                                   AnyObject *max_influenced_partitions,
                                                   char *public_info);

FfiResult<AnyDomain *> opendp_domains__series_domain(char *name, const AnyDomain *element_domain);

/// Construct an ExprDomain from a LazyFrameDomain.
///
/// Must pass either `context` or `grouping_columns`.
///
/// # Arguments
/// * `lazyframe_domain` - the domain of the LazyFrame to be constructed
/// * `grouping_columns` - set when creating an expression that aggregates
FfiResult<AnyDomain *> opendp_domains__expr_domain(const AnyDomain *lazyframe_domain,
                                                   const AnyObject *grouping_columns);

FfiResult<AnyMeasurement *> opendp_measurements__make_gaussian(const AnyDomain *input_domain,
                                                               const AnyMetric *input_metric,
                                                               const void *scale,
                                                               const int32_t *k,
                                                               const char *MO);

FfiResult<AnyMeasurement *> opendp_measurements__make_geometric(const AnyDomain *input_domain,
                                                                const AnyMetric *input_metric,
                                                                const void *scale,
                                                                const AnyObject *bounds,
                                                                const char *QO);

FfiResult<AnyMeasurement *> opendp_measurements__make_report_noisy_max_gumbel(const AnyDomain *input_domain,
                                                                              const AnyMetric *input_metric,
                                                                              const AnyObject *scale,
                                                                              const char *optimize,
                                                                              const char *QO);

FfiResult<AnyMeasurement *> opendp_measurements__make_laplace(const AnyDomain *input_domain,
                                                              const AnyMetric *input_metric,
                                                              const void *scale,
                                                              const int32_t *k,
                                                              const char *QO);

FfiResult<AnyMeasurement *> opendp_measurements__make_private_expr(const AnyDomain *input_domain,
                                                                   const AnyMetric *input_metric,
                                                                   const AnyMeasure *output_measure,
                                                                   const AnyObject *expr,
                                                                   const AnyObject *global_scale);

FfiResult<AnyMeasurement *> opendp_measurements__make_private_lazyframe(const AnyDomain *input_domain,
                                                                        const AnyMetric *input_metric,
                                                                        const AnyMeasure *output_measure,
                                                                        const AnyObject *lazyframe,
                                                                        const AnyObject *global_scale);

FfiResult<AnyMeasurement *> opendp_measurements__make_user_measurement(const AnyDomain *input_domain,
                                                                       const AnyMetric *input_metric,
                                                                       const AnyMeasure *output_measure,
                                                                       CallbackFn function,
                                                                       CallbackFn privacy_map,
                                                                       const char *TO);

FfiResult<AnyMeasurement *> opendp_measurements__make_laplace_threshold(const AnyDomain *input_domain,
                                                                        const AnyMetric *input_metric,
                                                                        const void *scale,
                                                                        const void *threshold,
                                                                        long k);

FfiResult<AnyMeasurement *> opendp_measurements__make_randomized_response_bool(const void *prob,
                                                                               c_bool constant_time,
                                                                               const char *QO);

FfiResult<AnyMeasurement *> opendp_measurements__make_randomized_response(const AnyObject *categories,
                                                                          const void *prob,
                                                                          c_bool constant_time,
                                                                          const char *T,
                                                                          const char *QO);

FfiResult<AnyMeasurement *> opendp_measurements__make_alp_queryable(const AnyDomain *input_domain,
                                                                    const AnyMetric *input_metric,
                                                                    const void *scale,
                                                                    const void *total_limit,
                                                                    const void *value_limit,
                                                                    const unsigned int *size_factor,
                                                                    const void *alpha,
                                                                    const char *CO);

/// Internal function. Free the memory associated with `this`.
FfiResult<void *> opendp_measures___measure_free(AnyMeasure *this_);

/// Debug a `measure`.
///
/// # Arguments
/// * `this` - The measure to debug (stringify).
FfiResult<char *> opendp_measures__measure_debug(AnyMeasure *this_);

/// Get the type of a `measure`.
///
/// # Arguments
/// * `this` - The measure to retrieve the type from.
FfiResult<char *> opendp_measures__measure_type(AnyMeasure *this_);

/// Get the distance type of a `measure`.
///
/// # Arguments
/// * `this` - The measure to retrieve the distance type from.
FfiResult<char *> opendp_measures__measure_distance_type(AnyMeasure *this_);

FfiResult<AnyMeasure *> opendp_measures__max_divergence(const char *T);

FfiResult<AnyMeasure *> opendp_measures__smoothed_max_divergence(const char *T);

FfiResult<AnyMeasure *> opendp_measures__fixed_smoothed_max_divergence(const char *T);

FfiResult<AnyMeasure *> opendp_measures__zero_concentrated_divergence(const char *T);

/// Construct a new UserDivergence.
/// Any two instances of an UserDivergence are equal if their string descriptors are equal.
///
/// # Arguments
/// * `descriptor` - A string description of the privacy measure.
FfiResult<AnyMeasure *> opendp_measures__user_divergence(char *descriptor);

/// Internal function. Free the memory associated with `this`.
FfiResult<void *> opendp_metrics___metric_free(AnyMetric *this_);

/// Debug a `metric`.
///
/// # Arguments
/// * `this` - The metric to debug (stringify).
FfiResult<char *> opendp_metrics__metric_debug(AnyMetric *this_);

/// Get the type of a `metric`.
///
/// # Arguments
/// * `this` - The metric to retrieve the type from.
FfiResult<char *> opendp_metrics__metric_type(AnyMetric *this_);

/// Get the distance type of a `metric`.
///
/// # Arguments
/// * `this` - The metric to retrieve the distance type from.
FfiResult<char *> opendp_metrics__metric_distance_type(AnyMetric *this_);

/// Construct an instance of the `SymmetricDistance` metric.
FfiResult<AnyMetric *> opendp_metrics__symmetric_distance();

/// Construct an instance of the `InsertDeleteDistance` metric.
FfiResult<AnyMetric *> opendp_metrics__insert_delete_distance();

/// Construct an instance of the `ChangeOneDistance` metric.
FfiResult<AnyMetric *> opendp_metrics__change_one_distance();

/// Construct an instance of the `HammingDistance` metric.
FfiResult<AnyMetric *> opendp_metrics__hamming_distance();

FfiResult<AnyMetric *> opendp_metrics__absolute_distance(const char *T);

FfiResult<AnyMetric *> opendp_metrics__l1_distance(const char *T);

FfiResult<AnyMetric *> opendp_metrics__l2_distance(const char *T);

/// Construct an instance of the `DiscreteDistance` metric.
FfiResult<AnyMetric *> opendp_metrics__discrete_distance();

FfiResult<AnyMetric *> opendp_metrics__partition_distance(const AnyMetric *metric);

FfiResult<AnyMetric *> opendp_metrics__linf_distance(c_bool monotonic, const char *T);

/// Construct a new UserDistance.
/// Any two instances of an UserDistance are equal if their string descriptors are equal.
///
/// # Arguments
/// * `descriptor` - A string description of the metric.
FfiResult<AnyMetric *> opendp_metrics__user_distance(char *descriptor);

FfiResult<AnyTransformation *> opendp_transformations__make_stable_lazyframe(const AnyDomain *input_domain,
                                                                             const AnyMetric *input_metric,
                                                                             const AnyObject *lazyframe);

FfiResult<AnyTransformation *> opendp_transformations__make_stable_expr(const AnyDomain *input_domain,
                                                                        const AnyMetric *input_metric,
                                                                        const AnyObject *expr);

FfiResult<AnyTransformation *> opendp_transformations__make_sized_bounded_covariance(unsigned int size,
                                                                                     const AnyObject *bounds_0,
                                                                                     const AnyObject *bounds_1,
                                                                                     unsigned int ddof,
                                                                                     const char *S);

FfiResult<AnyTransformation *> opendp_transformations__make_b_ary_tree(const AnyDomain *input_domain,
                                                                       const AnyMetric *input_metric,
                                                                       uint32_t leaf_count,
                                                                       uint32_t branching_factor);

uint32_t opendp_transformations__choose_branching_factor(uint32_t size_guess);

FfiResult<AnyFunction *> opendp_transformations__make_consistent_b_ary_tree(uint32_t branching_factor,
                                                                            const char *TIA,
                                                                            const char *TOA);

FfiResult<AnyTransformation *> opendp_transformations__make_df_cast_default(const AnyDomain *input_domain,
                                                                            const AnyMetric *input_metric,
                                                                            const AnyObject *column_name,
                                                                            const char *TIA,
                                                                            const char *TOA);

FfiResult<AnyTransformation *> opendp_transformations__make_df_is_equal(const AnyDomain *input_domain,
                                                                        const AnyMetric *input_metric,
                                                                        const AnyObject *column_name,
                                                                        const AnyObject *value,
                                                                        const char *TIA);

FfiResult<AnyTransformation *> opendp_transformations__make_split_lines();

FfiResult<AnyTransformation *> opendp_transformations__make_split_records(const char *separator);

FfiResult<AnyTransformation *> opendp_transformations__make_create_dataframe(const AnyObject *col_names,
                                                                             const char *K);

FfiResult<AnyTransformation *> opendp_transformations__make_split_dataframe(const char *separator,
                                                                            const AnyObject *col_names,
                                                                            const char *K);

FfiResult<AnyTransformation *> opendp_transformations__make_select_column(const AnyObject *key,
                                                                          const char *K,
                                                                          const char *TOA);

FfiResult<AnyTransformation *> opendp_transformations__make_subset_by(const AnyObject *indicator_column,
                                                                      const AnyObject *keep_columns,
                                                                      const char *TK);

FfiResult<AnyTransformation *> opendp_transformations__make_quantile_score_candidates(const AnyDomain *input_domain,
                                                                                      const AnyMetric *input_metric,
                                                                                      const AnyObject *candidates,
                                                                                      double alpha);

FfiResult<AnyTransformation *> opendp_transformations__make_identity(const AnyDomain *domain,
                                                                     const AnyMetric *metric);

FfiResult<AnyTransformation *> opendp_transformations__make_is_equal(const AnyDomain *input_domain,
                                                                     const AnyMetric *input_metric,
                                                                     const AnyObject *value);

FfiResult<AnyTransformation *> opendp_transformations__make_is_null(const AnyDomain *input_domain,
                                                                    const AnyMetric *input_metric);

FfiResult<AnyTransformation *> opendp_transformations__make_sum(const AnyDomain *input_domain,
                                                                const AnyMetric *input_metric);

FfiResult<AnyTransformation *> opendp_transformations__make_sized_bounded_int_checked_sum(unsigned int size,
                                                                                          const AnyObject *bounds,
                                                                                          const char *T);

FfiResult<AnyTransformation *> opendp_transformations__make_bounded_int_monotonic_sum(const AnyObject *bounds,
                                                                                      const char *T);

FfiResult<AnyTransformation *> opendp_transformations__make_sized_bounded_int_monotonic_sum(unsigned int size,
                                                                                            const AnyObject *bounds,
                                                                                            const char *T);

FfiResult<AnyTransformation *> opendp_transformations__make_bounded_int_ordered_sum(const AnyObject *bounds,
                                                                                    const char *T);

FfiResult<AnyTransformation *> opendp_transformations__make_sized_bounded_int_ordered_sum(unsigned int size,
                                                                                          const AnyObject *bounds,
                                                                                          const char *T);

FfiResult<AnyTransformation *> opendp_transformations__make_bounded_int_split_sum(const AnyObject *bounds,
                                                                                  const char *T);

FfiResult<AnyTransformation *> opendp_transformations__make_sized_bounded_int_split_sum(unsigned int size,
                                                                                        const AnyObject *bounds,
                                                                                        const char *T);

FfiResult<AnyTransformation *> opendp_transformations__make_bounded_float_checked_sum(unsigned int size_limit,
                                                                                      const AnyObject *bounds,
                                                                                      const char *S);

FfiResult<AnyTransformation *> opendp_transformations__make_sized_bounded_float_checked_sum(unsigned int size,
                                                                                            const AnyObject *bounds,
                                                                                            const char *S);

FfiResult<AnyTransformation *> opendp_transformations__make_bounded_float_ordered_sum(unsigned int size_limit,
                                                                                      const AnyObject *bounds,
                                                                                      const char *S);

FfiResult<AnyTransformation *> opendp_transformations__make_sized_bounded_float_ordered_sum(unsigned int size,
                                                                                            const AnyObject *bounds,
                                                                                            const char *S);

FfiResult<AnyTransformation *> opendp_transformations__make_sum_of_squared_deviations(const AnyDomain *input_domain,
                                                                                      const AnyMetric *input_metric,
                                                                                      const char *S);

FfiResult<AnyTransformation *> opendp_transformations__make_count(const AnyDomain *input_domain,
                                                                  const AnyMetric *input_metric,
                                                                  const char *TO);

FfiResult<AnyTransformation *> opendp_transformations__make_count_distinct(const AnyDomain *input_domain,
                                                                           const AnyMetric *input_metric,
                                                                           const char *TO);

FfiResult<AnyTransformation *> opendp_transformations__make_count_by_categories(const AnyDomain *input_domain,
                                                                                const AnyMetric *input_metric,
                                                                                const AnyObject *categories,
                                                                                c_bool null_category,
                                                                                const char *MO,
                                                                                const char *TO);

FfiResult<AnyTransformation *> opendp_transformations__make_count_by(const AnyDomain *input_domain,
                                                                     const AnyMetric *input_metric,
                                                                     const char *MO,
                                                                     const char *TV);

FfiResult<AnyFunction *> opendp_transformations__make_cdf(const char *TA);

FfiResult<AnyFunction *> opendp_transformations__make_quantiles_from_counts(const AnyObject *bin_edges,
                                                                            const AnyObject *alphas,
                                                                            const char *interpolation,
                                                                            const char *TA,
                                                                            const char *F);

FfiResult<AnyTransformation *> opendp_transformations__make_mean(const AnyDomain *input_domain,
                                                                 const AnyMetric *input_metric);

FfiResult<AnyTransformation *> opendp_transformations__make_variance(const AnyDomain *input_domain,
                                                                     const AnyMetric *input_metric,
                                                                     unsigned int ddof,
                                                                     const char *S);

FfiResult<AnyTransformation *> opendp_transformations__make_impute_uniform_float(const AnyDomain *input_domain,
                                                                                 const AnyMetric *input_metric,
                                                                                 const AnyObject *bounds);

FfiResult<AnyTransformation *> opendp_transformations__make_impute_constant(const AnyDomain *input_domain,
                                                                            const AnyMetric *input_metric,
                                                                            const AnyObject *constant);

FfiResult<AnyTransformation *> opendp_transformations__make_drop_null(const AnyDomain *input_domain,
                                                                      const AnyMetric *input_metric);

FfiResult<AnyTransformation *> opendp_transformations__make_find(const AnyDomain *input_domain,
                                                                 const AnyMetric *input_metric,
                                                                 const AnyObject *categories);

FfiResult<AnyTransformation *> opendp_transformations__make_find_bin(const AnyDomain *input_domain,
                                                                     const AnyMetric *input_metric,
                                                                     const AnyObject *edges);

FfiResult<AnyTransformation *> opendp_transformations__make_index(const AnyDomain *input_domain,
                                                                  const AnyMetric *input_metric,
                                                                  const AnyObject *categories,
                                                                  const AnyObject *null,
                                                                  const char *TOA);

FfiResult<AnyTransformation *> opendp_transformations__make_lipschitz_float_mul(const void *constant,
                                                                                const AnyObject *bounds,
                                                                                const char *D,
                                                                                const char *M);

/// Construct a Transformation from user-defined callbacks.
///
/// # Arguments
/// * `input_domain` - A domain describing the set of valid inputs for the function.
/// * `input_metric` - The metric from which distances between adjacent inputs are measured.
/// * `output_domain` - A domain describing the set of valid outputs of the function.
/// * `output_metric` - The metric from which distances between outputs of adjacent inputs are measured.
/// * `function` - A function mapping data from `input_domain` to `output_domain`.
/// * `stability_map` - A function mapping distances from `input_metric` to `output_metric`.
FfiResult<AnyTransformation *> opendp_transformations__make_user_transformation(const AnyDomain *input_domain,
                                                                                const AnyMetric *input_metric,
                                                                                const AnyDomain *output_domain,
                                                                                const AnyMetric *output_metric,
                                                                                CallbackFn function,
                                                                                CallbackFn stability_map);

FfiResult<AnyTransformation *> opendp_transformations__make_clamp(const AnyDomain *input_domain,
                                                                  const AnyMetric *input_metric,
                                                                  const AnyObject *bounds);

FfiResult<AnyTransformation *> opendp_transformations__make_cast(const AnyDomain *input_domain,
                                                                 const AnyMetric *input_metric,
                                                                 const char *TOA);

FfiResult<AnyTransformation *> opendp_transformations__make_cast_default(const AnyDomain *input_domain,
                                                                         const AnyMetric *input_metric,
                                                                         const char *TOA);

FfiResult<AnyTransformation *> opendp_transformations__make_cast_inherent(const AnyDomain *input_domain,
                                                                          const AnyMetric *input_metric,
                                                                          const char *TOA);

FfiResult<AnyTransformation *> opendp_transformations__make_ordered_random(const AnyDomain *input_domain,
                                                                           const AnyMetric *input_metric);

FfiResult<AnyTransformation *> opendp_transformations__make_unordered(const AnyDomain *input_domain,
                                                                      const AnyMetric *input_metric);

FfiResult<AnyTransformation *> opendp_transformations__make_metric_bounded(const AnyDomain *input_domain,
                                                                           const AnyMetric *input_metric);

FfiResult<AnyTransformation *> opendp_transformations__make_metric_unbounded(const AnyDomain *input_domain,
                                                                             const AnyMetric *input_metric);

FfiResult<AnyTransformation *> opendp_transformations__make_resize(const AnyDomain *input_domain,
                                                                   const AnyMetric *input_metric,
                                                                   unsigned int size,
                                                                   const AnyObject *constant,
                                                                   const char *MO);

} // extern "C"
