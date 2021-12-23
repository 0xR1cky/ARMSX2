
#include "PrecompiledHeader.h"
#include "Memcard.h"

Memcard::Memcard(size_t port, size_t slot)
{
	this->port = port;
	this->slot = slot;
}

Memcard::~Memcard() = default;

MemcardType Memcard::GetMemcardType()
{
	return type;
}

MemcardPS1* Memcard::GetMemcardPS1()
{
	return memcardPS1.get();
}

MemcardPS2* Memcard::GetMemcardPS2()
{
	return memcardPS2.get();
}

void Memcard::SetMemcardType(MemcardType type)
{
	this->type = type;

	switch (type)
	{
		case MemcardType::PS1:
			memcardPS1 = std::make_unique<MemcardPS1>();
			memcardPS2.reset();
			break;
		case MemcardType::PS2:
			memcardPS1.reset();
			memcardPS2 = std::make_unique<MemcardPS2>(port, slot);
			break;
		case MemcardType::EJECTED:
			memcardPS1.reset();
			memcardPS2.reset();
			break;
		default:
			break;
	}
}
