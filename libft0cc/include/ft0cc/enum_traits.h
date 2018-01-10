#pragma once

#include <type_traits>
#include <limits>

// The default enumeration category. Conversion is equivalent to static_cast.
// Unspecialized enumeration traits use this category.
struct enum_default { };

// The standard-layout enumeration category. Values outside the given range are
// mapped to a given none-element. Requires that:
//  - If both EnumT::min and EnumT::max are given, then EnumT::min <= EnumT::max
//  - EnumT::none must be given, and !(EnumT::min <= EnumT::none <= EnumT::max)
// EnumT::min and EnumT::max default to the minimum / maximum representable
// value of the underlying type if not given.
// Linear enum class types additionally support operator!.
struct enum_standard { };

// The linear enumeration category. Values not equal to the given none-element
// are clipped to the given range. Requires that:
//  - If both EnumT::min and EnumT::max are given, then EnumT::min <= EnumT::max
//  - EnumT::none must be given, and !(EnumT::min <= EnumT::none <= EnumT::max)
// EnumT::min and EnumT::max default to the minimum / maximum representable
// value of the underlying type respectively if not given.
// Linear enum class types additionally support !, ++, and --.
struct enum_linear { };

// The bitmask enumeration category. Bits common to the given range limits are
// kept constant. Requires that:
//  - If both EnumT::min and EnumT::max are given, then all of the set bits of
//    EnumT::min must also be set in EnumT::max
//  - The underlying type of EnumT must be unsigned
// EnumT::min and EnumT::max default to the minimum / maximum representable
// value of the underlying type respectively if not given.
// Bitmask enum class types additionally support &, &=, |, |=, ^, ^=, and ~.
struct enum_bitmask { };

// The discrete enumeration category. Values not equal to any of the given
// template parameters are mapped to the given none-element.
// zero. Requires that:
//  - EnumT::none must be given, and Vals... must not contain EnumT::none
// Discrete enum class types additionally support operator!.
template <typename EnumT, EnumT... Vals>
struct enum_discrete { };

// The enumeration traits class template. Contains only one member typedef:
//  - category: The enumeration category to use for this enumeration type
// User-defined enumeration types may specialize this template.
template <typename EnumT>
struct enum_traits {
	using category = enum_default;
};

// The enumeration category traits class template. Contains:
//  - typedef CatT category;
//  - template <typename EnumT> static bool valid()
//     Checks during compile-time whether the given enumeration type satisfies
//     the category's constraints. (will become a concept bool later)
//  - template <typename EnumT, typename ValT> static EnumT enum_cast(ValT x)
//     Maps a given value to the enumeration type.
template <typename CatT>
struct enum_category_traits;



namespace details {

template <typename EnumT>
constexpr bool is_scoped_enum_f(std::false_type) noexcept {
	return false;
}
template <typename EnumT>
constexpr bool is_scoped_enum_f(std::true_type) noexcept {
	return !std::is_convertible_v<EnumT, std::underlying_type_t<EnumT>>;
}
template <typename EnumT>
struct is_scoped_enum : std::integral_constant<bool, is_scoped_enum_f<EnumT>(std::is_enum<EnumT>())> { };
template <typename EnumT>
inline constexpr bool is_scoped_enum_v = is_scoped_enum<EnumT>::value;

template <typename EnumT>
constexpr std::underlying_type_t<EnumT> value_cast_impl(EnumT x) noexcept {
	return static_cast<std::underlying_type_t<EnumT>>(x);
}

template <typename CatT>
struct is_enum_category_discrete : std::false_type { };
template <typename EnumT, EnumT... Vals>
struct is_enum_category_discrete<enum_discrete<EnumT, Vals...>> : std::true_type { };

template <typename CatT, typename = void>
struct is_enum_category : std::false_type { };
template <typename CatT>
struct is_enum_category<CatT, std::void_t<
	typename enum_category_traits<CatT>::category>> : std::true_type { };

template <typename EnumT, typename = void>
struct get_enum_category {
	using type = enum_default;
};
template <typename EnumT>
struct get_enum_category<EnumT, std::void_t<typename enum_traits<EnumT>::category>> {
private:
	using T2 = typename enum_traits<EnumT>::category;
public:
	using type = std::conditional_t<is_enum_category<T2>::value, T2, enum_default>;
};

} // namespace details

