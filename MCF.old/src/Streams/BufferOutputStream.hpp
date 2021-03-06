// 这个文件是 MCF 的一部分。
// 有关具体授权说明，请参阅 MCFLicense.txt。
// Copyleft 2013 - 2018, LH_Mouse. All wrongs reserved.

#ifndef MCF_STREAMS_BUFFER_OUTPUT_STREAM_HPP_
#define MCF_STREAMS_BUFFER_OUTPUT_STREAM_HPP_

#include "AbstractOutputStream.hpp"
#include "../Core/StreamBuffer.hpp"

namespace MCF {

class BufferOutputStream : public AbstractOutputStream {
private:
	StreamBuffer x_vBuffer;

public:
	explicit BufferOutputStream(StreamBuffer vBuffer = StreamBuffer()) noexcept
		: x_vBuffer(std::move(vBuffer))
	{ }
	~BufferOutputStream() override;

public:
	void Put(unsigned char byData) override;
	void Put(const void *pData, std::size_t uSize) override;
	void Flush(bool bHard) override;

	const StreamBuffer &GetBuffer() const noexcept {
		return x_vBuffer;
	}
	StreamBuffer &GetBuffer() noexcept {
		return x_vBuffer;
	}
	void SetBuffer(StreamBuffer vBuffer) noexcept {
		x_vBuffer = std::move(vBuffer);
	}
};

}

#endif
