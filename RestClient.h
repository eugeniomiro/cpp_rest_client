#include <boost/asio/awaitable.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/redirect_error.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/ssl.hpp>
#include <openssl/err.h>
#include <openssl/ssl.h>

#include <chrono>
#include <iostream>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace net = boost::asio;
namespace ssl = net::ssl;
namespace beast = boost::beast;
namespace http = beast::http;
using tcp = net::ip::tcp;

class RestClient
{
public:
    using Headers = std::vector<std::pair<std::string, std::string>>;

    RestClient(net::any_io_executor executor, ssl::context &ssl_ctx, std::string host, std::string port, std::string user_agent)
        : executor_(std::move(executor)), ssl_ctx_(ssl_ctx), host_(std::move(host)), port_(std::move(port)), user_agent_(std::move(user_agent)) {}

    net::awaitable<http::response<http::string_body>> async_get(
        std::string target,
        std::optional<std::string> bearer_token = std::nullopt,
        Headers headers = {})
    {
        co_return co_await async_request(
            http::verb::get,
            std::move(target),
            std::move(bearer_token),
            "",
            std::move(headers));
    }

    net::awaitable<http::response<http::string_body>> async_post_json(
        std::string target,
        std::string json_body,
        std::optional<std::string> bearer_token = std::nullopt,
        Headers headers = {})
    {
        headers.emplace_back("Content-Type", "application/json");
        co_return co_await async_request(
            http::verb::post,
            std::move(target),
            std::move(bearer_token),
            std::move(json_body),
            std::move(headers));
    }

    net::awaitable<http::response<http::string_body>> async_request(
        http::verb method,
        std::string target,
        std::optional<std::string> bearer_token = std::nullopt,
        std::string body = "",
        Headers headers = {})
    {
        tcp::resolver resolver(executor_);
        beast::ssl_stream<beast::tcp_stream> stream(executor_, ssl_ctx_);

        if (!SSL_set_tlsext_host_name(stream.native_handle(), host_.c_str()))
        {
            throw beast::system_error(
                beast::error_code(
                    static_cast<int>(::ERR_get_error()),
                    net::error::get_ssl_category()));
        }

        beast::get_lowest_layer(stream).expires_after(std::chrono::seconds(30));

        auto endpoints = co_await resolver.async_resolve(
            host_, port_, net::use_awaitable);

        co_await beast::get_lowest_layer(stream).async_connect(
            endpoints, net::use_awaitable);

        co_await stream.async_handshake(
            ssl::stream_base::client, net::use_awaitable);

        http::request<http::string_body> req{method, std::move(target), 11};
        req.set(http::field::host, host_);
        req.set(http::field::user_agent, user_agent_);
        req.set(http::field::accept, "application/json");

        if (bearer_token)
        {
            req.set(http::field::authorization, "Bearer " + *bearer_token);
        }

        for (const auto &[name, value] : headers)
        {
            req.set(name, value);
        }

        req.body() = std::move(body);
        if (!req.body().empty())
        {
            req.prepare_payload();
        }

        co_await http::async_write(stream, req, net::use_awaitable);

        beast::flat_buffer buffer;
        http::response<http::string_body> res;
        co_await http::async_read(stream, buffer, res, net::use_awaitable);

        beast::error_code ec;
        co_await stream.async_shutdown(net::redirect_error(net::use_awaitable, ec));
        if (ec && ec != net::error::eof)
        {
            throw beast::system_error(ec);
        }

        co_return res;
    }

private:
    net::any_io_executor executor_;
    ssl::context &ssl_ctx_;
    std::string host_;
    std::string port_;
    std::string user_agent_;
};