template <typename CatT>
using is_enum_category = details::is_enum_category<CatT>;
// Checks if a given type is an enumeration category type.
template <typename CatT>
inline constexpr bool is_enum_category_v = is_enum_category<CatT>::value;

template <typename EnumT>
using get_enum_category = details::get_enum_category<EnumT>;
// Obtains the category for a given enumeration type, defaulting to enum_default
// if a valid category cannot be found.
template <typename EnumT>
using get_enum_category_t = typename get_enum_category<EnumT>::type;



namespace details {

template <typename T>
constexpr T clamp(T x, T lo, T hi) noexcept {
	return x < lo ? lo : (x > hi ? hi : x);
}

template <typename T,
	typename = std::enable_if_t<std::is_unsigned_v<std::decay_t<T>>>>
constexpr T bitwise_clamp(T x, T lo, T hi) noexcept {
	return (x & hi) | lo;
}

template <typename EnumT, typename = void>
struct enum_has_none_member : std::false_type { };
template <typename EnumT>
struct enum_has_none_member<EnumT, std::void_t<decltype(EnumT::none)>> : std::true_type { };

template <typename EnumT, typename = void>
struct enum_has_min_member : std::false_type { };
template <typename EnumT>
struct enum_has_min_member<EnumT, std::void_t<decltype(EnumT::min)>> : std::true_type { };

template <typename EnumT, typename = void>
struct enum_has_max_member : std::false_type { };
template <typename EnumT>
struct enum_has_max_member<EnumT, std::void_t<decltype(EnumT::max)>> : std::true_type { };

} // namespace details

// Checks whether the given enumeration type has a none-element.
template <typename EnumT,
	typename = std::enable_if_t<std::is_enum_v<EnumT>>>
constexpr bool enum_has_none() noexcept {
	return details::enum_has_none_member<EnumT>::value;
}

// Obtains the none-element of a given enumeration type.
template <typename EnumT,
	typename = std::enable_if_t<std::is_enum_v<EnumT> && enum_has_none<EnumT>()>>
constexpr EnumT enum_none() noexcept {
	if constexpr (details::enum_has_none_member<EnumT>::value)
		return EnumT::none;
}

// Checks whether the given enumeration type has a minimum element.
template <typename EnumT,
	typename = std::enable_if_t<std::is_enum_v<EnumT>>>
constexpr bool enum_has_min() noexcept {
	return !details::is_enum_category_discrete<get_enum_category_t<EnumT>>::value;
}

// Obtains the minimum element of a given enumeration type.
template <typename EnumT,
	typename = std::enable_if_t<std::is_enum_v<EnumT> && enum_has_min<EnumT>()>>
constexpr EnumT enum_min() noexcept {
	if constexpr (details::enum_has_min_member<EnumT>::value)
		return EnumT::min;
	if constexpr (!details::is_enum_category_discrete<get_enum_category_t<EnumT>>::value)
		return EnumT {std::numeric_limits<std::underlying_type_t<EnumT>>::min()};
}

// Checks whether the given enumeration type has a maximum element.
template <typename EnumT,
	typename = std::enable_if_t<std::is_enum_v<EnumT>>>
constexpr bool enum_has_max() noexcept {
	return !details::is_enum_category_discrete<get_enum_category_t<EnumT>>::value;
}

// Obtains the maximum element of a given enumeration type.
template <typename EnumT,
	typename = std::enable_if_t<std::is_enum_v<EnumT> && enum_has_max<EnumT>()>>
constexpr EnumT enum_max() noexcept {
	if constexpr (details::enum_has_max_member<EnumT>::value)
		return EnumT::max;
	if constexpr (!details::is_enum_category_discrete<get_enum_category_t<EnumT>>::value)
		return EnumT {std::numeric_limits<std::underlying_type_t<EnumT>>::max()};
}



