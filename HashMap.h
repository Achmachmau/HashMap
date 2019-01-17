#pragma once

template<class T>
class MyHash
{
public:
	size_t operator()( const T& item ) const
	{
		return std::hash<T>()( item );
	}
};

template <typename TKey, typename TValue>
class THashEntry
{
	typedef THashEntry<TKey, TValue> HashEntry;

//!!! private
public:
	bool 	UsedFlag = true;	//!? use MAGIC_VALUE
	TKey 	Key;
	TValue 	Value;
	size_t 	HashValue;

	THashEntry() {};

	THashEntry( const TKey& key, const TValue& value )
		: Key(key),
		  Value (value)
	{
		HashValue = MyHash<TKey>()( Key );
	}

	THashEntry( const HashEntry& other )
		: UsedFlag( other.UsedFlag ),
		  Key( other.Key ),
		  Value( other.Value ),
		  HashValue( other.HashValue ) {};

	THashEntry( const HashEntry&& tmp )
		: THashEntry()
	{
		Swap( tmp );
	}

	~THashEntry()
	{
		UsedFlag = false;
		//Key.~TKey();
		//Value.~TValue();
		HashValue = 0;
	}

	HashEntry& operator=( const HashEntry& other )
	{
		UsedFlag = other.UsedFlag;
		Key = other.Key;
		Value = other.Value;
		HashValue = other.HashValue;
		return *this;
	}
	
	HashEntry& operator=( HashEntry&& tmp )
	{
		Swap( tmp );
		return *this;
	}

private:
	void Swap( HashEntry& tmp )
	{
		std::swap( UsedFlag, tmp.UsedFlag );
		std::swap( Key, tmp.Key );
		std::swap( Value, tmp.Value );
		std::swap( HashValue, tmp.HashValue );
	}
};

template <typename TKey, typename TValue>
class THashMap
{
	typedef THashEntry<TKey, TValue> HashEntry;
	typedef THashMap<TKey, TValue> HashMap;

	HashEntry*	pData;		
	unsigned	logN;		//!? byte

public:
	THashMap() : pData( nullptr ) {};

	THashMap( const HashMap& other )
	{
		Copy( other );
	}

	THashMap( const HashMap&& tmp )
		: THashMap()
	{
		Swap( std::move( tmp ) );
	}

	~THashMap()
	{
		Clear();
	}

	void Insert( const TKey& key, const TValue& value )
	{
		HashEntry entry( key, value ) ;

		if( !pData )
		{
			Allocate();
		}

		size_t index = GetHashMod( entry.HashValue ),
			priceEntry = 0,
			priceAnotherEntry = 0;

		auto maxShift = GetMaxShift();
		auto size = BucketCount();

		while( pData[ index ].UsedFlag )
		{
			if( entry.Key == pData[ index ].Key )
				return;

			priceAnotherEntry = GetPrice( index, pData[ index ].HashValue );

			if( priceEntry > priceAnotherEntry )
			{
//!!!AAAAAAAAAaaa MoveSemantic
				HashEntry tmp( pData[ index ] );
				pData[ index ] = entry;
				entry = tmp;
				//std::swap( entry, pData[ index ] );
			}

			++index %= size;
			priceEntry = GetPrice( index, entry.HashValue );

			if( priceEntry > maxShift )
			{
				Allocate( logN + 1 );

				index = GetHashMod( entry.HashValue );
				priceEntry = GetPrice( index, entry.HashValue );
				maxShift = GetMaxShift();
				size = BucketCount();
			}
		}

		new ( &pData[ index ] ) HashEntry( /*static_cast<HashEntry&&>*/(entry) );
		return;
	}

	void Erase( const TKey& key )
	{
		HashEntry* searchResult = SearchEntry( key );
		if( searchResult )
			searchResult->~THashEntry();
	}

	TValue* Find( const TKey& key )
	{
		HashEntry* searchResult = SearchEntry( key );
		return searchResult ? &searchResult->Value : nullptr;
	}

