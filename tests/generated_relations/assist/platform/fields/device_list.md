# Field Assist: platform.device_list

Use the exact generated names below. SDK setters are not a client-writable allow-list. Do not accept internal IDs, versions, Context-root assignments or readonly/generated server fields from untrusted input. Choose writable business fields explicitly. Load complete scalar state before modifying a persisted entity. E reads preserve NULL and NotLoaded without SQL.

- Model relation evaluation: `evalDeviceList()`; query view handle: `deviceListHandle()`.
- Reverse relation: `deviceList()` returns SmartList<Device>&; query `selectDeviceList()`, `selectDeviceListWith(DeviceRequest<P>)`, `withDeviceList(DeviceRequest<P>)`; expression `getDeviceList()`.

Presence: `haveDevices()`, `haveNoDevices()`. Matching: `withDeviceListMatching(child)`, `withoutDeviceListMatching(child)`.

Count: `countDevices()`, `countDevicesAs(alias)`, `countDevicesWith(alias, child)`. Statistics/details: `statsFromDevices(child)`, `statsFromDevicesAs(alias, child)`, `groupByDevicesWithDetails(child)`.

[Model/action index](../../../ASSIST.md)
