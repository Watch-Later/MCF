// 这个文件是 MCF 的一部分。
// 有关具体授权说明，请参阅 MCFLicense.txt。
// Copyleft 2014. LH_Mouse. All wrongs reserved.

#include "../StdMCF.hpp"
#include "PeerInfo.hpp"
#include "_SocketUtils.hpp"
#include "../../MCFCRT/ext/assert.h"
#include "../Core/Exception.hpp"
#include "../Core/Utilities.hpp"
using namespace MCF;

namespace {

template<typename T, std::size_t N>
inline bool IsAllZeroes(const T (&a)[N]) noexcept {
	return std::all_of(
		std::begin(a),
		std::end(a),
		[](auto v) noexcept { return v == 0; }
	);
}

}

namespace MCF {

namespace Impl {
	PeerInfo LocalInfoFromSocket(SOCKET sockLocal){
		SOCKADDR_STORAGE vSockAddr;
		auto nSockAddrSize = (int)sizeof(vSockAddr);
		if(::getsockname(sockLocal, (SOCKADDR *)&vSockAddr, &nSockAddrSize)){
			MCF_THROW(::GetLastError(), L"::getsockname() 失败。"_wso);
		}
		return PeerInfo(&vSockAddr, (std::size_t)nSockAddrSize);
	}
}

}


// 构造函数和析构函数。
PeerInfo::PeerInfo(bool bIPv6, std::uint16_t u16Port) noexcept {
	if(bIPv6){
		BZero(xm_au16IPv6);
	} else {
		BZero(xm_au16IPv4Zeroes);
		xm_au16IPv4Ones = 0xFFFF;
		BZero(xm_au8IPv4);
	}
	xm_u16Port = ::ntohs(u16Port);
}

PeerInfo::PeerInfo(const void *pSockAddr, std::size_t uSockAddrSize){
	auto &vSockAddr = *(const SOCKADDR_STORAGE *)pSockAddr;
	if((vSockAddr.ss_family == AF_INET6) && (uSockAddrSize >= sizeof(SOCKADDR_IN6))){
		const auto &vSockAddrIn6 = reinterpret_cast<const SOCKADDR_IN6 &>(vSockAddr);
		BCopy(xm_au16IPv6, vSockAddrIn6.sin6_addr);
		BCopy(xm_u16Port, vSockAddrIn6.sin6_port);
	} else if((vSockAddr.ss_family == AF_INET) && (uSockAddrSize >= sizeof(SOCKADDR_IN))){
		BZero(xm_au16IPv4Zeroes);
		xm_au16IPv4Ones = 0xFFFF;

		const auto &vSockAddrIn = reinterpret_cast<const SOCKADDR_IN &>(vSockAddr);
		BCopy(xm_au8IPv4, vSockAddrIn.sin_addr);
		BCopy(xm_u16Port, vSockAddrIn.sin_port);
	} else {
		MCF_THROW(ERROR_NOT_SUPPORTED, L"不支持该协议。"_wso);
	}
}

PeerInfo::PeerInfo(
	std::uint16_t u16IPv6_0,
	std::uint16_t u16IPv6_1,
	std::uint16_t u16IPv6_2,
	std::uint16_t u16IPv6_3,
	std::uint16_t u16IPv6_4,
	std::uint16_t u16IPv6_5,
	std::uint16_t u16IPv6_6,
	std::uint16_t u16IPv6_7,
	std::uint16_t u16Port
) noexcept {
	xm_au16IPv6[0]	= ::ntohs(u16IPv6_0);
	xm_au16IPv6[1]	= ::ntohs(u16IPv6_1);
	xm_au16IPv6[2]	= ::ntohs(u16IPv6_2);
	xm_au16IPv6[3]	= ::ntohs(u16IPv6_3);
	xm_au16IPv6[4]	= ::ntohs(u16IPv6_4);
	xm_au16IPv6[5]	= ::ntohs(u16IPv6_5);
	xm_au16IPv6[6]	= ::ntohs(u16IPv6_6);
	xm_au16IPv6[7]	= ::ntohs(u16IPv6_7);
	xm_u16Port		= ::ntohs(u16Port);
}

PeerInfo::PeerInfo(
	std::uint8_t u8IPv4_0,
	std::uint8_t u8IPv4_1,
	std::uint8_t u8IPv4_2,
	std::uint8_t u8IPv4_3,
	std::uint16_t u16Port
) noexcept {
	BZero(xm_au16IPv4Zeroes);
	xm_au16IPv4Ones = 0xFFFF;

	xm_au8IPv4[0]	= u8IPv4_0;
	xm_au8IPv4[1]	= u8IPv4_1;
	xm_au8IPv4[2]	= u8IPv4_2;
	xm_au8IPv4[3]	= u8IPv4_3;
	xm_u16Port		= ::ntohs(u16Port);
}

// 其他非静态成员函数。
bool PeerInfo::IsNull() const noexcept {
	if(!IsAllZeroes(xm_au16IPv4Zeroes)){
		return false;
	}
	if((xm_au16IPv4Ones != 0) && (xm_au16IPv4Ones != 0xFFFF)){
		return false;
	}
	if(!IsAllZeroes(xm_au8IPv4)){
		return false;
	}
	return true;
}
bool PeerInfo::IsIPv6Null() const noexcept {
	return IsAllZeroes(xm_au16IPv6);
}
bool PeerInfo::IsIPv4Null() const noexcept {
	return IsIPv4() && IsAllZeroes(xm_au8IPv4);
}

int PeerInfo::ToSockAddr(void *pSockAddr, std::size_t uSockAddrSize) const noexcept {
	if(IsIPv4()){
		if(uSockAddrSize < sizeof(SOCKADDR_IN)){
			return -(int)sizeof(SOCKADDR_IN);
		}
		auto &vSockAddrIn = *(SOCKADDR_IN *)pSockAddr;
		std::memset(&vSockAddrIn, 0, sizeof(vSockAddrIn));
		vSockAddrIn.sin_family = AF_INET;
		BCopy(vSockAddrIn.sin_port, xm_u16Port);
		BCopy(vSockAddrIn.sin_addr, xm_au8IPv4);
		return sizeof(SOCKADDR_IN);
	} else {
		if(uSockAddrSize < sizeof(SOCKADDR_IN6)){
			return -(int)sizeof(SOCKADDR_IN6);
		}
		auto &vSockAddrIn6 = *(SOCKADDR_IN6 *)pSockAddr;
		std::memset(&vSockAddrIn6, 0, sizeof(vSockAddrIn6));
		vSockAddrIn6.sin6_family = AF_INET6;
		BCopy(vSockAddrIn6.sin6_port, xm_u16Port);
		BCopy(vSockAddrIn6.sin6_addr, xm_au16IPv6);
		return sizeof(SOCKADDR_IN6);
	}
}

void PeerInfo::ToIPv6(std::uint16_t (&au16IPv6)[8], std::uint16_t &u16Port) const noexcept {
	for(std::size_t i = 0; i < 8; ++i){
		au16IPv6[i] = ::ntohs(xm_au16IPv6[i]);
	}
	u16Port = ::ntohs(xm_u16Port);
}

bool PeerInfo::IsIPv4() const noexcept {
	return IsAllZeroes(xm_au16IPv4Zeroes) && (xm_au16IPv4Ones == 0xFFFF);
}
void PeerInfo::ToIPv4(std::uint8_t (&au8IPv4)[4], std::uint16_t &u16Port) const noexcept {
	ASSERT(IsIPv4());

	BCopy(au8IPv4, xm_au8IPv4);
	u16Port = ::ntohs(xm_u16Port);
}
