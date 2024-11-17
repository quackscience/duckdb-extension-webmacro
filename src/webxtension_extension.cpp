#define DUCKDB_EXTENSION_MAIN

#include "webxtension_extension.hpp"
#include "duckdb.hpp"
#include "duckdb/common/exception.hpp"
#include "duckdb/common/string_util.hpp"
#include "duckdb/function/scalar_function.hpp"
#include "duckdb/main/extension_util.hpp"
#include "duckdb/parser/parsed_data/create_macro_info.hpp"
#include <duckdb/parser/parsed_data/create_scalar_function_info.hpp>
#include "duckdb/common/exception/http_exception.hpp"

#define CPPHTTPLIB_OPENSSL_SUPPORT
#include "httplib.hpp"
#include "yyjson.hpp"

namespace duckdb {

// Helper function to setup HTTP client
static std::pair<duckdb_httplib_openssl::Client, std::string> SetupHttpClient(const std::string &url) {
    std::string scheme, domain, path;
    size_t pos = url.find("://");
    std::string mod_url = url;
    if (pos != std::string::npos) {
        scheme = mod_url.substr(0, pos);
        mod_url.erase(0, pos + 3);
    }

    pos = mod_url.find("/");
    if (pos != std::string::npos) {
        domain = mod_url.substr(0, pos);
        path = mod_url.substr(pos);
    } else {
        domain = mod_url;
        path = "/";
    }

    duckdb_httplib_openssl::Client client(domain.c_str());
    client.set_read_timeout(10, 0);
    client.set_follow_location(true);

    return std::make_pair(std::move(client), path);
}

// Helper function to handle HTTP errors
static void HandleHttpError(const duckdb_httplib_openssl::Result &res, const std::string &request_type) {
    std::string err_message = "HTTP " + request_type + " request failed. ";

    switch (res.error()) {
        case duckdb_httplib_openssl::Error::Connection:
            err_message += "Connection error.";
            break;
        case duckdb_httplib_openssl::Error::Read:
            err_message += "Error reading response.";
            break;
        default:
            err_message += "Unknown error.";
            break;
    }
    throw std::runtime_error(err_message);
}


static bool ContainsMacroDefinition(const std::string &content) {
    std::string upper_content = StringUtil::Upper(content);
    const char* patterns[] = {
        "CREATE MACRO",
        "CREATE OR REPLACE MACRO",
        "CREATE TEMP MACRO",
        "CREATE TEMPORARY MACRO",
        "CREATE OR REPLACE TEMP MACRO",
        "CREATE OR REPLACE TEMPORARY MACRO"
    };
    
    for (const auto& pattern : patterns) {
        if (upper_content.find(pattern) != std::string::npos) {
            return true;
        }
    }
    return false;
}

// Parse Function Name
static std::string ExtractMacroName(const std::string &macro_sql) {
    try {
        // Convert to uppercase for case-insensitive matching
        std::string upper_sql = StringUtil::Upper(macro_sql);
        
        // Find the MACRO keyword
        size_t macro_pos = upper_sql.find("MACRO");
        if (macro_pos == std::string::npos) {
            return "unknown";
        }
        
        // Find the start of the name (after MACRO and any whitespace)
        size_t name_start = macro_pos + 5;  // length of "MACRO"
        while (name_start < upper_sql.length() && std::isspace(upper_sql[name_start])) {
            name_start++;
        }
        
        // Find the end of the name (before the opening parenthesis)
        size_t name_end = upper_sql.find('(', name_start);
        if (name_end == std::string::npos) {
            return "unknown";
        }
        
        // Trim any trailing whitespace
        while (name_end > name_start && std::isspace(upper_sql[name_end - 1])) {
            name_end--;
        }
        
        // Get the original case version of the name from the input string
        return macro_sql.substr(name_start, name_end - name_start);
    } catch (...) {
        return "unknown";
    }
}

// Function to fetch and create macro from URL
static void LoadMacroFromUrlFunction(DataChunk &args, ExpressionState &state, Vector &result, DatabaseInstance *db_instance) {
    auto &context = state.GetContext();

    UnaryExecutor::Execute<string_t, string_t>(
        args.data[0], result, args.size(),
        [&](string_t url) {
            try {
                // Setup HTTP client
                auto client_and_path = SetupHttpClient(url.GetString());
                auto &client = client_and_path.first;
                auto &path = client_and_path.second;

                // Make GET request
                auto res = client.Get(path.c_str());
                if (!res) {
                    HandleHttpError(res, "GET");
                }

                if (res->status != 200) {
                    throw std::runtime_error("HTTP error " + std::to_string(res->status) + ": " + res->reason);
                }

                // Get the SQL content
                std::string macro_sql = res->body;

		// Replace all \r\n with \n
                macro_sql = StringUtil::Replace(macro_sql, "\r\n", "\n");
                // Replace any remaining \r with \n
                macro_sql = StringUtil::Replace(macro_sql, "\r", "\n");
                // Normalize multiple newlines to single newlines
                macro_sql = StringUtil::Replace(macro_sql, "\n\n", "\n");
                // Trim in place
                StringUtil::Trim(macro_sql);

                if (!ContainsMacroDefinition(macro_sql)) {
                    throw std::runtime_error("URL content does not contain a valid macro definition");
                }

		//std::cout << macro_sql << "\n";
		Connection conn(*db_instance);

		// Execute the macro directly in the current context
                auto query_result = conn.Query(macro_sql);

                if (query_result->HasError()) {
                    throw std::runtime_error("Failed loading Macro: " + query_result->GetError());
                }

		// Extract macro name before executing
                std::string macro_name = ExtractMacroName(macro_sql);

                return StringVector::AddString(result, "Successfully loaded macro: " + macro_name);

            } catch (std::exception &e) {
                std::string error_msg = "Error: " + std::string(e.what());
                throw std::runtime_error(error_msg);
            }
        });
}

static void LoadInternal(DatabaseInstance &instance) {
    // Create lambda to capture database instance
    auto load_macro_func = [&instance](DataChunk &args, ExpressionState &state, Vector &result) {
        LoadMacroFromUrlFunction(args, state, result, &instance);
    };

    // Register function with captured database instance
    ExtensionUtil::RegisterFunction(
        instance,
        ScalarFunction("load_macro_from_url", {LogicalType::VARCHAR}, 
                      LogicalType::VARCHAR, load_macro_func)
    );
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
