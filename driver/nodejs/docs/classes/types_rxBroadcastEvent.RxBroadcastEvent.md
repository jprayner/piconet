[@jprayner/piconet-nodejs](../README.md) / [Modules](../modules.md) / [types/rxBroadcastEvent](../modules/types_rxBroadcastEvent.md) / RxBroadcastEvent

# Class: RxBroadcastEvent

[types/rxBroadcastEvent](../modules/types_rxBroadcastEvent.md).RxBroadcastEvent

Fired asynchronously whilst in `LISTEN` mode as broadcast packets are received.

## Hierarchy

- [`RxDataEvent`](types_rxDataEvent.RxDataEvent.md)

  ↳ **`RxBroadcastEvent`**

## Table of contents

### Constructors

- [constructor](types_rxBroadcastEvent.RxBroadcastEvent.md#constructor)

### Properties

- [econetFrame](types_rxBroadcastEvent.RxBroadcastEvent.md#econetframe)

### Methods

- [titleForFrame](types_rxBroadcastEvent.RxBroadcastEvent.md#titleforframe)
- [toString](types_rxBroadcastEvent.RxBroadcastEvent.md#tostring)

## Constructors

### constructor

• **new RxBroadcastEvent**(`econetFrame`)

#### Parameters

| Name | Type | Description |
| :------ | :------ | :------ |
| `econetFrame` | `Buffer` | The raw Econet frame. |

#### Overrides

[RxDataEvent](types_rxDataEvent.RxDataEvent.md).[constructor](types_rxDataEvent.RxDataEvent.md#constructor)

#### Defined in

[types/rxBroadcastEvent.ts:8](https://github.com/jprayner/piconet/blob/81026b7/driver/nodejs/src/types/rxBroadcastEvent.ts#L8)

## Properties

### econetFrame

• **econetFrame**: `Buffer`

The raw Econet frame.

#### Defined in

[types/rxBroadcastEvent.ts:12](https://github.com/jprayner/piconet/blob/81026b7/driver/nodejs/src/types/rxBroadcastEvent.ts#L12)

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

[types/rxBroadcastEvent.ts:17](https://github.com/jprayner/piconet/blob/81026b7/driver/nodejs/src/types/rxBroadcastEvent.ts#L17)
