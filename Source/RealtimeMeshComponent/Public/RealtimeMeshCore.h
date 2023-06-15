// Copyright TriAxis Games, L.L.C. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Runtime/Launch/Resources/Version.h"
#include "StaticMeshResources.h"

// This version of the RMC is only supported by engine version 5.0.0 and above
static_assert(ENGINE_MAJOR_VERSION >= 5 && ENGINE_MINOR_VERSION >= 0);

DECLARE_STATS_GROUP(TEXT("RealtimeMesh"), STATGROUP_RealtimeMesh, STATCAT_Advanced);

#define REALTIME_MESH_MAX_TEX_COORDS MAX_STATIC_TEXCOORDS
#define REALTIME_MESH_MAX_LODS MAX_STATIC_MESH_LODS

#define REALTIME_MESH_ENABLE_DEBUG_RENDERING (!(UE_BUILD_SHIPPING || UE_BUILD_TEST) || WITH_EDITOR)

// Maximum number of elements in a vertex stream 
#define REALTIME_MESH_MAX_STREAM_ELEMENTS 8

#define REALTIME_MESH_MAX_INLINE_SECTION_ALLOCATION 16

#define REALTIME_MESH_NUM_INDICES_PER_PRIMITIVE 3

static_assert(REALTIME_MESH_MAX_STREAM_ELEMENTS >= REALTIME_MESH_MAX_TEX_COORDS, "REALTIME_MESH_MAX_STREAM_ELEMENTS must be large enough to contain REALTIME_MESH_MAX_TEX_COORDS");


struct FRealtimeMeshSectionConfig;
struct FRealtimeMeshStreamRange;
struct FRealtimeMeshLODConfig;


#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION < 2
template <typename T>
FORCEINLINE uint32 GetTypeHashHelper(const T& V) { return GetTypeHash(V); }
#endif


namespace RealtimeMesh
{
	template<typename InElementType>
	using TFixedLODArray = TArray<InElementType, TFixedAllocator<REALTIME_MESH_MAX_LODS>>;

	// Custom version for runtime mesh serialization
	namespace FRealtimeMeshVersion
	{
		enum Type
		{
			Initial = 0,

			// -----<new versions can be added above this line>-------------------------------------------------
			VersionPlusOne,
			LatestVersion = VersionPlusOne - 1
		};

		// The GUID for this custom version
		const static FGuid GUID = FGuid(0xAF3DD80C, 0x4C114B25, 0x9C7A9515, 0x5062D6E9);
	}

	enum class ERealtimeMeshStreamType
	{
		Unknown,
		Vertex,
		Index,
	};

	struct FRealtimeMeshStreamKey
	{
	private:
		ERealtimeMeshStreamType StreamType;
		FName StreamName;

	public:
		FRealtimeMeshStreamKey() : StreamType(ERealtimeMeshStreamType::Unknown), StreamName(NAME_None) { }
		FRealtimeMeshStreamKey(ERealtimeMeshStreamType InStreamType, FName InStreamName)
			: StreamType(InStreamType), StreamName(InStreamName) { }

		FName GetName() const { return StreamName; }

		ERealtimeMeshStreamType GetStreamType() const { return StreamType; }
		bool IsVertexStream() const { return StreamType == ERealtimeMeshStreamType::Vertex; }
		bool IsIndexStream() const { return StreamType == ERealtimeMeshStreamType::Index; }

		FORCEINLINE bool operator==(const FRealtimeMeshStreamKey& Other) const { return StreamType == Other.StreamType && StreamName == Other.StreamName; }
		FORCEINLINE bool operator!=(const FRealtimeMeshStreamKey& Other) const { return StreamType != Other.StreamType || StreamName != Other.StreamName; }
		
		friend inline uint32 GetTypeHash(const FRealtimeMeshStreamKey& StreamKey)
		{			
			return GetTypeHashHelper(StreamKey.StreamType) + 23 * GetTypeHashHelper(StreamKey.StreamName);
		}

		FString ToString() const
		{
			FString TypeString;

			switch(StreamType)
			{
			case ERealtimeMeshStreamType::Unknown:
				TypeString += "Unknown";
				break;
			case ERealtimeMeshStreamType::Vertex:
				TypeString += "Vertex";
				break;
			case ERealtimeMeshStreamType::Index:
				TypeString += "Index";
				break;				
			}

			return TEXT("[") + StreamName.ToString() + TEXT(", Type:") + TypeString + TEXT("]");
		}
		
		friend FArchive& operator<<(FArchive& Ar, FRealtimeMeshStreamKey& Key);
	};
	
