declare module "@port-labs/jq-wasm" {
  /**
   * Execute a jq filter on JSON input
   *
   * @param input_json The JSON string to process
   * @param filter The jq filter expression
   * @return JSON string result (caller must free), or NULL on error
   */
  export type jq_exec = (input: string, filter: string) => number;
  /**
   * Get the last error message
   * @return Error message string (do not free)
   */
  export type jq_get_error = () => string;
  /**
   * Free memory allocated by jq_exec
   * @param ptr Pointer to free
   */
  export type jq_free_result = (jqExecHandle: number) => void;
  /**
   * Check if there was an error
   * @return 1 if error occurred, 0 otherwise
   */
  export type jq_has_error = () => number;
  /**
   * Validate a jq filter without executing it
   * @param filter The jq filter expression to validate
   * @return 1 if valid, 0 if invalid
   */
  export type jq_validate_filter = (filter: string) => number;
  /**
   * Get jq version information
   * @return Version string (do not free)
   */
  export type jq_wasm_version = () => string;
  export type JqModule = WebAssembly.Module & {
    cwrap: <
      TName extends "jq_exec" | "jq_get_error" | "jq_free_result",
      TReturn extends TName extends "jq_exec"
        ? "number"
        : TName extends "jq_get_error"
        ? "string"
        : null,
      TArgs extends TName extends "jq_exec"
        ? [string, string]
        : TName extends "jq_get_error"
        ? []
        : [number]
    >(
      functionName: TName,
      returnType: TReturn,
      args: TArgs
    ) => TName extends "jq_exec"
      ? jq_exec
      : TName extends "jq_get_error"
      ? jq_get_error
      : jq_free_result;
  };
  export default function createJqModule(options: {
    locateFile: (path: string) => string;
  }): Promise<JqModule>;
}
