#pragma once

#include <iostream>
#include <type_traits>
#include <vector>
#include <unordered_map>
#include <tuple>
#include <iomanip>
#include <boost/type_index.hpp>



// has_ostream_operator 结构使用 SFINAE 技术，尝试调用 operator<< 操作符，并检查是否可以实例化。如果可以实例化，则返回 std::true_type，否则返回 std::false_type。用has_ostream_operator<int>::value来获取bool值。
template <typename T>
struct has_ostream_operator
{
    template <typename U>
    static auto test(int) -> decltype(std::declval<std::ostream&>() << std::declval<U>(), std::true_type{});

    template <typename>
    static std::false_type test(...);

    static const bool value = std::is_same<decltype(test<T>(0)), std::true_type>::value;
};

template<typename T>
std::ostream& operator<<(std::ostream& os, std::vector<T> vec){
    // 仅在参数类型运行时才确定时会使用到，否则会在编译时直接报错
    if(!has_ostream_operator<T>::value){
        std::string errmsg = "Type " + boost::typeindex::type_id_with_cvr<T>().pretty_name() + " does't define ostream_operator!";
        throw std::runtime_error(errmsg);
    }

    os << "[";
    for(auto it=vec.begin();it!=vec.end();++it){
        os << *it;
        if(it!=vec.end()-1){
            os << ", ";
        }
    }
    os << "]";
    return os;
}

template<std::size_t I = 0, typename... Types>
void print_tuple(std::ostream& os, const std::tuple<Types...>& t) {
    if constexpr (I < sizeof...(Types)) {
        if constexpr (I != 0)
            os << ", ";
        os << std::get<I>(t);
        print_tuple<I + 1>(os, t);
    }
}
template<typename... Types>
std::ostream& operator<<(std::ostream& os, const std::tuple<Types...>& t) {
    os << "(";
    print_tuple(os, t);
    os << ")";
    return os;
}


template<typename T1, typename T2>
std::ostream& operator<<(std::ostream& os, std::pair<T1,T2> p){
    // 仅在参数类型运行时才确定时会使用到，否则会在编译时直接报错
    if(!has_ostream_operator<T1>::value){
        std::string errmsg = "Type " + boost::typeindex::type_id_with_cvr<T1>().pretty_name() + " does't define ostream_operator!";
        throw std::runtime_error(errmsg);
    }
    if(!has_ostream_operator<T2>::value){
        std::string errmsg = "Type " + boost::typeindex::type_id_with_cvr<T2>().pretty_name() + " does't define ostream_operator!";
        throw std::runtime_error(errmsg);
    }

    os << "(" << p.first << ", " << p.second << ")";
    return os;
}


template<typename T1, typename T2>
std::ostream& operator<<(std::ostream& os,const std::unordered_map<T1,T2> m){
    // 仅在参数类型运行时才确定时会使用到，否则会在编译时直接报错
    if(!has_ostream_operator<T1>::value){
        std::string errmsg = "Type " + boost::typeindex::type_id_with_cvr<T1>().pretty_name() + " does't define ostream_operator!";
        throw std::runtime_error(errmsg);
    }
    if(!has_ostream_operator<T2>::value){
        std::string errmsg = "Type " + boost::typeindex::type_id_with_cvr<T2>().pretty_name() + " does't define ostream_operator!";
        throw std::runtime_error(errmsg);
    }

    os << "{";
    for(auto it=m.begin();it!=m.end();++it){
        os << (*it).first << ":" << (*it).second;
        if (std::next(it) != m.end()) {
            std::cout << ", ";
        }
    }
    os << "}";
    return os;
}

// std::ostream& operator<<(std::ostream& os,const bool boolval){
//     os << (boolval ? "true":"false");
//     return os;
// }

template<typename... Args>
void print(Args... args) {
    auto print_arg = [](auto arg) {
        std::cout << std::setprecision(16) << arg << " ";
    };

    (void)std::initializer_list<int>{(print_arg(args), 0)...};
    std::cout << std::endl;
}


