[@jprayner/piconet-nodejs](../README.md) / [Modules](../modules.md) / [index](../modules/index.md) / RxBroadcastEvent

# Class: RxBroadcastEvent

[index](../modules/index.md).RxBroadcastEvent

Fired asynchronously whilst in `LISTEN` mode as broadcast packets are received.

## Hierarchy

- [`RxDataEvent`](index.RxDataEvent.md)

  ↳ **`RxBroadcastEvent`**

## Table of contents

### Constructors

- [constructor](index.RxBroadcastEvent.md#constructor)

### Properties

- [econetFrame](index.RxBroadcastEvent.md#econetframe)

### Methods

- [titleForFrame](index.RxBroadcastEvent.md#titleforframe)
- [toString](index.RxBroadcastEvent.md#tostring)

## Constructors

### constructor

• **new RxBroadcastEvent**(`econetFrame`)

#### Parameters

| Name | Type | Description |
| :------ | :------ | :------ |
| `econetFrame` | `Buffer` | The raw Econet frame. |

#### Overrides

[RxDataEvent](index.RxDataEvent.md).[constructor](index.RxDataEvent.md#constructor)

#### Defined in

[types/rxBroadcastEvent.ts:8](https://github.com/jprayner/piconet/blob/21a31c9/driver/nodejs/src/types/rxBroadcastEvent.ts#L8)

## Properties

### econetFrame

• **econetFrame**: `Buffer`

The raw Econet frame.

#### Defined in

[types/rxBroadcastEvent.ts:12](https://github.com/jprayner/piconet/blob/21a31c9/driver/nodejs/src/types/rxBroadcastEvent.ts#L12)

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

[types/rxDataEvent.ts:12](https://github.com/jprayner/piconet/blob/21a31c9/driver/nodejs/src/types/rxDataEvent.ts#L12)

___

### toString

▸ **toString**(): `string`

#### Returns

`string`

#### Defined in

[types/rxBroadcastEvent.ts:17](https://github.com/jprayner/piconet/blob/21a31c9/driver/nodejs/src/types/rxBroadcastEvent.ts#L17)
