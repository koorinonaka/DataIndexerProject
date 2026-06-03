#pragma once

#include "DataIndexerDrivenCollection.h"
#include "DataIndexerTypes.h"

#include "CharacterClassIconCollection.generated.h"

class UTexture2D;

/**
 * Driven collection of per-character-class icons. Entries are keyed by the
 * CharacterClassRepository's FDataIndexerPrimaryKey set and synchronized automatically in
 * the editor. Icons are kept out of the row data as soft references so hard-reference loading
 * can be managed independently.
 */
UCLASS( BlueprintType )
class DATAINDEXERPROJECT_API UCharacterClassIconCollection : public UDataIndexerDrivenCollection
{
	GENERATED_BODY()

public:
	UCharacterClassIconCollection();

	UFUNCTION( BlueprintCallable, Category = UI )
	TSoftObjectPtr<UTexture2D> GetIcon( const FDataIndexerPrimaryKey& Key ) const;

protected:
	UPROPERTY( EditDefaultsOnly, Category = UI, EditFixedSize,	  //
		meta = ( ReadOnlyKeys, Repository = "SourceRepository" ) )
	TMap<FDataIndexerPrimaryKey, TSoftObjectPtr<UTexture2D>> Icons;

#if WITH_EDITOR
private:
	class FIconEntryBuilder : public TEntryBuilder<TSoftObjectPtr<UTexture2D>>
	{
	public:
		explicit FIconEntryBuilder( UCharacterClassIconCollection& Asset ) : TEntryBuilder( Asset ) {}

	private:
		virtual TMap<FDataIndexerPrimaryKey, TSoftObjectPtr<UTexture2D>>& GetEntries() const override
		{
			return GetAsset<UCharacterClassIconCollection>().Icons;
		}
	};
#endif
};
