#pragma once

#include <assert.h>
#include <numeric>
#include <string>
#include <tuple>
#include <vector>
#include <unordered_map>
#include "Index-stackindex.hpp"
#include "lib/ConvenientPrint.hpp"

namespace __serialize_detail {
  using stream = std::vector<uint8_t>;
  using stream_ptr = stream::iterator;
  using stream_cptr = stream::const_iterator;
  template <size_t> struct __pos { };
}

template <class T> size_t get_size(const T& obj);

namespace __serialize_detail {
  template <class T> struct get_size_helper;

  template <class T>
  struct get_size_helper<std::vector<T>> {
    static size_t value(const std::vector<T>& obj) {
      return std::accumulate(obj.begin(), obj.end(), sizeof(size_t),
        [](size_t acc, const T& cur) { return acc + get_size(cur); });
    }
  };

  template <>
  struct get_size_helper<std::string> {
    static size_t value(const std::string& obj) {
      return sizeof(size_t) + obj.length() * sizeof(uint8_t);
    }
  };

  template <class tuple_type>
  size_t get_tuple_size(const tuple_type& obj, __pos<0>) {
    constexpr size_t idx = std::tuple_size<tuple_type>::value - 1;
    return get_size(std::get<idx>(obj));
  }

  template <class tuple_type, size_t pos>
  size_t get_tuple_size(const tuple_type& obj, __pos<pos>) {
    constexpr size_t idx = std::tuple_size<tuple_type>::value - pos - 1;
    size_t acc = get_size(std::get<idx>(obj));
    return acc + get_tuple_size(obj, __pos<pos - 1>());
  }

  template <class ...T>
  struct get_size_helper<std::tuple<T...>> {
    static size_t value(const std::tuple<T...>& obj) {
      return get_tuple_size(obj, __pos<sizeof...(T) - 1>());
    }
  };

  template <class T>
  struct get_size_helper {
    static size_t value(const T& obj) { return sizeof(T); }
  };

  template <class K, class V>
    struct get_size_helper<std::unordered_map<K, V>> {
        static size_t value(const std::unordered_map<K, V>& obj) {
            return std::accumulate(obj.begin(), obj.end(), sizeof(size_t),
                [](size_t acc, const std::pair<K, V>& cur) {
                    return acc + get_size(cur.first) + get_size(cur.second);
                });
        }
    };

    template <class T1, class T2>
    struct get_size_helper<std::pair<T1, T2>> {
        static size_t value(const std::pair<T1, T2>& obj) {
            return get_size(obj.first) + get_size(obj.second);
        }
    };
}

template <class T>
size_t get_size(const T& obj) {
  return __serialize_detail::get_size_helper<T>::value(obj);
}

namespace __serialize_detail {
  template <class T> struct serialize_helper;

  template <class T> void serializer(const T& obj, stream_ptr&);

  template <class tuple_type>
  void serialize_tuple(const tuple_type& obj, stream_ptr& res, __pos<0>) {
    constexpr size_t idx = std::tuple_size<tuple_type>::value - 1;
    serializer(std::get<idx>(obj), res);
  }

  template <class tuple_type, size_t pos>
  void serialize_tuple(const tuple_type& obj, stream_ptr& res, __pos<pos>) {
    constexpr size_t idx = std::tuple_size<tuple_type>::value - pos - 1;
    serializer(std::get<idx>(obj), res);
    serialize_tuple(obj, res, __pos<pos - 1>());
  }

  template <class... T>
  struct serialize_helper<std::tuple<T...>> {
    static void apply(const std::tuple<T...>& obj, stream_ptr& res) {
      __serialize_detail::serialize_tuple(obj, res,
        __serialize_detail::__pos<sizeof...(T) - 1>());
    }
  };

  template <>
  struct serialize_helper<std::string> {
    static void apply(const std::string& obj, stream_ptr& res) {
      serializer(obj.length(), res);
      for (const auto& cur : obj) serializer(cur, res);
    }
  };

