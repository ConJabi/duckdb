#include "duckdb.hpp"

namespace duckdb {

//! Class to register the PRAGMA create_inbox function
class CorePragma {
public:
	//! Register the PRAGMA function
	static void Register(DatabaseInstance &instance) {
		RegisterMakeTablePrivate(instance);
		RegisterAddBoundsToColumn(instance);
	}

private:
	static void RegisterMakeTablePrivate(DatabaseInstance &instance);
	static void RegisterAddBoundsToColumn(DatabaseInstance &instance);
};

} // namespace duckdb