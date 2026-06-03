#include "Collections/CharacterClassIconCollection.h"

UCharacterClassIconCollection::UCharacterClassIconCollection()
{
#if WITH_EDITOR
	EntryBuilder = MakeShared<FIconEntryBuilder>( *this );
#endif
}

TSoftObjectPtr<UTexture2D> UCharacterClassIconCollection::GetIcon( const FDataIndexerPrimaryKey& Key ) const
{
	return Icons.FindRef( Key );
}