	class FRealtimeMeshGPUBuffer;
	using FRealtimeMeshGPUBufferMap = TMap<FRealtimeMeshStreamKey, TSharedPtr<FRealtimeMeshGPUBuffer>>;

	/** Deleter function for TSharedPtrs that only allows the object to be destructed on the render thread. */
	template<typename Type>
	struct FRealtimeMeshRenderThreadDeleter
	{
		void operator()(Type* Object) const
		{
			// This is a custom deleter to make sure the runtime mesh proxy is only ever deleted on the rendering thread.
			if (IsInRenderingThread())
			{
				delete Object;
			}
			else
			{
				ENQUEUE_RENDER_COMMAND(FRealtimeMeshProxyDeleterCommand)(
					[Object](FRHICommandListImmediate& RHICmdList)
					{
						delete static_cast<Type*>(Object);
					}
				);
			}
		}
	};



	


#define CREATE_RMC_PTR_TYPES(TypeName) \
	using TypeName##Ref = TSharedRef<TypeName, ESPMode::ThreadSafe>; \
	using TypeName##Ptr = TSharedPtr<TypeName, ESPMode::ThreadSafe>; \
	using TypeName##WeakPtr = TWeakPtr<TypeName, ESPMode::ThreadSafe>;

	class FRealtimeMeshVertexFactory;
	CREATE_RMC_PTR_TYPES(FRealtimeMeshVertexFactory);
	
	struct FRealtimeMeshSectionGroupProxyInitializationParameters;
	using FRealtimeMeshSectionGroupProxyInitializationParametersRef = TSharedRef<FRealtimeMeshSectionGroupProxyInitializationParameters>;
	class FRealtimeMeshSectionGroupProxy;
	CREATE_RMC_PTR_TYPES(FRealtimeMeshSectionGroupProxy);
	
	struct FRealtimeMeshSectionProxyInitializationParameters;
	using FRealtimeMeshSectionProxyInitializationParametersRef = TSharedRef<FRealtimeMeshSectionProxyInitializationParameters>;
	class FRealtimeMeshSectionProxy;
	CREATE_RMC_PTR_TYPES(FRealtimeMeshSectionProxy);
	
	struct FRealtimeMeshLODProxyInitializationParameters;
	using FRealtimeMeshLODProxyInitializationParametersRef = TSharedRef<FRealtimeMeshLODProxyInitializationParameters>;
	class FRealtimeMeshLODProxy;
	CREATE_RMC_PTR_TYPES(FRealtimeMeshLODProxy);

	struct FRealtimeMeshProxyInitializationParameters;
	using FRealtimeMeshProxyInitializationParametersRef = TSharedRef<FRealtimeMeshProxyInitializationParameters>;
	class FRealtimeMeshProxy;
	CREATE_RMC_PTR_TYPES(FRealtimeMeshProxy);


	class FRealtimeMeshSectionGroup;
	CREATE_RMC_PTR_TYPES(FRealtimeMeshSectionGroup);
	
	class FRealtimeMeshSectionData;
	CREATE_RMC_PTR_TYPES(FRealtimeMeshSectionData);
	
	class FRealtimeMeshLODData;
	CREATE_RMC_PTR_TYPES(FRealtimeMeshLODData);
	
	class FRealtimeMesh;
	CREATE_RMC_PTR_TYPES(FRealtimeMesh);

	



#undef CREATE_RMC_PTR_TYPES



	class FRWScopeLockEx
	{
	public:
		
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1
		UE_NODISCARD_CTOR 
#endif
		explicit FRWScopeLockEx(FRWLock& InLockObject,FRWScopeLockType InLockType)
			: LockObject(InLockObject)
			, LockType(InLockType)
		{
			if(LockType != SLT_ReadOnly)
			{
				LockObject.WriteLock();
			}
			else
			{
				LockObject.ReadLock();
			}
		}
		
		void Release()
		{			
			if(LockType == SLT_ReadOnly)
			{
				LockObject.ReadUnlock();
			}
			else if (LockType == SLT_Write)
			{
				LockObject.WriteUnlock();
			}
			LockType.Reset();
		}

		void Lock(FRWScopeLockType InLockType)
		{
			check(!LockType.IsSet());
			LockType = InLockType;
			if(LockType != SLT_ReadOnly)
			{
				LockObject.WriteLock();
			}
			else
			{
				LockObject.ReadLock();
			}			
		}
	
		~FRWScopeLockEx()
		{
			Release();
		}
	
	private:
		UE_NONCOPYABLE(FRWScopeLockEx);

		FRWLock& LockObject;
		TOptional<FRWScopeLockType> LockType;
	};


}



class URealtimeMesh;
class URealtimeMeshComponent;