// Casts an enumeration value to its underlying type.
template <typename EnumT,
	typename CatTraits = enum_category_traits<get_enum_category_t<EnumT>>,
	typename = std::enable_if_t<std::is_enum_v<EnumT>>>
constexpr std::underlying_type_t<EnumT> value_cast(EnumT x) noexcept {
	static_assert(CatTraits::template valid<EnumT>());
	return details::value_cast_impl(x);
}

// Casts a value to the given enumeration type.
template <typename EnumT, typename ValT,
	typename CatTraits = enum_category_traits<get_enum_category_t<EnumT>>,
	typename = std::enable_if_t<std::is_enum_v<EnumT>>,
	typename = std::enable_if_t<std::is_convertible_v<ValT, std::underlying_type_t<EnumT>>>>
constexpr EnumT enum_cast(ValT x) noexcept {
	static_assert(CatTraits::template valid<EnumT>());
	return CatTraits::template enum_cast<EnumT>(x);
}

// Constrains a given value by the given enumeration type, as if by casting it
// to and from the enumeration type.
template <typename EnumT, typename ValT,
	typename = std::enable_if_t<std::is_enum_v<EnumT>>,
	typename = std::enable_if_t<std::is_same_v<ValT, std::underlying_type_t<EnumT>>>>
constexpr std::underlying_type_t<EnumT> value_cast(ValT x) noexcept {
	return value_cast(enum_cast<EnumT>(x));
}

// Constrains the given enumeration type, as if by casting it to and from its
// underlying type.
template <typename EnumT,
	typename CatTraits = enum_category_traits<get_enum_category_t<EnumT>>,
	typename = std::enable_if_t<std::is_enum_v<EnumT>>>
constexpr EnumT enum_cast(EnumT x) noexcept {
	return enum_cast<EnumT>(value_cast(x));
}



