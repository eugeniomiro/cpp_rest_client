#include <boost/program_options.hpp>

#include "RestClient.h"

namespace po = boost::program_options;

int main(int argc, char* argv[])
{
    po::options_description desc("Allowed options");
    // clang-format off
    desc.add_options()
        ("help,h", "Show help message")
        ("host", po::value<std::string>()->default_value("api.ejemplo.com"), "API host")
        ("port", po::value<std::string>()->default_value("443"), "API port")
        ("body", po::value<std::string>()->default_value(R"({"name":"demo"})"), "body for POST request")
        ("user-agent", po::value<std::string>()->default_value("rest-client-beast/1.0"), "User-Agent header")
        ("bearer-token", po::value<std::string>(), "Bearer token for authentication");
    // clang-format on

    po::variables_map vm;
    try
    {
        po::store(po::parse_command_line(argc, argv, desc), vm);
        po::notify(vm);
    }
    catch (const po::error& e)
    {
        std::cerr << "Error parsing command line: " << e.what() << "\n";
        std::cerr << desc << "\n";
        return 1;
    }

    if (vm.count("help"))
    {
        std::cout << desc << "\n";
        return 0;
    }
    net::io_context ioc;
    ssl::context    ctx{ssl::context::tls_client};
    
    ctx.set_default_verify_paths();

    RestClient client(ioc.get_executor(), ctx, vm["host"].as<std::string>(), vm["port"].as<std::string>(), vm["user-agent"].as<std::string>());

    net::co_spawn(
        ioc,
        [&]() -> net::awaitable<void>
        {
            auto res = co_await client.async_get("/v1/items", vm["bearer-token"].as<std::string>());

            std::cout << res.result_int() << "\n";
            std::cout << res.body() << "\n";

            auto post_res = co_await client.async_post_json("/v1/items", vm["body"].as<std::string>(), vm["bearer-token"].as<std::string>());

            std::cout << post_res.result_int() << "\n";
            std::cout << post_res.body() << "\n";
        },
        [](std::exception_ptr ep)
        {
            if (!ep)
            {
                return;
            }
            try
            {
                std::rethrow_exception(ep);
            }
            catch (const std::exception& ex)
            {
                std::cerr << "Error: " << ex.what() << "\n";
            }
        });

    ioc.run();
}