	bool Empty()
	{
		if( !pData )
			return true;

		size_t size = BucketCount();
		for( size_t i = 0; i < size; ++i )
		{
			if( pData[ i ].UsedFlag )
				return false;
		}
		return true;
	}

	size_t Size()
	{
		size_t buff = BucketCount(),
			size = 0;
		for( size_t i = 0; i < buff; ++i )
		{
			if( pData[ i ].UsedFlag )
			{
				++size;
			}
		}
		return size;
	}

	size_t BucketCount( unsigned n = 0 )
	{
		return n ? ( 1 << n ) + ( GetMaxShift( n ) >> 1 )
			: ( 1 << logN ) + ( GetMaxShift() >> 1 );
	}

	void Clear( bool bBuffClear = true )
	{
		size_t size = BucketCount();
		for( size_t i = 0; i < size; ++i )
		{
			if( pData[ i ].UsedFlag )
			{
				//pData[ i ].~THashEntry();
				( ( HashEntry* ) &pData[ i ] )->~THashEntry();
			}
		}
		if( bBuffClear )
		{
			std::free( pData );
			pData = nullptr;
			logN = 0;
		}
	}

	void Rehash( size_t newSize )
	{
		if( newSize < BucketCount() )
			return;

		unsigned newLog = 0;
		while( newSize > 0 )
		{
			++newLog;
			newSize >>= 1;
		}

		if( newLog <= logN )
			return;
		
		Allocate( newLog );
	}

private:
	HashEntry* SearchEntry( const TKey& key )
	{
		size_t	hashMod = GetHashMod( MyHash<TKey>()( key ) ),
			index = hashMod;
		auto maxShift = GetMaxShift();
		auto size = BucketCount();

		while( index < hashMod + maxShift )
		{
			if( !pData[ index ].UsedFlag )
				return nullptr;

			if( pData[ index ].Key == key )
				return &( pData[ index ] );

			++index %= size;
		}

		return nullptr;
	}

	void Allocate(unsigned newLog = 5)
	{
		if( !pData )
		{
			logN = newLog;
			pData = ( HashEntry* ) std::calloc( BucketCount(), sizeof( HashEntry ) );
			assert( pData );
			return;
		}

		size_t newSize = BucketCount( newLog );
		HashEntry* newData = ( HashEntry* ) std::calloc( newSize, sizeof( HashEntry ) );
		assert( newData );

		size_t size = BucketCount();
		size_t newIndex = 0;

		logN = newLog;

		for( size_t i = 0; i < size; ++i )
		{
			if( pData[ i ].UsedFlag )
			{
				newIndex = GetHashMod( pData[ i ].HashValue );
				while( newData[ newIndex ].UsedFlag )
				{
					++newIndex;
				}
				std::memcpy( ( HashEntry* ) &newData[ newIndex ], ( HashEntry* ) &pData[ i ], sizeof( HashEntry ) );
			}
		}
		std::free( pData );
		pData = newData;
	}

	void Copy( const HashMap& other )
	{
		if( !other.pData )
			return;

		logN = other.logN;
		Allocate(logN);

		size_t size = BucketCount();
		for( size_t i = 0; i < size; ++i )
		{
			if( other.pData[ i ].UsedFlag )
			{
				new ( &pData[ i ] ) HashEntry( other.pData[ i ] );
			}
		}
	}

	void Swap( HashEntry&& tmp )
	{
		std::swap( pData, tmp.pData );
		std::swap( logN, tmp.logN );
	}

	size_t GetHashMod( const size_t& hashVal )
	{
		return hashVal >> ( 64 - logN );	//shift right on nbit(size_t)-nbit(NSlot) - up' bits
	}

//!?
	unsigned GetMaxShift(unsigned n = 0)
	{
		return n ? 2 * n
			: 2 * logN;
	}

//!?unsigned
	size_t GetPrice( const size_t& index, const size_t& hashValue )
	{
		auto size = BucketCount();
		return ( index - GetHashMod( hashValue ) + size ) % size;
	}
};