inline namespace enum_operators {

// Returns true if lhs is equal to EnumT::none. Only supports enum class types
// that have a none-element.
template <typename EnumT,
	typename = std::enable_if_t<details::is_scoped_enum_v<EnumT>>,
	typename = std::enable_if_t<enum_has_none<EnumT>()>>
constexpr bool operator!(const EnumT &lhs) noexcept {
	return lhs == enum_none<EnumT>();
}

// Pre-increments lhs if it is equal to neither the none-element nor the maximum
// element. Only supports linear enum class types.
template <typename EnumT,
	typename = std::enable_if_t<details::is_scoped_enum_v<EnumT>>,
	typename = std::enable_if_t<std::is_same_v<get_enum_category_t<EnumT>, enum_linear>>>
constexpr EnumT &operator++(EnumT &lhs) noexcept {
	if (!(!lhs || lhs == enum_max<EnumT>()))
		lhs = enum_cast<EnumT>(value_cast(lhs) + 1);
	return lhs;
}

// Post-increments lhs if it is equal to neither the none-element nor the
// maximum element. Only supports linear enum class types.
template <typename EnumT,
	typename = std::enable_if_t<details::is_scoped_enum_v<EnumT>>,
	typename = std::enable_if_t<std::is_same_v<get_enum_category_t<EnumT>, enum_linear>>>
constexpr EnumT operator++(const EnumT &lhs, int) noexcept {
	EnumT ret = lhs;
	++lhs;
	return ret;
}

// Pre-decrements lhs if it is equal to neither the none-element nor the maximum
// element. Only supports linear enum class types.
template <typename EnumT,
	typename = std::enable_if_t<details::is_scoped_enum_v<EnumT>>,
	typename = std::enable_if_t<std::is_same_v<get_enum_category_t<EnumT>, enum_linear>>>
constexpr EnumT &operator--(EnumT &lhs) noexcept {
	if (!(!lhs || lhs == enum_min<EnumT>()))
		lhs = enum_cast<EnumT>(value_cast(lhs) - 1);
	return lhs;
}

// Post-decrements lhs if it is equal to neither the none-element nor the
// maximum element. Only supports linear enum class types.
template <typename EnumT,
	typename = std::enable_if_t<details::is_scoped_enum_v<EnumT>>,
	typename = std::enable_if_t<std::is_same_v<get_enum_category_t<EnumT>, enum_linear>>>
constexpr EnumT operator--(const EnumT &lhs, int) noexcept {
	EnumT ret = lhs;
	--lhs;
	return ret;
}



// If neither operand is EnumT::None, returns the union of the two bit masks.
// Otherwise returns EnumT::None. Only supports bitmask enum class types.
template <typename EnumT,
	typename = std::enable_if_t<details::is_scoped_enum_v<EnumT>>,
	typename = std::enable_if_t<std::is_same_v<get_enum_category_t<EnumT>, enum_bitmask>>>
constexpr EnumT operator|(const EnumT &lhs, const EnumT &rhs) noexcept {
	if constexpr (enum_has_none<EnumT>())
		if (!lhs || !rhs)
			return enum_none<EnumT>();
	return enum_cast<EnumT>(value_cast(lhs) | value_cast(rhs));
}

// If neither operand is EnumT::None, assigns (lhs | rhs) to lhs. The operands
// are not interchangeable because (lhs |= EnumT::None) != (lhs | EnumT::None).
// Only supports bitmask enum class types.
template <typename EnumT,
	typename = std::enable_if_t<details::is_scoped_enum_v<EnumT>>,
	typename = std::enable_if_t<std::is_same_v<get_enum_category_t<EnumT>, enum_bitmask>>>
constexpr EnumT &operator|=(EnumT &lhs, const EnumT &rhs) noexcept {
	if constexpr (enum_has_none<EnumT>()) {
		if (!(!lhs || !rhs))
			lhs = enum_cast<EnumT>(value_cast(lhs) | value_cast(rhs));
	}
	else
		lhs = enum_cast<EnumT>(value_cast(lhs) | value_cast(rhs));
	return lhs;
}

// If neither operand is EnumT::None, returns the intersection of the two bit
// masks. Otherwise returns EnumT::None. Only supports bitmask enum class types.
template <typename EnumT,
	typename = std::enable_if_t<details::is_scoped_enum_v<EnumT>>,
	typename = std::enable_if_t<std::is_same_v<get_enum_category_t<EnumT>, enum_bitmask>>>
constexpr EnumT operator&(const EnumT &lhs, const EnumT &rhs) noexcept {
	if constexpr (enum_has_none<EnumT>())
		if (!lhs || !rhs)
			return enum_none<EnumT>();
	return enum_cast<EnumT>(value_cast(lhs) & value_cast(rhs));
}

// If neither operand is EnumT::None, assigns (lhs & rhs) to lhs. The operands
// are not interchangeable because (lhs &= EnumT::None) != (lhs & EnumT::None).
// Only supports bitmask enum class types.
template <typename EnumT,
	typename = std::enable_if_t<details::is_scoped_enum_v<EnumT>>,
	typename = std::enable_if_t<std::is_same_v<get_enum_category_t<EnumT>, enum_bitmask>>>
constexpr EnumT &operator&=(EnumT &lhs, const EnumT &rhs) noexcept {
	if constexpr (enum_has_none<EnumT>()) {
		if (!(!lhs || !rhs))
			lhs = enum_cast<EnumT>(value_cast(lhs) & value_cast(rhs));
	}
	else
		lhs = enum_cast<EnumT>(value_cast(lhs) & value_cast(rhs));
	return lhs;
}

// If neither operand is EnumT::None, returns the symmetric difference of the
// two bit masks. Otherwise returns EnumT::None. Only supports bitmask enum
// class types.
template <typename EnumT,
	typename = std::enable_if_t<details::is_scoped_enum_v<EnumT>>,
	typename = std::enable_if_t<std::is_same_v<get_enum_category_t<EnumT>, enum_bitmask>>>
constexpr EnumT operator^(const EnumT &lhs, const EnumT &rhs) noexcept {
	if constexpr (enum_has_none<EnumT>())
		if (!lhs || !rhs)
			return enum_none<EnumT>();
	return enum_cast<EnumT>(value_cast(lhs) ^ value_cast(rhs));
}

// If neither operand is EnumT::None, assigns (lhs ^ rhs) to lhs. The operands
// are not interchangeable because (lhs ^= EnumT::None) != (lhs ^ EnumT::None).
// Only supports bitmask enum class types.
template <typename EnumT,
	typename = std::enable_if_t<details::is_scoped_enum_v<EnumT>>,
	typename = std::enable_if_t<std::is_same_v<get_enum_category_t<EnumT>, enum_bitmask>>>
constexpr EnumT &operator^=(EnumT &lhs, const EnumT &rhs) noexcept {
	if constexpr (enum_has_none<EnumT>()) {
		if (!(!lhs || !rhs))
			lhs = enum_cast<EnumT>(value_cast(lhs) ^ value_cast(rhs));
	}
	else
		lhs = enum_cast<EnumT>(value_cast(lhs) ^ value_cast(rhs));
	return lhs;
}

// If lhs is not equal to EnumT::None, toggles all bits in lhs. Otherwise
// returns EnumT::None. Only supports bitmask enum class types.
template <typename EnumT,
	typename = std::enable_if_t<details::is_scoped_enum_v<EnumT>>,
	typename = std::enable_if_t<std::is_same_v<get_enum_category_t<EnumT>, enum_bitmask>>>
constexpr EnumT operator~(const EnumT &lhs) noexcept {
	if constexpr (enum_has_none<EnumT>())
		if (!lhs)
			return enum_none<EnumT>();
	return enum_cast<EnumT>(~value_cast(lhs));
}

} // inline namespace enum_operators



