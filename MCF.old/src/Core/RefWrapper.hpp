// 这个文件是 MCF 的一部分。
// 有关具体授权说明，请参阅 MCFLicense.txt。
// Copyleft 2013 - 2018, LH_Mouse. All wrongs reserved.

#ifndef MCF_CORE_REF_WRAPPER_HPP_
#define MCF_CORE_REF_WRAPPER_HPP_

#include "AddressOf.hpp"
#include <type_traits>
#include <utility>

namespace MCF {

template<typename ObjectT>
class RefWrapper {
private:
	std::remove_reference_t<ObjectT> *x_pObject;

public:
	constexpr RefWrapper(ObjectT &vObject) noexcept
		: x_pObject(AddressOf(vObject))
	{ }
	RefWrapper(ObjectT &&vObject) = delete;

public:
	constexpr ObjectT &Get() const noexcept {
		return *x_pObject;
	}

	void Swap(RefWrapper &vOther) noexcept {
		using std::swap;
		swap(x_pObject, vOther.x_pObject);
	}

public:
	constexpr operator ObjectT &() const noexcept {
		return Get();
	}

	template<typename ...ParamsT>
	decltype(auto) operator()(ParamsT &&...vParams) const {
		return Get()(std::forward<ParamsT>(vParams)...);
	}

	constexpr bool operator==(const RefWrapper &vOther) const {
		return Get() == vOther.Get();
	}
	constexpr bool operator==(const ObjectT &vOther) const {
		return Get() == vOther;
	}
	friend constexpr bool operator==(const ObjectT &vSelf, const RefWrapper &vOther){
		return vSelf == vOther.Get();
	}

	constexpr bool operator!=(const RefWrapper &vOther) const {
		return Get() != vOther.Get();
	}
	constexpr bool operator!=(const ObjectT &vOther) const {
		return Get() != vOther;
	}
	friend constexpr bool operator!=(const ObjectT &vSelf, const RefWrapper &vOther){
		return vSelf != vOther.Get();
	}

	constexpr bool operator<(const RefWrapper &vOther) const {
		return Get() < vOther.Get();
	}
	constexpr bool operator<(const ObjectT &vOther) const {
		return Get() < vOther;
	}
	friend constexpr bool operator<(const ObjectT &vSelf, const RefWrapper &vOther){
		return vSelf < vOther.Get();
	}

	constexpr bool operator>(const RefWrapper &vOther) const {
		return Get() > vOther.Get();
	}
	constexpr bool operator>(const ObjectT &vOther) const {
		return Get() > vOther;
	}
	friend constexpr bool operator>(const ObjectT &vSelf, const RefWrapper &vOther){
		return vSelf > vOther.Get();
	}

	constexpr bool operator<=(const RefWrapper &vOther) const {
		return Get() <= vOther.Get();
	}
	constexpr bool operator<=(const ObjectT &vOther) const {
		return Get() <= vOther;
	}
	friend constexpr bool operator<=(const ObjectT &vSelf, const RefWrapper &vOther){
		return vSelf <= vOther.Get();
	}

	constexpr bool operator>=(const RefWrapper &vOther) const {
		return Get() >= vOther.Get();
	}
	constexpr bool operator>=(const ObjectT &vOther) const {
		return Get() >= vOther;
	}
	friend constexpr bool operator>=(const ObjectT &vSelf, const RefWrapper &vOther){
		return vSelf >= vOther.Get();
	}

	friend void swap(RefWrapper &vSelf, RefWrapper &vOther) noexcept {
		vSelf.Swap(vOther);
	}
};

template<typename ObjectT>
constexpr RefWrapper<const volatile ObjectT> Ref(const volatile ObjectT &vObject) noexcept {
	return vObject;
}
template<typename ObjectT>
constexpr RefWrapper<const ObjectT> Ref(const ObjectT &vObject) noexcept {
	return vObject;
}
template<typename ObjectT>
constexpr RefWrapper<volatile ObjectT> Ref(volatile ObjectT &vObject) noexcept {
	return vObject;
}
template<typename ObjectT>
constexpr RefWrapper<ObjectT> Ref(ObjectT &vObject) noexcept {
	return vObject;
}

}

#endif
