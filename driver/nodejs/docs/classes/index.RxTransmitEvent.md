[@jprayner/piconet-nodejs](../README.md) / [Modules](../modules.md) / [index](../modules/index.md) / RxTransmitEvent

# Class: RxTransmitEvent

[index](../modules/index.md).RxTransmitEvent

Fired asynchronously whilst in `LISTEN` mode as `TRANSMIT` operation packets are received for the
local Econet station.

## Hierarchy

- [`RxDataEvent`](index.RxDataEvent.md)

  ↳ **`RxTransmitEvent`**

## Table of contents

### Constructors

- [constructor](index.RxTransmitEvent.md#constructor)

### Properties

- [dataFrame](index.RxTransmitEvent.md#dataframe)
- [scoutFrame](index.RxTransmitEvent.md#scoutframe)

### Methods

- [titleForFrame](index.RxTransmitEvent.md#titleforframe)
- [toString](index.RxTransmitEvent.md#tostring)

## Constructors

### constructor

• **new RxTransmitEvent**(`scoutFrame`, `dataFrame`)

#### Parameters

| Name | Type | Description |
| :------ | :------ | :------ |
| `scoutFrame` | `Buffer` | The raw scout frame. |
| `dataFrame` | `Buffer` | The raw data frame. |

#### Overrides

[RxDataEvent](index.RxDataEvent.md).[constructor](index.RxDataEvent.md#constructor)

#### Defined in

[types/rxTransmitEvent.ts:9](https://github.com/jprayner/piconet/blob/21a31c9/driver/nodejs/src/types/rxTransmitEvent.ts#L9)

## Properties

### dataFrame

• **dataFrame**: `Buffer`

The raw data frame.

#### Defined in

[types/rxTransmitEvent.ts:17](https://github.com/jprayner/piconet/blob/21a31c9/driver/nodejs/src/types/rxTransmitEvent.ts#L17)

___

### scoutFrame

• **scoutFrame**: `Buffer`

The raw scout frame.

#### Defined in

[types/rxTransmitEvent.ts:13](https://github.com/jprayner/piconet/blob/21a31c9/driver/nodejs/src/types/rxTransmitEvent.ts#L13)

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

[types/rxTransmitEvent.ts:22](https://github.com/jprayner/piconet/blob/21a31c9/driver/nodejs/src/types/rxTransmitEvent.ts#L22)