template <>
struct enum_category_traits<enum_default> {
	using category = enum_default;

	template <typename EnumT,
		typename = std::enable_if_t<std::is_same_v<get_enum_category_t<EnumT>, category>>>
	static constexpr bool valid() noexcept {
		return true;
	}

	template <typename EnumT, typename ValT,
		typename = std::enable_if_t<std::is_same_v<get_enum_category_t<EnumT>, category>>>
	static constexpr EnumT enum_cast(ValT x) noexcept {
		return static_cast<EnumT>(x);
	}
};

template <>
struct enum_category_traits<enum_standard> {
	using category = enum_standard;

	template <typename EnumT,
		typename = std::enable_if_t<std::is_same_v<get_enum_category_t<EnumT>, category>>>
	static constexpr bool valid() noexcept {
		if constexpr (enum_has_none<EnumT>()) {
			constexpr auto xnone = details::value_cast_impl(enum_none<EnumT>());
			constexpr auto xmin = details::value_cast_impl(enum_min<EnumT>());
			constexpr auto xmax = details::value_cast_impl(enum_max<EnumT>());
			return xmin <= xmax && xnone != details::clamp(xnone, xmin, xmax);
		}
		else
			return false;
	}

	template <typename EnumT, typename ValT,
		typename = std::enable_if_t<std::is_same_v<get_enum_category_t<EnumT>, category>>>
	static constexpr EnumT enum_cast(ValT x) noexcept {
		constexpr auto xnone = details::value_cast_impl(enum_none<EnumT>());
		constexpr auto xmin = details::value_cast_impl(enum_min<EnumT>());
		constexpr auto xmax = details::value_cast_impl(enum_max<EnumT>());
		return static_cast<EnumT>(x == details::clamp(x, xmin, xmax) ? x : xnone);
	}
};

template <>
struct enum_category_traits<enum_linear> {
	using category = enum_linear;

	template <typename EnumT,
		typename = std::enable_if_t<std::is_same_v<get_enum_category_t<EnumT>, category>>>
	static constexpr bool valid() noexcept {
		if constexpr (enum_has_none<EnumT>()) {
			constexpr auto xnone = details::value_cast_impl(enum_none<EnumT>());
			constexpr auto xmin = details::value_cast_impl(enum_min<EnumT>());
			constexpr auto xmax = details::value_cast_impl(enum_max<EnumT>());
			return xmin <= xmax && xnone != details::clamp(xnone, xmin, xmax);
		}
		else
			return false;
	}

