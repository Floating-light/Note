struct __declspec(dllimport) FFieldNotificationClassDescriptor : public ::UE::FieldNotification::IClassDescriptor
{
private:
    using SuperDescriptor = ::UE::FieldNotification::IClassDescriptor;
    static const ::UE::FieldNotification::FFieldId *AllFields[];
    friend ThisClass;

public:
    static const ::UE::FieldNotification::FFieldId ToolTipText;
    static const ::UE::FieldNotification::FFieldId Visibility;
    static const ::UE::FieldNotification::FFieldId bIsEnabled;
    enum EField
    {
        IndexOf_ToolTipText = SuperDescriptor::Max_IndexOf_ + 0,
        IndexOf_Visibility,
        IndexOf_bIsEnabled,
        Max_IndexOf_,
    };
    virtual void ForEachField(const UClass *Class, TFunctionRef<bool(::UE::FieldNotification::FFieldId FielId)> Callback) const override;
};
virtual const ::UE::FieldNotification::IClassDescriptor &GetFieldNotificationDescriptor() const
{
    static FFieldNotificationClassDescriptor Instance;
    return Instance;
}
void UWidget::FFieldNotificationClassDescriptor::ForEachField(const UClass* Class, TFunctionRef<bool(::UE::FieldNotification::FFieldId FielId)> Callback) const
{
	for (int32 Index = 0; Index < Max_IndexOf_; ++Index)
	{
		if (!Callback(*AllFields[Index]))
		{
			return;
		}
	}
	if (const UWidgetBlueprintGeneratedClass* WidgetBPClass = Cast<const UWidgetBlueprintGeneratedClass>(Class))
	{
		WidgetBPClass->ForEachField(Callback);
	}
}
const ::UE::FieldNotification::FFieldId UWidget::FFieldNotificationClassDescriptor::ToolTipText(FName(L"ToolTipText"), UWidget::FFieldNotificationClassDescriptor::IndexOf_ToolTipText);
const ::UE::FieldNotification::FFieldId UWidget::FFieldNotificationClassDescriptor::Visibility(FName(L"Visibility"), UWidget::FFieldNotificationClassDescriptor::IndexOf_Visibility);
const ::UE::FieldNotification::FFieldId UWidget::FFieldNotificationClassDescriptor::bIsEnabled(FName(L"bIsEnabled"), UWidget::FFieldNotificationClassDescriptor::IndexOf_bIsEnabled);
const ::UE::FieldNotification::FFieldId *UWidget::FFieldNotificationClassDescriptor::AllFields[] = {
    &UWidget::FFieldNotificationClassDescriptor::ToolTipText,
    &UWidget::FFieldNotificationClassDescriptor::Visibility,
    &UWidget::FFieldNotificationClassDescriptor::bIsEnabled,
}