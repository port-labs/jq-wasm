/**
 * jq WebAssembly wrapper
 * Provides a simplified interface for using jq from JavaScript/WebAssembly
 */

#include <string.h>
#include <emscripten.h>

#include "deps/jq/src/jv.h"
#include "deps/jq/src/jq.h"

// Error message buffer
static char error_buffer[4096];
static int has_error = 0;

// Custom error callback to capture errors
static void error_callback(void *data, jv msg) {
    if (jv_get_kind(msg) == JV_KIND_STRING) {
        strncpy(error_buffer, jv_string_value(msg), sizeof(error_buffer) - 1);
        error_buffer[sizeof(error_buffer) - 1] = '\0';
    } else {
        jv formatted = jq_format_error(msg);
        if (jv_get_kind(formatted) == JV_KIND_STRING) {
            strncpy(error_buffer, jv_string_value(formatted), sizeof(error_buffer) - 1);
            error_buffer[sizeof(error_buffer) - 1] = '\0';
        }
        jv_free(formatted);
        return; // msg already freed by jq_format_error
    }
    has_error = 1;
    jv_free(msg);
}

/**
 * Execute a jq filter on JSON input
 * 
 * @param input_json The JSON string to process
 * @param filter The jq filter expression
 * @param timeout_sec Timeout in seconds (0 for no timeout)
 * @return JSON string result (caller must free), or NULL on error
 */
EMSCRIPTEN_KEEPALIVE
char* jq_exec(const char* input_json, const char* filter, unsigned int timeout_sec) {
    if (!input_json || !filter) {
        strcpy(error_buffer, "Input or filter is NULL");
        has_error = 1;
        return NULL;
    }

    has_error = 0;
    error_buffer[0] = '\0';

    // Initialize jq
    jq_state *jq = jq_init();
    if (!jq) {
        strcpy(error_buffer, "Failed to initialize jq");
        has_error = 1;
        return NULL;
    }

    // Set error callback
    jq_set_error_cb(jq, error_callback, NULL);

    // Compile the filter
    if (!jq_compile(jq, filter)) {
        jq_teardown(&jq);
        if (!has_error) {
            strcpy(error_buffer, "Failed to compile jq filter");
            has_error = 1;
        }
        return NULL;
    }

    // Parse input JSON
    jv input = jv_parse(input_json);
    if (!jv_is_valid(input)) {
        jv msg = jv_invalid_get_msg(input);
        if (jv_get_kind(msg) == JV_KIND_STRING) {
            snprintf(error_buffer, sizeof(error_buffer), "Invalid JSON input: %s", jv_string_value(msg));
        } else {
            strcpy(error_buffer, "Invalid JSON input");
        }
        jv_free(msg);
        has_error = 1;
        jq_teardown(&jq);
        return NULL;
    }

    // Start execution
    jq_start(jq, input, 0);

    // Collect all results into an array
    jv results = jv_array();
    jv result;
    int result_count = 0;

    while (1) {
        result = jq_next(jq);
        
        if (!jv_is_valid(result)) {
            if (jv_invalid_has_msg(jv_copy(result))) {
                jv msg = jv_invalid_get_msg(result);
                if (jv_get_kind(msg) == JV_KIND_STRING) {
                    strncpy(error_buffer, jv_string_value(msg), sizeof(error_buffer) - 1);
                    error_buffer[sizeof(error_buffer) - 1] = '\0';
                    has_error = 1;
                }
                jv_free(msg);
                jv_free(results);
                jq_teardown(&jq);
                return NULL;
            }
            jv_free(result);
            break;
        }
        
        results = jv_array_append(results, result);
        result_count++;
    }

    // Convert results to JSON string
    char* output = NULL;
    
    if (result_count == 0) {
        // Empty result - return "null"
        output = strdup("null");
    } else if (result_count == 1) {
        // Single result - return it directly
        jv single = jv_array_get(jv_copy(results), 0);
        jv dumped = jv_dump_string(single, 0);
        if (jv_get_kind(dumped) == JV_KIND_STRING) {
            output = strdup(jv_string_value(dumped));
        }
        jv_free(dumped);
    } else {
        // Multiple results - return as array
        jv dumped = jv_dump_string(jv_copy(results), 0);
        if (jv_get_kind(dumped) == JV_KIND_STRING) {
            output = strdup(jv_string_value(dumped));
        }
        jv_free(dumped);
    }

    jv_free(results);
    jq_teardown(&jq);

    return output;
}

/**
 * Execute a jq filter and return all results as a JSON array
 * 
 * @param input_json The JSON string to process
 * @param filter The jq filter expression
 * @param timeout_sec Timeout in seconds (0 for no timeout)
 * @return JSON array string of all results (caller must free), or NULL on error
 */