	template <typename EnumT, typename ValT,
		typename = std::enable_if_t<std::is_same_v<get_enum_category_t<EnumT>, category>>>
	static constexpr EnumT enum_cast(ValT x) noexcept {
		constexpr auto xnone = details::value_cast_impl(enum_none<EnumT>());
		constexpr auto xmin = details::value_cast_impl(enum_min<EnumT>());
		constexpr auto xmax = details::value_cast_impl(enum_max<EnumT>());
		return static_cast<EnumT>(x == xnone ? xnone : details::clamp(x, xmin, xmax));
	}
};

template <>
struct enum_category_traits<enum_bitmask> {
	using category = enum_bitmask;

	template <typename EnumT,
		typename = std::enable_if_t<std::is_same_v<get_enum_category_t<EnumT>, category>>>
	static constexpr bool valid() noexcept {
		constexpr auto xmin = details::value_cast_impl(enum_min<EnumT>());
		constexpr auto xmax = details::value_cast_impl(enum_max<EnumT>());
		if constexpr (std::is_unsigned_v<std::underlying_type_t<EnumT>> && (xmin & xmax) == xmin) {
			if constexpr (enum_has_none<EnumT>()) {
				constexpr auto xnone = details::value_cast_impl(enum_none<EnumT>());
				return xnone != details::bitwise_clamp(xnone, xmin, xmax);
			}
			else
				return true;
		}
		else
			return false;
	}

	template <typename EnumT, typename ValT,
		typename = std::enable_if_t<std::is_same_v<get_enum_category_t<EnumT>, category>>>
	static constexpr EnumT enum_cast(ValT x) noexcept {
		constexpr auto xmin = details::value_cast_impl(enum_min<EnumT>());
		constexpr auto xmax = details::value_cast_impl(enum_max<EnumT>());
		if constexpr (!enum_has_none<EnumT>())
			return static_cast<EnumT>(details::bitwise_clamp(x, xmin, xmax));
		else {
			constexpr auto xnone = details::value_cast_impl(enum_none<EnumT>());
			return static_cast<EnumT>(x == xnone ? xnone : details::bitwise_clamp(x, xmin, xmax));
		}
	}
};

template <typename EnumT, EnumT... Vals>
struct enum_category_traits<enum_discrete<EnumT, Vals...>> {
	using category = enum_discrete<EnumT, Vals...>;

	template <typename EnumT_,
		typename = std::enable_if_t<std::is_same_v<get_enum_category_t<EnumT_>, category>>>
	static constexpr bool valid() noexcept {
		if constexpr (enum_has_none<EnumT_>())
			return std::is_same_v<EnumT_, EnumT> && !(... || (Vals == EnumT_::none));
		else
			return false;
	}

	template <typename EnumT_, typename ValT,
		typename = std::enable_if_t<std::is_same_v<get_enum_category_t<EnumT_>, category>>,
		typename = std::enable_if_t<std::is_convertible_v<EnumT_, EnumT>>>
	static constexpr EnumT enum_cast(ValT x) noexcept {
		if ((... || (details::value_cast_impl(Vals) == x)))
			return static_cast<EnumT>(x);
		return EnumT::none;
	}
};



#define ENUM_CLASS_WITH_CATEGORY(NAME, TYPE, CATEGORY) \
enum class NAME : TYPE; \
template <> \
struct enum_traits<NAME> { using category = CATEGORY; }; \
enum class NAME : TYPE

// Defines a standard-layout enum class as by:
//  enum class NAME : TYPE
// with a specialization of enum_traits<NAME>.
#define ENUM_CLASS_STANDARD(NAME, TYPE) ENUM_CLASS_WITH_CATEGORY(NAME, TYPE, enum_standard)

// Defines a linear enum class as by:
//  enum class NAME : TYPE
// with a specialization of enum_traits<NAME>.
#define ENUM_CLASS_LINEAR(NAME, TYPE) ENUM_CLASS_WITH_CATEGORY(NAME, TYPE, enum_linear)

// Defines a bitmask enum class as by:
//  enum class NAME : TYPE
// with a specialization of enum_traits<NAME>.
#define ENUM_CLASS_BITMASK(NAME, TYPE) ENUM_CLASS_WITH_CATEGORY(NAME, TYPE, enum_bitmask)
