#pragma once

#include <cstdint>
#include <type_traits>

namespace scaling {
  /**
   * @brief Template struct for scaling integral types.
   * 
   * @tparam T The integral type to be scaled.
   * @tparam O The scaling factor.
   */
  template <typename T, int O> struct int_scaling;

  /**
   * @brief Alias template for the scaled integral type.
   * 
   * @tparam T The integral type to be scaled.
   * @tparam O The scaling factor.
   */
  template <typename T, int O>
  using int_scaling_t = typename int_scaling<T, O>::type;

  /**
   * @brief Specialization of int_scaling for scaling factor 0.
   * 
   * @tparam T The integral type to be scaled.
   */
  template <typename T>
  requires std::is_integral_v<T>
  struct int_scaling<T, 0>  { using type = T; };

  /**
   * @brief Specializations of int_scaling for positive scaling factors.
   * 
   * @tparam T The integral type to be scaled.
   */
  template <> struct int_scaling<int8_t, 1> { using type = int16_t; };
  template <> struct int_scaling<int16_t, 1> { using type = int32_t; };
  template <> struct int_scaling<int32_t, 1> { using type = int64_t; };
  template <> struct int_scaling<int64_t, 1> { using type = int64_t; };

  template <> struct int_scaling<uint8_t, 1> { using type = uint16_t; };
  template <> struct int_scaling<uint16_t, 1> { using type = uint32_t; };
  template <> struct int_scaling<uint32_t, 1> { using type = uint64_t; };
  template <> struct int_scaling<uint64_t, 1> { using type = uint64_t; };

  /**
   * @brief Specializations of int_scaling for negative scaling factors.
   * 
   * @tparam T The integral type to be scaled.
   */
  template <> struct int_scaling<int8_t, -1> { using type = int8_t; };
  template <> struct int_scaling<int16_t, -1> { using type = int8_t; };
  template <> struct int_scaling<int32_t, -1> { using type = int16_t; };
  template <> struct int_scaling<int64_t, -1> { using type = int32_t; };

  template <> struct int_scaling<uint8_t, -1> { using type = uint8_t; };
  template <> struct int_scaling<uint16_t, -1> { using type = uint8_t; };
  template <> struct int_scaling<uint32_t, -1> { using type = uint16_t; };
  template <> struct int_scaling<uint64_t, -1> { using type = uint32_t; };

  /**
   * @brief Recursive specialization of int_scaling for positive scaling factors greater than 1.
   * 
   * @tparam T The integral type to be scaled.
   * @tparam O The scaling factor.
   */
  template <typename T, int O>
  requires std::is_integral_v<T> && (O > 1)
  struct int_scaling<T, O> {
    using type = int_scaling_t<int_scaling_t<T, O - 1>, 1>;
  };

  /**
   * @brief Recursive specialization of int_scaling for negative scaling factors less than -1.
   * 
   * @tparam T The integral type to be scaled.
   * @tparam O The scaling factor.
   */
  template <typename T, int O>
  requires std::is_integral_v<T> && (O < -1)
  struct int_scaling<T, O> {
    using type = int_scaling_t<int_scaling_t<T, O + 1>, -1>;
  };
}
