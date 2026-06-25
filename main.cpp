#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <iostream>
#include <string>

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
namespace ssl = net::ssl;
using tcp = net::ip::tcp;

int main()
{
    try
    {
        const std::string host = "api.ejemplo.com";
        const std::string port = "443";
        const std::string target = "/recurso";
        const std::string token = "TU_BEARER_TOKEN";
        const int version = 11; // HTTP/1.1

        net::io_context ioc;
        ssl::context ctx{ssl::context::tls_client};
        ctx.set_default_verify_paths();

        tcp::resolver resolver{ioc};
        beast::ssl_stream<beast::tcp_stream> stream{ioc, ctx};

        if (!SSL_set_tlsext_host_name(stream.native_handle(), host.c_str()))
        {
            throw beast::system_error(
                beast::error_code(static_cast<int>(::ERR_get_error()),
                                  net::error::get_ssl_category()));
        }

        auto const results = resolver.resolve(host, port);
        beast::get_lowest_layer(stream).connect(results);

        stream.handshake(ssl::stream_base::client);

        http::request<http::empty_body> req{http::verb::get, target, version};
        req.set(http::field::host, host);
        req.set(http::field::user_agent, "mi-cliente-beast");
        req.set(http::field::authorization, "Bearer " + token);
        req.set(http::field::accept, "application/json");

        http::write(stream, req);

        beast::flat_buffer buffer;
        http::response<http::string_body> res;
        http::read(stream, buffer, res);

        std::cout << res << std::endl;

        beast::error_code ec;
        stream.shutdown(ec);
        if (ec && ec != net::error::eof)
        {
            throw beast::system_error{ec};
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}