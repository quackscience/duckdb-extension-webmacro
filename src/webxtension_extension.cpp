#define DUCKDB_EXTENSION_MAIN

#include "webxtension_extension.hpp"
#include "duckdb.hpp"
#include "duckdb/common/exception.hpp"
#include "duckdb/common/string_util.hpp"
#include "duckdb/function/scalar_function.hpp"
#include "duckdb/main/extension_util.hpp"
#include <duckdb/parser/parsed_data/create_scalar_function_info.hpp>

// OpenSSL linked through vcpkg
#include <openssl/opensslv.h>

namespace duckdb {

inline void WebxtensionScalarFun(DataChunk &args, ExpressionState &state, Vector &result) {
    auto &name_vector = args.data[0];
    UnaryExecutor::Execute<string_t, string_t>(
	    name_vector, result, args.size(),
	    [&](string_t name) {
			return StringVector::AddString(result, "Webxtension "+name.GetString()+" 🐥");;
        });
}

inline void WebxtensionOpenSSLVersionScalarFun(DataChunk &args, ExpressionState &state, Vector &result) {
    auto &name_vector = args.data[0];
    UnaryExecutor::Execute<string_t, string_t>(
	    name_vector, result, args.size(),
	    [&](string_t name) {
			return StringVector::AddString(result, "Webxtension " + name.GetString() +
                                                     ", my linked OpenSSL version is " +
                                                     OPENSSL_VERSION_TEXT );;
        });
}

static void LoadInternal(DatabaseInstance &instance) {
    // Register a scalar function
    auto webxtension_scalar_function = ScalarFunction("webxtension", {LogicalType::VARCHAR}, LogicalType::VARCHAR, WebxtensionScalarFun);
    ExtensionUtil::RegisterFunction(instance, webxtension_scalar_function);

    // Register another scalar function
    auto webxtension_openssl_version_scalar_function = ScalarFunction("webxtension_openssl_version", {LogicalType::VARCHAR},
                                                LogicalType::VARCHAR, WebxtensionOpenSSLVersionScalarFun);
    ExtensionUtil::RegisterFunction(instance, webxtension_openssl_version_scalar_function);
}

void WebxtensionExtension::Load(DuckDB &db) {
	LoadInternal(*db.instance);
}
std::string WebxtensionExtension::Name() {
	return "webxtension";
}

std::string WebxtensionExtension::Version() const {
#ifdef EXT_VERSION_WEBXTENSION
	return EXT_VERSION_WEBXTENSION;
#else
	return "";
#endif
}

} // namespace duckdb

extern "C" {

DUCKDB_EXTENSION_API void webxtension_init(duckdb::DatabaseInstance &db) {
    duckdb::DuckDB db_wrapper(db);
    db_wrapper.LoadExtension<duckdb::WebxtensionExtension>();
}

DUCKDB_EXTENSION_API const char *webxtension_version() {
	return duckdb::DuckDB::LibraryVersion();
}
}

#ifndef DUCKDB_EXTENSION_MAIN
#error DUCKDB_EXTENSION_MAIN not defined
#endif
