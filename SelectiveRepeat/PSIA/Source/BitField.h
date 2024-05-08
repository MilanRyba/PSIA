#pragma once
#include <cstdint>

template<uint32_t Bits, typename Type = uint32_t>
class BitField
{
public:
	BitField()
	{
		ClearAll();
	}

	// Set every bit to 0
	void ClearAll()
	{
		memset(mData, 0x00000000, sizeof(Type) * sElements());
	}

	// Set every bit to 1
	void SetAll()
	{
		memset(mData, 0xffffffff, sizeof(Type) * sElements());
	}

	void SetBit(uint32_t inBit)
	{
		// TODO: Turn it into an assertion
		if (inBit < Bits)
			mData[sUintIndexOfBit(inBit)] |= sCreateBitMaskForType(inBit);
	}

	void ClearBit(uint32_t inBit)
	{
		// TODO: Turn it into an assertion
		if (inBit < Bits)
			mData[sUintIndexOfBit(inBit)] &= ~sCreateBitMaskForType(inBit);
	}

	void AssignBit(uint32_t inBit, bool inSet)
	{
		inSet ? SetBit(inBit) : ClearBit(inBit);
	}

	bool GetBit(uint32_t inBit) const
	{
		return (mData[sUintIndexOfBit(inBit)] & sCreateBitMaskForType(inBit)) != 0;
	}

	void LeastSignificantBit(uint32_t* outIndex) const
	{
		for (uint32_t i = 0; i < sElements(); i++)
		{
			*outIndex = ~0u;
			Type mask = mData[i];
			while (mask)
			{
				outIndex += 1;
				if ((mask & 1) == 1)
				{
					outIndex += i * sBitsPerType();
					return;
				}
				mask >>= 1;
			}
		}
	}

private:
	// Create a uint bit mask where only the inBit is set to 1
	static constexpr Type sCreateBitMaskForType(uint32_t inBit)
	{
		return (Type)1 << (inBit % sBitsPerType());
	}

	// Return the index of the uint of this bit
	static constexpr uint32_t sUintIndexOfBit(uint32_t inBit)
	{
		return inBit / sBitsPerType();
	}

	static constexpr uint32_t sBitsPerType()
	{
		return sizeof(Type) * 8;
	}

	// Return the amount of uints this BitField has
	static constexpr uint32_t sElements()
	{
		return (Bits + sBitsPerType() - 1) / sBitsPerType();
	}

	Type mData[sElements()];
};


