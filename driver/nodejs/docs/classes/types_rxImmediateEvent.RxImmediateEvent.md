[@jprayner/piconet-nodejs](../README.md) / [Modules](../modules.md) / [types/rxImmediateEvent](../modules/types_rxImmediateEvent.md) / RxImmediateEvent

# Class: RxImmediateEvent

[types/rxImmediateEvent](../modules/types_rxImmediateEvent.md).RxImmediateEvent

Fired asynchronously whilst in `LISTEN` mode as `IMMEDIATE` operation packets are received for the
local Econet station.

## Hierarchy

- [`RxDataEvent`](types_rxDataEvent.RxDataEvent.md)

  ↳ **`RxImmediateEvent`**

## Table of contents

### Constructors

- [constructor](types_rxImmediateEvent.RxImmediateEvent.md#constructor)

### Properties

- [dataFrame](types_rxImmediateEvent.RxImmediateEvent.md#dataframe)
- [scoutFrame](types_rxImmediateEvent.RxImmediateEvent.md#scoutframe)

### Methods

- [titleForFrame](types_rxImmediateEvent.RxImmediateEvent.md#titleforframe)
- [toString](types_rxImmediateEvent.RxImmediateEvent.md#tostring)

## Constructors

### constructor

• **new RxImmediateEvent**(`scoutFrame`, `dataFrame`)

#### Parameters

| Name | Type | Description |
| :------ | :------ | :------ |
| `scoutFrame` | `Buffer` | The raw scout frame. |
| `dataFrame` | `Buffer` | The raw data frame. |

#### Overrides

[RxDataEvent](types_rxDataEvent.RxDataEvent.md).[constructor](types_rxDataEvent.RxDataEvent.md#constructor)

#### Defined in

[types/rxImmediateEvent.ts:9](https://github.com/jprayner/piconet/blob/81026b7/driver/nodejs/src/types/rxImmediateEvent.ts#L9)

## Properties

### dataFrame

• **dataFrame**: `Buffer`

The raw data frame.

#### Defined in

[types/rxImmediateEvent.ts:18](https://github.com/jprayner/piconet/blob/81026b7/driver/nodejs/src/types/rxImmediateEvent.ts#L18)

___

### scoutFrame

• **scoutFrame**: `Buffer`

The raw scout frame.

#### Defined in

[types/rxImmediateEvent.ts:13](https://github.com/jprayner/piconet/blob/81026b7/driver/nodejs/src/types/rxImmediateEvent.ts#L13)

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

[types/rxImmediateEvent.ts:23](https://github.com/jprayner/piconet/blob/81026b7/driver/nodejs/src/types/rxImmediateEvent.ts#L23)
