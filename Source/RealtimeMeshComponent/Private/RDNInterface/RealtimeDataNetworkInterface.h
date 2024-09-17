


class IRealtimeDataNetworkingChannel
{
	virtual ~IRealtimeDataNetworkingChannel() {}
};






class FRealtimeDataNetworkingFeature : public IModularFeature
{
private:
	static FName GetModularFeatureName() { return FName(TEXT("RealtimeDataNetworking_v0")); }

public:
	static bool IsAvailable() { return IModularFeatures::Get().IsModularFeatureAvailable(GetModularFeatureName()); }
	static FRealtimeDataNetworkingFeature* Get() { return IsAvailable() ? &IModularFeatures::Get().GetModularFeature<FRealtimeDataNetworkingFeature>(GetModularFeatureName()) : nullptr; }
	static FRealtimeDataNetworkingFeature& GetChecked() { return IModularFeatures::Get().GetModularFeature<FRealtimeDataNetworkingFeature>(GetModularFeatureName()); }


	IRealtimeDataNetworkingChannel* CreateChannel(FName Name);
	
	DECLARE_MULTICAST_DELEGATE_OneParam(FOnSectionRemoved, const TSharedRef<ISettingsSection>&)
	virtual FOnSectionRemoved& OnSectionRemoved() = 0;
};