EMSCRIPTEN_KEEPALIVE
char* jq_exec_all(const char* input_json, const char* filter, unsigned int timeout_sec) {
    if (!input_json || !filter) {
        strcpy(error_buffer, "Input or filter is NULL");
        has_error = 1;
        return NULL;
    }

    has_error = 0;
    error_buffer[0] = '\0';

    jq_state *jq = jq_init();
    if (!jq) {
        strcpy(error_buffer, "Failed to initialize jq");
        has_error = 1;
        return NULL;
    }

    jq_set_error_cb(jq, error_callback, NULL);

    if (!jq_compile(jq, filter)) {
        jq_teardown(&jq);
        if (!has_error) {
            strcpy(error_buffer, "Failed to compile jq filter");
            has_error = 1;
        }
        return NULL;
    }

    jv input = jv_parse(input_json);
    if (!jv_is_valid(input)) {
        jv msg = jv_invalid_get_msg(input);
        if (jv_get_kind(msg) == JV_KIND_STRING) {
            snprintf(error_buffer, sizeof(error_buffer), "Invalid JSON input: %s", jv_string_value(msg));
        } else {
            strcpy(error_buffer, "Invalid JSON input");
        }
        jv_free(msg);
        has_error = 1;
        jq_teardown(&jq);
        return NULL;
    }

    jq_start(jq, input, 0);

    jv results = jv_array();
    jv result;

    while (1) {
        result = jq_next(jq);
        
        if (!jv_is_valid(result)) {
            if (jv_invalid_has_msg(jv_copy(result))) {
                jv msg = jv_invalid_get_msg(result);
                if (jv_get_kind(msg) == JV_KIND_STRING) {
                    strncpy(error_buffer, jv_string_value(msg), sizeof(error_buffer) - 1);
                    error_buffer[sizeof(error_buffer) - 1] = '\0';
                    has_error = 1;
                }
                jv_free(msg);
                jv_free(results);
                jq_teardown(&jq);
                return NULL;
            }
            jv_free(result);
            break;
        }
        
        results = jv_array_append(results, result);
    }

    // Always return as array
    jv dumped = jv_dump_string(results, 0);
    char* output = NULL;
    if (jv_get_kind(dumped) == JV_KIND_STRING) {
        output = strdup(jv_string_value(dumped));
    }
    jv_free(dumped);

    jq_teardown(&jq);

    return output;
}

/**
 * Get the last error message
 * @return Error message string (do not free)
 */
EMSCRIPTEN_KEEPALIVE
const char* jq_get_error(void) {
    return error_buffer;
}

/**
 * Check if there was an error
 * @return 1 if error occurred, 0 otherwise
 */
EMSCRIPTEN_KEEPALIVE
int jq_has_error(void) {
    return has_error;
}

/**
 * Free memory allocated by jq_exec or jq_exec_all
 * @param ptr Pointer to free
 */
EMSCRIPTEN_KEEPALIVE
void jq_free_result(char* ptr) {
    if (ptr) {
        free(ptr);
    }
}

/**
 * Validate a jq filter without executing it
 * @param filter The jq filter expression to validate
 * @return 1 if valid, 0 if invalid
 */
EMSCRIPTEN_KEEPALIVE
int jq_validate_filter(const char* filter) {
    if (!filter) {
        strcpy(error_buffer, "Filter is NULL");
        has_error = 1;
        return 0;
    }

    has_error = 0;
    error_buffer[0] = '\0';

    jq_state *jq = jq_init();
    if (!jq) {
        strcpy(error_buffer, "Failed to initialize jq");
        has_error = 1;
        return 0;
    }

    jq_set_error_cb(jq, error_callback, NULL);

    int valid = jq_compile(jq, filter);
    
    if (!valid && !has_error) {
        strcpy(error_buffer, "Invalid jq filter syntax");
        has_error = 1;
    }

    jq_teardown(&jq);

    return valid;
}

/**
 * Validate JSON string
 * @param json The JSON string to validate
 * @return 1 if valid, 0 if invalid
 */
EMSCRIPTEN_KEEPALIVE
int jq_validate_json(const char* json) {
    if (!json) {
        strcpy(error_buffer, "JSON input is NULL");
        has_error = 1;
        return 0;
    }

    has_error = 0;
    error_buffer[0] = '\0';

    jv parsed = jv_parse(json);
    int valid = jv_is_valid(parsed);
    
    if (!valid) {
        jv msg = jv_invalid_get_msg(parsed);
        if (jv_get_kind(msg) == JV_KIND_STRING) {
            snprintf(error_buffer, sizeof(error_buffer), "Invalid JSON: %s", jv_string_value(msg));
        } else {
            strcpy(error_buffer, "Invalid JSON syntax");
        }
        jv_free(msg);
        has_error = 1;
    } else {
        jv_free(parsed);
    }

    return valid;
}

/**
 * Get jq version information
 * @return Version string (do not free)
 */
EMSCRIPTEN_KEEPALIVE
const char* jq_wasm_version(void) {
    return "jq-wasm 1.0.0";
}