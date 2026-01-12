declare module "@port-labs/jq-wasm" {
  export type jq_exec = (input: string, filter: string) => number;
  export type jq_get_error = () => string;
  export type jq_free_result = (jqExecHandle: number) => void;
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
  export function createJqModule(): Promise<JqModule>;
}
