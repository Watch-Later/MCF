#include <MCF/StdMCF.hpp>
#include <MCF/Core/Variant.hpp>
#include <cstdio>

template class MCF::Variant<char, int, double>;

extern "C" unsigned _MCFCRT_Main(void) noexcept {
	MCF::Variant<char, int, double> v;
	try {
		v = 12.34;
		auto p = v.Require<double>();
		std::printf("value = %f\n", *p);
	} catch(MCF::Exception &e){
		std::printf("exception caught: what = %s\n", e.what());
	}
	return 0;
}
