#include "RestClient.h"

int main() {
       net::io_context ioc;
       ssl::context ctx{ssl::context::tls_client};
       ctx.set_default_verify_paths();

       RestClient client(
           ioc.get_executor(),
           ctx,
           "api.ejemplo.com");

       net::co_spawn(
           ioc,
           [&]() -> net::awaitable<void> {
               auto res = co_await client.async_get(
                   "/v1/items",
                   "TU_TOKEN");

               std::cout << res.result_int() << "\n";
               std::cout << res.body() << "\n";

               auto post_res = co_await client.async_post_json(
                   "/v1/items",
                   R"({"name":"demo"})",
                   "TU_TOKEN");

               std::cout << post_res.result_int() << "\n";
               std::cout << post_res.body() << "\n";
           },
           [](std::exception_ptr ep) {
               if (!ep) return;
               try {
                   std::rethrow_exception(ep);
               } catch (const std::exception& ex) {
                   std::cerr << "Error: " << ex.what() << "\n";
               }
           });

       ioc.run();
   }