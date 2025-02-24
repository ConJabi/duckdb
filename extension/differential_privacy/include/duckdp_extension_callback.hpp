#pragma once

#include "duckdb.hpp"
#include "duckdb/planner/extension_callback.hpp"
#include "duckdp_state.hpp"

namespace duckdb {
class DuckDPExtensionCallback : public ExtensionCallback {
	void OnConnectionOpened(ClientContext &context) override {
		context.registered_state->Insert(
			"duckdp", make_shared_ptr<DuckDPState>());
	}
};
} // namespace duckdbs