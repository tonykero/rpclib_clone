#include <iostream>
#include <functional>
#include <any>
#include <variant>

#include <memory>
#include <atomic>

#include <msgpack.hpp>

// https://stackoverflow.com/questions/45715219/store-functions-with-different-signatures-in-a-map/45718187
template<typename Ret>
struct AnyCallable
{
    AnyCallable() {}
    template<typename F>
    AnyCallable(F fun) : AnyCallable(std::function(fun)) {}
    template<typename... Args>
    AnyCallable(std::function<Ret(Args...)> fun) : m_any(fun) {}
    template<typename ... Args>
    Ret operator()(Args... args) 
    { 
        using f_type = std::function<Ret(Args...)>;
        return std::invoke(std::any_cast<f_type>(m_any), std::forward<Args>(args)...); 
    }
    std::any m_any;
};
 
template<>
struct AnyCallable<std::any>
{
    AnyCallable() {}
    template<typename F>
    AnyCallable(F fun) : AnyCallable(std::function<std::any>(fun)) {}
    template<typename ... Args>
    AnyCallable(const std::function<std::any(Args...)>& fun) : m_any(fun) {}
    template<typename... Args>
    std::any operator()(Args... args) 
    { 
        using f_type = std::function<std::any(Args...)>;
        return std::invoke(std::any_cast<f_type>(m_any), std::forward<Args>(args)...); 
    }
    std::any m_any;
};

template<>
struct AnyCallable<void>
{
    AnyCallable() {}
    template<typename F>
    AnyCallable(F fun) : AnyCallable(std::function(fun)) {}
    template<typename ... Args>
    AnyCallable(std::function<void(Args...)> fun) : m_any(fun) {}
    template<typename ... Args>
    void operator()(Args... args) 
    { 
        using f_type = std::function<void(Args...)>
        std::invoke(std::any_cast<f_type>(m_any), std::forward<Args>(args)...); 
    }
    std::any m_any;
};

// Will return an AnyCallable<std::any> for any functions passed
template <typename R, typename... T>
AnyCallable<std::any> create_wrapper(std::shared_ptr<std::function<R(T...)>> func) {
    std::function<R(T...)> ret_fun = [=](T... args) -> std::any{
        return std::any(std::invoke(*func, args...));
    };
    return AnyCallable<std::any>(ret_fun);
}

template <typename... T>
AnyCallable<std::any> create_wrapper(std::shared_ptr<std::function<void(T...)>> func) {
    std::function<std::any(T...)> ret_fun = [=](T... args) -> std::any{
        std::cout << typeid(func).name() << std::endl;
        std::invoke(*func, args...);
        return std::any();
    };
    return AnyCallable<std::any>(ret_fun);
}

template <typename R, typename... T>
std::shared_ptr<std::function<R(T...)>> create_fun(R(*f)(T...)) {
    using f_type = std::function<R(T...)>;
    return std::make_shared<f_type>(f);
}

template <typename R, typename... T>
AnyCallable<std::any> create_wrapper(R(*func)(T...)) {
    return create_wrapper(create_fun(func));
}

template<typename... T, std::size_t... I>
auto drop_is(std::tuple<T...> tuple, std::index_sequence<I...>) {
    return std::make_tuple(std::get<I+1>(tuple)...);
}

template<typename... T, typename Is = std::make_index_sequence<sizeof...(T)-1>>
auto drop(std::tuple<T...> tuple) {
    return drop_is(tuple, Is{});
}

template <typename R, typename... T>
AnyCallable<std::any> create_msgpack_wrapper(std::shared_ptr<std::function<R(T...)>> func) {
    std::function<std::any(msgpack::object)> ret_fun = [=](msgpack::object args) -> std::any{
        std::tuple<std::string, T...> obj_tuple;
        std::cout << "before convert" << std::endl;
        args.convert(obj_tuple);
        std::cout << "calling " << std::get<0>(obj_tuple) << " with " << args << std::endl;
        std::tuple<T...> dst_tuple = drop(obj_tuple);
        return std::any(std::apply(*func, dst_tuple));
    };
    return AnyCallable<std::any>(ret_fun);
}

template <typename... T>
AnyCallable<std::any> create_msgpack_wrapper(std::shared_ptr<std::function<void(T...)>> func) {
    std::function<std::any(msgpack::object)> ret_fun = [=](msgpack::object args) -> std::any{
        std::tuple<std::string, T...> obj_tuple;
        std::cout << "before convert" << std::endl;
        args.convert(obj_tuple);
        std::tuple<T...> dst_tuple = drop(obj_tuple);
        std::cout << "calling " << std::get<0>(obj_tuple) << " with " << args << std::endl;
        std::apply(*func, dst_tuple);
        return std::any();
    };

    return AnyCallable<std::any>(ret_fun);
}

template <typename R, typename... T>
AnyCallable<std::any> create_msgpack_wrapper(R(*func)(T...)) {
    return create_msgpack_wrapper(create_fun(func));
}

// use std::variant to store the return type
template<typename R, typename... T>
std::variant<R> get_return_type(R (*)(T...)) {
    return std::variant<R>();
}

std::ostream& operator<<(std::ostream& os, const std::monostate& ms) {
    //os << "monostate";
    return os;
}

template<typename... T>
constexpr std::variant<std::monostate> get_return_type(void (*)(T...)) {
    return std::variant<std::monostate>();
}

template<typename R, typename... T>
std::tuple<T...> get_args_tuple(R (*)(T...)) {
    return std::tuple<T...>();
}