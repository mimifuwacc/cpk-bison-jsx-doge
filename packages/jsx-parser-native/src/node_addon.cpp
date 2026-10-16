#include "jsx/ast.hpp"

#include <chrono>
#include <iostream>
#include <node_api.h>
#include <string>

namespace {

void check(napi_env env, napi_status status, const char* operation) {
  if (status == napi_ok)
    return;
  const napi_extended_error_info* info = nullptr;
  napi_get_last_error_info(env, &info);
  const char* detail =
      info != nullptr && info->error_message != nullptr ? info->error_message : operation;
  napi_throw_error(env, nullptr, detail);
}

napi_value parsePrefix(napi_env env, napi_callback_info callbackInfo) {
  std::size_t argumentCount = 1;
  napi_value arguments[1];
  check(env, napi_get_cb_info(env, callbackInfo, &argumentCount, arguments, nullptr, nullptr),
        "cannot read parsePrefix arguments");
  if (argumentCount != 1) {
    napi_throw_type_error(env, nullptr, "parsePrefix expects one source string");
    return nullptr;
  }

  napi_valuetype type;
  check(env, napi_typeof(env, arguments[0], &type), "cannot inspect source");
  if (type != napi_string) {
    napi_throw_type_error(env, nullptr, "parsePrefix source must be a string");
    return nullptr;
  }

  std::size_t byteLength = 0;
  check(env, napi_get_value_string_utf8(env, arguments[0], nullptr, 0, &byteLength),
        "cannot measure source");
  std::string source(byteLength + 1, '\0');
  std::size_t copied = 0;
  check(env, napi_get_value_string_utf8(env, arguments[0], source.data(), source.size(), &copied),
        "cannot copy source");
  source.resize(copied);

  const auto startedAt = std::chrono::steady_clock::now();
  std::cerr << "[Bison JSX] parse:start bytes=" << source.size() << '\n';
  const auto result = jsx::parsePrefix(std::move(source));
  const auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(
      std::chrono::steady_clock::now() - startedAt);
  if (!result) {
    std::cerr << "[Bison JSX] parse:error duration_us=" << elapsed.count() << " message=\""
              << *result.error << "\"\n";
    node_api_throw_syntax_error(env, "BISON_JSX_PARSE_ERROR", result.error->c_str());
    return nullptr;
  }
  std::cerr << "[Bison JSX] parse:success consumed=" << result.consumed
            << " duration_us=" << elapsed.count() << '\n';

  const std::string json = jsx::toJson(*result.root, false);
  napi_value output;
  check(env, napi_create_string_utf8(env, json.data(), json.size(), &output),
        "cannot create parser result");
  return output;
}

napi_value initialize(napi_env env, napi_value exports) {
  napi_value function;
  check(env,
        napi_create_function(env, "parsePrefix", NAPI_AUTO_LENGTH, parsePrefix, nullptr, &function),
        "cannot create parsePrefix function");
  check(env, napi_set_named_property(env, exports, "parsePrefix", function),
        "cannot export parsePrefix function");
  return exports;
}

} // namespace

NAPI_MODULE(NODE_GYP_MODULE_NAME, initialize)
