#pragma once

#include "DataIndexerTypes.h"

/**
 * Declares a static accessor that returns a lazily initialised FDataIndexerIndex.
 *
 * The GUID seed is "ClassName.VarName" derived from StaticClass()->GetPathName() at first call,
 * so it stays correct when the class is renamed without any manual string to update.
 *
 * Usage (inside a UCLASS body):
 *   DI_DEFINE_INDEX( ByClassIndex );
 *
 * Access:
 *   UMySchema::ByClassIndex()            — returns the full FDataIndexerIndex
 *   UMySchema::ByClassIndex().Identifier — the raw FGuid, if needed
 */
#define DI_DEFINE_INDEX( VarName ) \
	static const FDataIndexerIndex& VarName() \
	{ \
		static const FDataIndexerIndex Key \
		{ \
			FGuid::NewDeterministicGuid( StaticClass()->GetPathName() + TEXT( "." #VarName ) ), \
			INVTEXT( #VarName ) \
		}; \
		return Key; \
	}
