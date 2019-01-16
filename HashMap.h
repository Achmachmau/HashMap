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

	THashEntry( HashEntry&& tmp )
	{
		Swap( std::move(tmp) );
	}

	~THashEntry()
	{
		UsedFlag = false;
		Key.~TKey();
		Value.~TValue();
		HashValue = 0;
	}

	HashEntry& operator=( const HashEntry& other )
	{
		UsedFlag = other.UsedFlag;
		Key = other.Key;
		Value = other.Value;
		HashValue = other.HashValue;
	}

	HashEntry& operator=( HashEntry&& tmp )
	{
		Swap( std::move(tmp) );
		return *this;
	}

private:
	void Swap( HashEntry&& tmp )
	{
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
	unsigned	logN = 6;		//!? byte

public:
	THashMap() : pData( nullptr ) {};

	THashMap( const HashMap& other )
	{
		Copy( other );
	}

	THashMap( HashMap&& tmp )
	{
		Swap( std::move( tmp ) );
	}

	void Insert( const TKey& key, const TValue& value )
	{
		if( !pData )
		{
			Allocate();
		}
		InsertEntry( HashEntry( key, value ) );
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

	size_t Size( unsigned n = 0 )
	{
		return n ? ( 1 << n ) + n
			: ( 1 << logN ) + logN;
	}

private:
	HashEntry* SearchEntry( const TKey& key )
	{
		size_t	hashMod = GetHashMod( MyHash<TKey>()( key ) ),
			index = hashMod;

		while( index < hashMod + logN )
		{
			if( !pData[ index ].UsedFlag )
				return nullptr;

			if( pData[ index ].Key == key )
				return &( pData[ index ] );

			++index;
		}

		return nullptr;
	}

	void InsertEntry( HashEntry&& entry )
	{
		size_t	hashMod = GetHashMod( entry.HashValue ),
				index = hashMod;

		while( pData[ index ].UsedFlag ) //unused/deleted
		{
			if( entry.Key == pData[ index ].Key )
				return;

			auto priceAnotherEntry = index - ( pData[ index ].HashValue >> ( 64 - logN ) );
			auto priceEntry = index - hashMod;
			if( priceEntry > logN )
			{
				Expand();
				InsertEntry( std::move(entry) );
				return;
			}

			if( priceEntry > priceAnotherEntry )
			{
				entry = std::move( pData[ index ] );
			}

			++index;
		}
		new ( &pData[ index ] ) HashEntry( entry );
	}

	void Expand()
	{
		++logN;
		Allocate();
	}

	void Allocate()
	{
		if( !pData )
		{
			pData = ( HashEntry* ) std::calloc( Size(), sizeof( HashEntry ) );
			assert( pData );
			return;
		}

		HashEntry* newData = ( HashEntry* ) std::calloc( Size(), sizeof( HashEntry ) );
		assert( newData );

		size_t oldSize = Size(logN-1);
		for( size_t i = 0; i < oldSize; ++i )
		{
			if( pData[ i ].UsedFlag )
			{
				size_t newIndex = GetHashMod( pData[ i ].HashValue );
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
		Allocate();
		for( size_t i = 0; i < Size(); ++i )
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
		return hashVal >> ( 64 - logN );
	}
};
