[@jprayner/piconet-nodejs](../README.md) / [Modules](../modules.md) / [index](../modules/index.md) / RxImmediateEvent

# Class: RxImmediateEvent

[index](../modules/index.md).RxImmediateEvent

Fired asynchronously whilst in `LISTEN` mode as `IMMEDIATE` operation packets are received for the
local Econet station.

## Hierarchy

- [`RxDataEvent`](index.RxDataEvent.md)

  ↳ **`RxImmediateEvent`**

## Table of contents

### Constructors

- [constructor](index.RxImmediateEvent.md#constructor)

### Properties

- [dataFrame](index.RxImmediateEvent.md#dataframe)
- [scoutFrame](index.RxImmediateEvent.md#scoutframe)

### Methods

- [titleForFrame](index.RxImmediateEvent.md#titleforframe)
- [toString](index.RxImmediateEvent.md#tostring)

## Constructors

### constructor

• **new RxImmediateEvent**(`scoutFrame`, `dataFrame`)

#### Parameters

| Name | Type | Description |
| :------ | :------ | :------ |
| `scoutFrame` | `Buffer` | The raw scout frame. |
| `dataFrame` | `Buffer` | The raw data frame. |

#### Overrides

[RxDataEvent](index.RxDataEvent.md).[constructor](index.RxDataEvent.md#constructor)

#### Defined in

[types/rxImmediateEvent.ts:9](https://github.com/jprayner/piconet/blob/aed9c79/driver/nodejs/src/types/rxImmediateEvent.ts#L9)

## Properties

### dataFrame

• **dataFrame**: `Buffer`

The raw data frame.

#### Defined in

[types/rxImmediateEvent.ts:18](https://github.com/jprayner/piconet/blob/aed9c79/driver/nodejs/src/types/rxImmediateEvent.ts#L18)

___

### scoutFrame

• **scoutFrame**: `Buffer`

The raw scout frame.

#### Defined in

[types/rxImmediateEvent.ts:13](https://github.com/jprayner/piconet/blob/aed9c79/driver/nodejs/src/types/rxImmediateEvent.ts#L13)

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

[types/rxDataEvent.ts:12](https://github.com/jprayner/piconet/blob/aed9c79/driver/nodejs/src/types/rxDataEvent.ts#L12)

___

### toString

▸ **toString**(): `string`

#### Returns

`string`

#### Defined in

[types/rxImmediateEvent.ts:23](https://github.com/jprayner/piconet/blob/aed9c79/driver/nodejs/src/types/rxImmediateEvent.ts#L23)
