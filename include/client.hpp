#pragma once

#include <string>
#include <memory>
#include <queue>
#include <mutex>
#include <thread>

#include <uvw.hpp>
#include <msgpack.hpp>

#include <iostream>

class Client{
public:
    Client(const std::string& ip, unsigned int port)
    : m_loop(uvw::Loop::getDefault()), m_tcp_handle(m_loop->resource<uvw::TCPHandle>())
    {
        m_tcp_handle->on<uvw::ErrorEvent>([](const uvw::ErrorEvent &ev, uvw::TCPHandle &) { std::cout << ev.what() << std::endl; });
        m_tcp_handle->once<uvw::CloseEvent>([](const uvw::CloseEvent &, uvw::TCPHandle &) { std::cout << "client closed" << std::endl; });

        m_tcp_handle->once<uvw::ConnectEvent>([&](const uvw::ConnectEvent &, uvw::TCPHandle &tcp) {
        std::cout << "client connected" << std::endl;

        auto worker = m_loop->resource<uvw::WorkReq>([&](){
           while(!m_end_worker || !m_queue.empty()) {
                //std::this_thread::sleep_for(std::chrono::milliseconds(500));
                //std::cout << "query queue" << std::endl;
                while(!m_queue.empty() && tcp.writable()) {
                    std::cout << "writing from queue" << std::endl;
                    mutex.lock();
                    tcp.write(m_queue.front()->data(), m_queue.front()->size());
                    m_queue.pop();
                    mutex.unlock();
                }
            }
        });

        worker->on<uvw::WorkEvent>([&](const auto&, auto&){
           std::cout << "queue worker ended" << std::endl;
           tcp.close();
        });

        worker->queue();
        });

        m_tcp_handle->connect(ip, port);

        m_loop_thread = std::make_unique<std::thread>([&](){
            m_loop->run();
        });
    }
    ~Client() {
        this->close();
    }

    void close() {
        m_end_worker = true;
        if(m_loop->alive()) {
            m_loop->stop();
            m_loop->close();
        }
        if(m_loop_thread->joinable()) {
            m_loop_thread->join();
        }
    }

    template<typename... T>
    msgpack::object call(const std::string& name, T... args) {
        auto buf = std::make_unique<msgpack::sbuffer>();
        std::tuple<std::string, T...> msg(name, args...);
        msgpack::pack(buf.get(), msg);
        mutex.lock();
        m_queue.push(std::move(buf));
        mutex.unlock();
        return msgpack::object();
    }

private:
    std::unique_ptr<std::thread>    m_loop_thread;
    std::shared_ptr<uvw::Loop>      m_loop;
    std::shared_ptr<uvw::TCPHandle> m_tcp_handle;

    std::queue<std::unique_ptr<msgpack::sbuffer>>    m_queue;
    std::mutex mutex;

    bool m_end_worker = false;
};