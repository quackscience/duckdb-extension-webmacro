#include <emscripten.h>

namespace duckdb {

struct WasmResultInner {
  idx_t status;
  std::string reason;
  std::string body;
};
struct WasmResult {
  operator bool() { return true; }
  WasmResultInner *operator->() { return &_inner; }
  WasmResultInner _inner;
};
struct WasmClient {
  WasmResult Get(string path) {
    WasmResult res;

    char *exe = NULL;
    exe = (char *)EM_ASM_PTR(
        {
          var url = (UTF8ToString($0));
          if (typeof XMLHttpRequest === "undefined") {
            return 0;
          }
          const xhr = new XMLHttpRequest();
          xhr.open("GET", url, false);
          xhr.responseType = "arraybuffer";
          xhr.send(null);
          if (xhr.status != 200)
            return 0;
          var uInt8Array = xhr.response;

          var len = uInt8Array.byteLength;
          var fileOnWasmHeap = _malloc(len + 4);

          var properArray = new Uint8Array(uInt8Array);

          for (var iii = 0; iii < len; iii++) {
            Module.HEAPU8[iii + fileOnWasmHeap + 4] = properArray[iii];
            }
            var LEN123 = new Uint8Array(4);
            LEN123[0] = len % 256;
            len -= LEN123[0];
            len /= 256;
            LEN123[1] = len % 256;
            len -= LEN123[1];
            len /= 256;
            LEN123[2] = len % 256;
            len -= LEN123[2];
            len /= 256;
            LEN123[3] = len % 256;
            len -= LEN123[3];
            len /= 256;
            Module.HEAPU8.set(LEN123, fileOnWasmHeap);
		console.log(properArray);
            return fileOnWasmHeap;
        },
        path.c_str());

    if (!exe) {
      res._inner.status = 404;
      res._inner.reason = "Something went quack in Wasm land!";
    } else {
      res._inner.status = 200;
      uint64_t LEN = 0;
      LEN *= 256;
      LEN += ((uint8_t *)exe)[3];
      LEN *= 256;
      LEN += ((uint8_t *)exe)[2];
      LEN *= 256;
      LEN += ((uint8_t *)exe)[1];
      LEN *= 256;
      LEN += ((uint8_t *)exe)[0];
      res._inner.body = string(exe + 4, LEN);
      free(exe);
    }

    return res;
  }
};

static std::pair<WasmClient, std::string>
SetupHttpClient(const std::string &url) {
  WasmClient x;
  return std::make_pair(std::move(x), url);
}

static void HandleHttpError(const WasmResult &res,
                            const std::string &request_type) {}

} // namespace duckdb
