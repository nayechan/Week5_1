#pragma once

#include "Core/Public/Object.h"

template<typename TObject>
struct TObjectRange;

template<typename TObject>
class TObjectIterator
{
	friend struct TObjectRange<TObject>;

public:
	TObjectIterator()
		: Index(-1)
	{
		GetObjectsOfClass();
		Advance();
	}

	void operator++()
	{
		Advance();
	}

	/** @note: Iterator가 nullptr가 아닌지 보장하지 않음 */
	explicit operator bool() const
	{
		return 0 <= Index && Index < ObjectArray.size();
	}

	bool operator!() const
	{
		return !(bool)*this;
	}

	TObject* operator*() const
	{
		return (TObject*)GetObject();
	}

	TObject* operator->() const
	{
		return (TObject*)GetObject();
	}

	bool operator==(const TObjectIterator& Rhs) const
	{
		return Index == Rhs.Index;
	}
	bool operator!=(const TObjectIterator& Rhs) const
	{
		return Index != Rhs.Index;
	}

	/** @note: UE는 Thread-Safety를 보장하지만, 여기서는 Advance()와 동일하게 작동 */
	bool AdvanceIterator()
	{
		return Advance();
	}

protected:
	void GetObjectsOfClass()
	{
		ObjectArray.clear();

		for (size_t i = 0; i < GetUObjectArray().size(); ++i)
		{
			UObject* Object = Cast<TObject>(GetUObjectArray()[i]);

			if (Object)
			{
				ObjectArray.emplace_back(Object);
			}
		}
	}

	UObject* GetObject() const
	{
		/** @todo: Index가 -1일 때 nullptr을 리턴해도 괜찮은가 */
		if (Index == -1)
		{
			return nullptr;
		}
		return ObjectArray[Index];
	}

	bool Advance()
	{
		while (++Index < ObjectArray.size())
		{
			if (GetObject())
			{
				return true;
			}
		}
		return false;
	}

private:
	TObjectIterator(const TArray<UObject*>& InObjectArray, int32 InIndex)
		: ObjectArray(InObjectArray), Index(InIndex)
	{
	}

protected:
	/** @note: 언리얼 엔진은 TObjectPtr이 아닌 Raw 포인터 사용 */
	TArray<UObject*> ObjectArray;

	int32 Index;
};


template<typename TObject>
struct TObjectRange
{
public:
	TObjectRange()
		: It()
	{
	}

	/** @note: Ranged-For 지원 */
	TObjectIterator<TObject> begin() const { return It; }
	TObjectIterator<TObject> end() const
	{
		return TObjectIterator<TObject>(It.ObjectArray, It.ObjectArray.size());
	}

private:
	TObjectIterator<TObject> It;
};
