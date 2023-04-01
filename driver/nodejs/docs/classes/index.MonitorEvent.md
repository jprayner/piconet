[@jprayner/piconet-nodejs](../README.md) / [Modules](../modules.md) / [index](../modules/index.md) / MonitorEvent

# Class: MonitorEvent

[index](../modules/index.md).MonitorEvent

Fired asynchronously as frames are received by the ADLC whilst in `MONITOR` mode.

This event is fired regardless of the source or destination of the frame.

## Hierarchy

- [`RxDataEvent`](index.RxDataEvent.md)

  ↳ **`MonitorEvent`**

## Table of contents

### Constructors

- [constructor](index.MonitorEvent.md#constructor)

### Properties

- [econetFrame](index.MonitorEvent.md#econetframe)

### Methods

- [titleForFrame](index.MonitorEvent.md#titleforframe)
- [toString](index.MonitorEvent.md#tostring)

## Constructors

### constructor

• **new MonitorEvent**(`econetFrame`)

#### Parameters

| Name | Type | Description |
| :------ | :------ | :------ |
| `econetFrame` | `Buffer` | The raw Econet frame. |

#### Overrides

[RxDataEvent](index.RxDataEvent.md).[constructor](index.RxDataEvent.md#constructor)

#### Defined in

[types/monitorEvent.ts:10](https://github.com/jprayner/piconet/blob/55ff188/driver/nodejs/src/types/monitorEvent.ts#L10)

## Properties

### econetFrame

• **econetFrame**: `Buffer`

The raw Econet frame.

#### Defined in

[types/monitorEvent.ts:14](https://github.com/jprayner/piconet/blob/55ff188/driver/nodejs/src/types/monitorEvent.ts#L14)

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

[RxDataEvent](index.RxDataEvent.md).[titleForFrame](index.RxDataEvent.md#titleforframe)

#### Defined in

[types/rxDataEvent.ts:12](https://github.com/jprayner/piconet/blob/55ff188/driver/nodejs/src/types/rxDataEvent.ts#L12)

___

### toString

▸ **toString**(): `string`

#### Returns

`string`

#### Defined in

[types/monitorEvent.ts:19](https://github.com/jprayner/piconet/blob/55ff188/driver/nodejs/src/types/monitorEvent.ts#L19)
