[@jprayner/piconet-nodejs](../README.md) / [Modules](../modules.md) / [types/monitorEvent](../modules/types_monitorEvent.md) / MonitorEvent

# Class: MonitorEvent

[types/monitorEvent](../modules/types_monitorEvent.md).MonitorEvent

Fired asynchronously as frames are received by the ADLC whilst in `MONITOR` mode.

This event is fired regardless of the source or destination of the frame.

## Hierarchy

- [`RxDataEvent`](types_rxDataEvent.RxDataEvent.md)

  ↳ **`MonitorEvent`**

## Table of contents

### Constructors

- [constructor](types_monitorEvent.MonitorEvent.md#constructor)

### Properties

- [econetFrame](types_monitorEvent.MonitorEvent.md#econetframe)

### Methods

- [titleForFrame](types_monitorEvent.MonitorEvent.md#titleforframe)
- [toString](types_monitorEvent.MonitorEvent.md#tostring)

## Constructors

### constructor

• **new MonitorEvent**(`econetFrame`)

#### Parameters

| Name | Type | Description |
| :------ | :------ | :------ |
| `econetFrame` | `Buffer` | The raw Econet frame. |

#### Overrides

[RxDataEvent](types_rxDataEvent.RxDataEvent.md).[constructor](types_rxDataEvent.RxDataEvent.md#constructor)

#### Defined in

[types/monitorEvent.ts:10](https://github.com/jprayner/piconet/blob/81026b7/driver/nodejs/src/types/monitorEvent.ts#L10)

## Properties

### econetFrame

• **econetFrame**: `Buffer`

The raw Econet frame.

#### Defined in

[types/monitorEvent.ts:14](https://github.com/jprayner/piconet/blob/81026b7/driver/nodejs/src/types/monitorEvent.ts#L14)

## Methods

### titleForFrame

▸ `Protected` **titleForFrame**(`frame`): `string`

#### Parameters

| Name | Type |
| :------ | :------ |
| `frame` | `Buffer` |

#### Returns

`string`

#### Inherited from

[RxDataEvent](types_rxDataEvent.RxDataEvent.md).[titleForFrame](types_rxDataEvent.RxDataEvent.md#titleforframe)

#### Defined in

[types/rxDataEvent.ts:7](https://github.com/jprayner/piconet/blob/81026b7/driver/nodejs/src/types/rxDataEvent.ts#L7)

___

### toString

▸ **toString**(): `string`

#### Returns

`string`

#### Defined in

[types/monitorEvent.ts:19](https://github.com/jprayner/piconet/blob/81026b7/driver/nodejs/src/types/monitorEvent.ts#L19)