  template <class T>
  struct serialize_helper<std::vector<T>> {
    static void apply(const std::vector<T>& obj, stream_ptr& res) {
      serializer(obj.size(), res);
      for (const auto& cur : obj) serializer(cur, res);
    }
  };

  template <class T>
  struct serialize_helper {
    static void apply(const T& obj, stream_ptr& res) {
      const uint8_t* ptr = reinterpret_cast<const uint8_t*>(&obj);
      copy(ptr, ptr + sizeof(T), res);
      res += sizeof(T);
    }
  };

  template <class T>
  void serializer(const T& obj, stream_ptr& res) {
    serialize_helper<T>::apply(obj, res);
  }

  template <class K, class V>
    struct serialize_helper<std::unordered_map<K, V>> {
        static void apply(const std::unordered_map<K, V>& obj, stream_ptr& res) {
            serializer(obj.size(), res);
            for (const auto& cur : obj) {
                serializer(cur.first, res);
                serializer(cur.second, res);
            }
        }
    };

    template <class T1, class T2>
    struct serialize_helper<std::pair<T1, T2>> {
        static void apply(const std::pair<T1, T2>& obj, stream_ptr& res) {
            serializer(obj.first, res);
            serializer(obj.second, res);
        }
    };
}

template <class T>
void serialize(const T& obj, __serialize_detail::stream& res) {
  size_t offset = res.size();
  size_t size = get_size(obj);
  res.resize(res.size() + size);
  __serialize_detail::stream_ptr it = res.begin() + offset;
  __serialize_detail::serializer(obj, it);
  assert(res.begin() + offset + size == it);
}

namespace __serialize_detail {
  template <class T> struct deserialize_helper;

  template <class T>
  struct deserialize_helper {
    static T apply(stream_cptr& begin, stream_cptr end) {
      assert(begin + sizeof(T) <= end);
      T val;
      uint8_t* ptr = reinterpret_cast<uint8_t*>(&val);
      copy(begin, begin + sizeof(T), ptr);
      begin += sizeof(T);
      return val;
    }
  };

  template <class T>
  struct deserialize_helper<std::vector<T>> {
    static std::vector<T> apply(stream_cptr& begin, stream_cptr end) {
      size_t size = deserialize_helper<size_t>::apply(begin, end);
      std::vector<T> vect(size);
      for (size_t i = 0; i < size; ++i)
        vect[i] = std::move(deserialize_helper<T>::apply(begin, end));
      return vect;
    }
  };

  template <>
  struct deserialize_helper<std::string> {
    static std::string apply(stream_cptr& begin, stream_cptr end) {
      size_t size = deserialize_helper<size_t>::apply(begin, end);
      if (size == 0U) return std::string();
      std::string str(size, '\0');
      for (size_t i = 0; i < size; ++i) {
        str.at(i) = deserialize_helper<uint8_t>::apply(begin, end);
      }
      return str;
    }
  };

  template <class tuple_type>
  void deserialize_tuple(tuple_type& obj,
      stream_cptr& begin, stream_cptr end, __pos<0>) {
    constexpr size_t idx = std::tuple_size<tuple_type>::value - 1;
    typedef typename std::tuple_element<idx, tuple_type>::type T;
    std::get<idx>(obj) = std::move(deserialize_helper<T>::apply(begin, end));
  }

  template <class tuple_type, size_t pos>
  void deserialize_tuple(tuple_type& obj,
      stream_cptr& begin, stream_cptr end, __pos<pos>) {
    constexpr size_t idx = std::tuple_size<tuple_type>::value - pos - 1;
    typedef typename std::tuple_element<idx, tuple_type>::type T;
    std::get<idx>(obj) = std::move(deserialize_helper<T>::apply(begin, end));
    deserialize_tuple(obj, begin, end, __pos<pos - 1>());
  }

  template <class... T>
  struct deserialize_helper<std::tuple<T...>> {
    static std::tuple<T...> apply(stream_cptr& begin, stream_cptr end) {
      std::tuple<T...> ret;
      deserialize_tuple(ret, begin, end, __pos<sizeof...(T) - 1>());
      return ret;
    }
  };

