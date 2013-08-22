void UsbStor_Init(void);
/**
 ****************************************************************
 * Tell the usb storage layer when the Unit is now ready.
 * This allows initialization of the SD-Card or drive spin up
 ****************************************************************
 */
void UsbStor_UnitReady(bool value);
