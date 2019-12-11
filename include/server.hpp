#pragma once

#include <string>
#include <unordered_map>
#include <functional>
#include <sstream>
#include <memory>

#include <wrapper_type.hpp>

#include <msgpack.hpp>
#include <uvw.hpp>

class Server {
public:
    Server(const std::string& ip, unsigned int port)
    : m_loop(uvw::Loop::getDefault()), m_tcp_handle(m_loop->resource<uvw::TCPHandle>())
    {
        m_tcp_handle->bind(ip, port);

        m_tcp_handle->once<uvw::ListenEvent>([&](const uvw::ListenEvent &, uvw::TCPHandle &srv) {
            std::shared_ptr<uvw::TCPHandle> client = srv.loop().resource<uvw::TCPHandle>();

            client->on<uvw::ErrorEvent>([](const uvw::ErrorEvent &ev, uvw::TCPHandle &) { std::cout << ev.what() << std::endl;/* handle errors */ });
            client->on<uvw::CloseEvent>([&](const uvw::CloseEvent &, uvw::TCPHandle &) { srv.close(); std::cout << "server close" << std::endl; });
            client->on<uvw::EndEvent>([](const uvw::EndEvent &, uvw::TCPHandle &client) { client.close(); std::cout << "server end" << std::endl; });

            client->on<uvw::DataEvent>([&](const uvw::DataEvent &event, uvw::TCPHandle &) {
                std::cout << "server data received" << std::endl;

                std::size_t off = 0;
                while(off != event.length) {
                    msgpack::object_handle result;
                    msgpack::unpack(result, event.data.get(), event.length, off);
                    msgpack::object obj = result.get();
                    std::cout << obj << std::endl;

                    struct name_visitor {
                        name_visitor(std::ostream& os): m_visitor(os) { }
                        bool visit_nil()                        { return true;}
                        bool visit_boolean(bool)                { return true; }
                        bool visit_positive_integer(uint64_t)   { return true; }
                        bool visit_negative_integer(int64_t)    { return true; }
                        bool visit_float32(float)               { return true; }
                        bool visit_float64(double)              { return true; }
                        bool visit_str(const char* v, uint32_t s) {
                            if(item_count == 1) {
                                    return m_visitor.visit_str(v, s);
                            } else {
                                return true;
                            }
                        }
                        bool visit_bin(const char*, uint32_t)   { return true; }
                        bool visit_ext(const char*, uint32_t)   { return true; }
                        bool start_array(uint32_t)              { return true; }
                        bool start_array_item() { item_count++;   return true; }
                        bool end_array_item()                   { return true; }
                        bool end_array()                        { return true; }
                        bool start_map(uint32_t)                { return true; }
                        bool start_map_key()                    { return true; }
                        bool end_map_key()                      { return true; }
                        bool start_map_value()                  { return true; }
                        bool end_map_value()                    { return true; }
                        bool end_map()                          { return true; }

                        msgpack::object_stringize_visitor m_visitor;
                        unsigned int item_count = 0;
                    };
                    
                    std::stringstream ss("");
                    name_visitor visitor(ss);
                    msgpack::object_parser(obj).parse(visitor);
                    std::string func_name = ss.str();

                    std::size_t s = func_name.size();
                    func_name = func_name.substr(1, s-2);
                    auto it = m_functions.find(func_name);
                    if(it != m_functions.end()) {
                        std::cout << "calling " << func_name << std::endl;
                        AnyCallable<std::any>& func_ptr = m_functions[func_name];
                        func_ptr(obj);
                    } else {
                        std::cout << "not found " << func_name << std::endl;
                    }
                    
                }
            });

            srv.accept(*client);
            client->read();
        });
        m_tcp_handle->listen();
    }

    Server(unsigned int port)
    : Server("0.0.0.0", port)
    {

    }

    ~Server() {
        this->close();
    }

    template<typename R, typename... T>
    void bind(const std::string& name, R(*fun)(T...)) {
        m_functions[name] = create_msgpack_wrapper(fun);
    }


    void run() {
        //m_tcp_handle->listen();
        m_loop->run();
    }

    void close() {
        if(!m_tcp_handle->closing()) {
            m_tcp_handle->close();
        }
        if(m_loop->alive()) {
            m_loop->stop();
            m_loop->close();
        }
        m_functions.clear();
    }


private:
    std::unordered_map<std::string, AnyCallable<std::any>> m_functions;

    std::shared_ptr<uvw::Loop>      m_loop;
    std::shared_ptr<uvw::TCPHandle> m_tcp_handle;
};