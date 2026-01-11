# jq WebAssembly Build Configuration
# Requires: Emscripten SDK (emcc)

# Directories
JQ_DIR = deps/jq
JQ_SRC = $(JQ_DIR)/src
OUT_DIR = dist


# autoconf, make, libtool, automake
scriptdir=$(cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd)

# Default target
.PHONY: pre
pre: $(OUT_DIR) $(WASM_OUTPUT)

# Create output directory
$(OUT_DIR):
	mkdir -p $(OUT_DIR)

# Build WASM module (CommonJS style)
$(WASM_OUTPUT): $(ALL_SOURCES) | $(OUT_DIR)
	$(CC) $(CFLAGS) $(EMFLAGS) \
		-s ENVIRONMENT='web,webview,worker,node' \
		$(ALL_SOURCES) \
		-o $@

build_deps:
	cd ./deps/jq && git submodule update --init
	cd ./deps/jq && autoreconf -i
	cd ./deps/jq && ./configure --with-oniguruma=builtin
	cd ./deps/jq && make src/builtin.inc
	echo "finished building jq"

# Source files from jq library (excluding main.c and jq_test.c)
JQ_SOURCES = \
	$(JQ_SRC)/builtin.c \
	$(JQ_SRC)/bytecode.c \
	$(JQ_SRC)/compile.c \
	$(JQ_SRC)/execute.c \
	$(JQ_SRC)/jv.c \
	$(JQ_SRC)/jv_alloc.c \
	$(JQ_SRC)/jv_aux.c \
	$(JQ_SRC)/jv_dtoa.c \
	$(JQ_SRC)/jv_dtoa_tsd.c \
	$(JQ_SRC)/jv_file.c \
	$(JQ_SRC)/jv_parse.c \
	$(JQ_SRC)/jv_print.c \
	$(JQ_SRC)/jv_unicode.c \
	$(JQ_SRC)/lexer.c \
	$(JQ_SRC)/linker.c \
	$(JQ_SRC)/locfile.c \
	$(JQ_SRC)/parser.c \
	$(JQ_SRC)/util.c \
	$(JQ_DIR)/vendor/decNumber/decContext.c \
	$(JQ_DIR)/vendor/decNumber/decNumber.c

# Wrapper source
WRAPPER_SOURCE = jq_wasm.c

# All sources
ALL_SOURCES = $(WRAPPER_SOURCE) $(JQ_SOURCES)

# Output files
WASM_OUTPUT = $(OUT_DIR)/jq.js
WASM_ES6_OUTPUT = $(OUT_DIR)/jq.mjs

# Compiler
CC = emcc

# Include paths
# Note: -I$(JQ_DIR) is needed because builtin.c includes "src/builtin.inc"
INCLUDES = -I$(JQ_SRC) -I$(JQ_SRC)/decNumber -I$(JQ_DIR)

# Compiler flags
CFLAGS = -O2 \
	-DHAVE_DECNUM \
	-DHAVE_MEMMEM \
	-D_GNU_SOURCE \
	-DIEEE_8087 \
	$(INCLUDES)

# TODO: Look at these
# Emscripten-specific flags
EMFLAGS = \
	-s WASM=1 \
	-s MODULARIZE=1 \
	-s EXPORT_NAME="createJqModule" \
	-s EXPORTED_FUNCTIONS='["_jq_exec", "_jq_exec_all", "_jq_get_error", "_jq_has_error", "_jq_free_result", "_jq_validate_filter", "_jq_validate_json", "_jq_wasm_version", "_malloc", "_free"]' \
	-s EXPORTED_RUNTIME_METHODS='["ccall", "cwrap", "UTF8ToString", "stringToUTF8", "lengthBytesUTF8"]' \
	-s ALLOW_MEMORY_GROWTH=1 \
	-s INITIAL_MEMORY=16777216 \
	-s STACK_SIZE=1048576 \
	-s NO_EXIT_RUNTIME=1 \
	-s FILESYSTEM=0 \
	-s ASSERTIONS=0 \
	-s SINGLE_FILE=0

# Debug flags (use with make DEBUG=1)
ifdef DEBUG
	CFLAGS += -g -O0
	EMFLAGS += -s ASSERTIONS=2 -s SAFE_HEAP=1 -s STACK_OVERFLOW_CHECK=2
else
	CFLAGS += -O2 -DNDEBUG
endif

# Build for browser only (smaller output)
.PHONY: browser
browser: $(OUT_DIR)
	$(CC) $(CFLAGS) $(EMFLAGS) \
		-s ENVIRONMENT='web,webview,worker' \
		$(ALL_SOURCES) \
		-o $(OUT_DIR)/index.js

# Clean build artifacts
.PHONY: clean
clean:
	rm -rf $(OUT_DIR)

# Test build
.PHONY: test
test: all
	@echo "Testing WASM build..."
	@node -e " \
		const createJqModule = require('./$(OUT_DIR)/jq.js'); \
		createJqModule().then(Module => { \
			const exec = Module.cwrap('jq_exec', 'number', ['string', 'string', 'number']); \
			const getError = Module.cwrap('jq_get_error', 'string', []); \
			const freeResult = Module.cwrap('jq_free_result', null, ['number']); \
			const ptr = exec('{\"a\": 1}', '.a', 10); \
			if (ptr) { \
				console.log('Result:', Module.UTF8ToString(ptr)); \
				freeResult(ptr); \
			} else { \
				console.log('Error:', getError()); \
			} \
		}); \
	"