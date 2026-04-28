# Driven Collection

`UDataIndexerDrivenCollection` is an abstract `UDataAsset` base class for assets whose entries are automatically synchronized with a source repository.

## Concept

A driven collection is a derived asset: it watches a `UDataIndexerRepository` and maintains a parallel map keyed by the same `FDataIndexerPrimaryKey` set. When rows are added or removed from the source repository in the editor, the collection rebuilds its entries automatically.

**Typical use cases:**

- Per-row editor configuration that shouldn't live inside the repository (e.g., curve assets, spawn data overrides)
- Derived lookup tables that need a different value type than the repository's row struct
- Assets that aggregate repository data into a form optimized for runtime access

## SourceRepository

```cpp
UPROPERTY(EditDefaultsOnly, Category = Settings)
TObjectPtr<UDataIndexerRepository> SourceRepository;
```

Set this in the asset's details panel. When `SourceRepository` is saved or loaded in the editor, the collection calls `Rebuild()` on its `EntryBuilder`.

!!! warning "Editor-only rebuild"
    The rebuild mechanism (`TEntryBuilder`, `PostEditChangeProperty`, asset load delegates) is entirely `WITH_EDITOR`. The collection's data is baked into the asset at save time and accessed at runtime via normal UPROPERTY serialization — there is no runtime dependency on the repository.

## Subclassing in C++

Implement a `TEntryBuilder<TValue>` and assign it in the constructor:

```cpp
// MyDrivenCollection.h
UCLASS()
class UMyDrivenCollection : public UDataIndexerDrivenCollection
{
    GENERATED_BODY()

public:
    UMyDrivenCollection();

    UPROPERTY(EditDefaultsOnly)
    TMap<FDataIndexerPrimaryKey, FMyCurveData> Entries;

#if WITH_EDITOR
private:
    class FMyEntryBuilder : public TEntryBuilder<FMyCurveData>
    {
    public:
        explicit FMyEntryBuilder(UMyDrivenCollection& Asset)
            : TEntryBuilder(Asset) {}

    private:
        virtual TMap<FDataIndexerPrimaryKey, FMyCurveData>& GetEntries() const override
        {
            return GetAsset<UMyDrivenCollection>().Entries;
        }

        virtual FMyCurveData GetDefaultValue(const FDataIndexerPrimaryKey& Key) const override
        {
            return FMyCurveData{};
        }
    };
#endif
};

// MyDrivenCollection.cpp
UMyDrivenCollection::UMyDrivenCollection()
{
#if WITH_EDITOR
    EntryBuilder = MakeShared<FMyEntryBuilder>(*this);
#endif
}
```

## Rebuild behavior

`TEntryBuilder<TValue>::Rebuild()` performs a stable merge:

1. Iterates all primary keys from `SourceRepository`
2. Removes entries whose keys no longer exist in the repository
3. Adds default-constructed entries for new keys
4. Stable-sorts entries to match the repository's row order

Existing entries whose keys are still present are left untouched — their values survive the rebuild.

## Subclassing in Blueprint

Blueprint subclassing of `UDataIndexerDrivenCollection` is not supported. The entry builder pattern requires C++ template instantiation.