  template <class T1, class T2>
    struct deserialize_helper<std::pair<T1, T2>> {
        static std::pair<T1, T2> apply(stream_cptr& begin, stream_cptr end) {
            T1 first = deserialize_helper<T1>::apply(begin, end);
            T2 second = deserialize_helper<T2>::apply(begin, end);
            return std::make_pair(first, second);
        }
    };

    template <class K, class V>
    struct deserialize_helper<std::unordered_map<K, V>> {
        static std::unordered_map<K, V> apply(stream_cptr& begin, stream_cptr end) {
            printf("deserialize_helper<std::unordered_map<K, V>>\n");
            size_t size = deserialize_helper<size_t>::apply(begin, end);
            printf("size: %zu\n", size);
            std::unordered_map<K, V> map;
            for (size_t i = 0; i < size; ++i) {
              printf("i: %zu\n", i);
                K key = deserialize_helper<K>::apply(begin, end);
                print(key);
                V val = deserialize_helper<V>::apply(begin, end);
                print(val);
                map[key] = val;
            }
            return map;
        }
    };
}

template <class T>
T deserialize(
    __serialize_detail::stream_cptr& begin,
    __serialize_detail::stream_cptr end) {
  return __serialize_detail::deserialize_helper<T>::apply(begin, end);
}

template <class T>
T deserialize(const __serialize_detail::stream& res) {
  __serialize_detail::stream_cptr it = res.begin();
  return deserialize<T>(it, res.end());
}

// Serializer for StackIndex::Index
namespace __serialize_detail {
  template <>
  struct serialize_helper<StackIndex::Stack> {
      static void apply(const StackIndex::Stack& obj, stream_ptr& res) {
          printf("serialize_helper<StackIndex::Stack>\n");
          print(1);
          // print(obj.column);
          serializer(obj.top, res);
          print(2);
          serializer(obj.column, res);
          printf("serialize_helper<StackIndex::Stack> end\n");
      }
  };

  template <>
  struct serialize_helper<StackIndex::StackTree> {
      static void apply(const StackIndex::StackTree& obj, stream_ptr& res) {
          printf("serialize_helper<StackIndex::StackTree>\n");
          serializer(obj._stacktree, res);
          serializer(obj.next, res);
          serializer(obj.root, res);
          serializer(obj.vol, res);
          serializer(obj.aux_traverse, res);
          serializer(obj.aux_last, res);
      }
  };

  template <>
  struct serialize_helper<StackIndex::Index> {
      static void apply(const StackIndex::Index& obj, stream_ptr& res) {
        printf("serialize_helper<StackIndex::Index>\n");
          serializer(obj._index, res);
      }
  };

  template <>
  struct deserialize_helper<StackIndex::Stack> {
      static StackIndex::Stack apply(stream_cptr& begin, stream_cptr end) {
          StackIndex::Stack obj;
          obj.top = deserialize_helper<size_t>::apply(begin, end);
          obj.column = deserialize_helper<std::vector<node_id>>::apply(begin, end);
          return obj;
      }
  };

  template <>
  struct deserialize_helper<StackIndex::StackTree> {
      static StackIndex::StackTree apply(stream_cptr& begin, stream_cptr end) {
          StackIndex::StackTree obj;
          obj._stacktree = deserialize_helper<std::vector<StackIndex::Stack>>::apply(begin, end);
          obj.next = deserialize_helper<std::vector<node_id>>::apply(begin, end);
          obj.root = deserialize_helper<std::vector<node_id>>::apply(begin, end);
          obj.vol = deserialize_helper<std::vector<double>>::apply(begin, end);
          obj.aux_traverse = deserialize_helper<std::vector<node_id>>::apply(begin, end);
          obj.aux_last = deserialize_helper<std::vector<node_id>>::apply(begin, end);
          return obj;
      }
  };

  template <>
  struct deserialize_helper<StackIndex::Index> {
      static StackIndex::Index apply(stream_cptr& begin, stream_cptr end) {
          StackIndex::Index obj;
          obj._index = deserialize_helper<std::vector<StackIndex::StackTree>>::apply(begin, end);
          return obj;
      }
  };

}
