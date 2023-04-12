[@jprayner/piconet-nodejs](../README.md) / [Modules](../modules.md) / [types/rxTransmitEvent](../modules/types_rxTransmitEvent.md) / RxTransmitEvent

# Class: RxTransmitEvent

[types/rxTransmitEvent](../modules/types_rxTransmitEvent.md).RxTransmitEvent

Fired asynchronously whilst in `LISTEN` mode as `TRANSMIT` operation packets are received for the
local Econet station.

## Hierarchy

- [`RxDataEvent`](types_rxDataEvent.RxDataEvent.md)

  ↳ **`RxTransmitEvent`**

## Table of contents

### Constructors

- [constructor](types_rxTransmitEvent.RxTransmitEvent.md#constructor)

### Properties

- [dataFrame](types_rxTransmitEvent.RxTransmitEvent.md#dataframe)
- [scoutFrame](types_rxTransmitEvent.RxTransmitEvent.md#scoutframe)

### Methods

- [titleForFrame](types_rxTransmitEvent.RxTransmitEvent.md#titleforframe)
- [toString](types_rxTransmitEvent.RxTransmitEvent.md#tostring)

## Constructors

### constructor

• **new RxTransmitEvent**(`scoutFrame`, `dataFrame`)

#### Parameters

| Name | Type | Description |
| :------ | :------ | :------ |
| `scoutFrame` | `Buffer` | The raw scout frame. |
| `dataFrame` | `Buffer` | The raw data frame. |

#### Overrides

[RxDataEvent](types_rxDataEvent.RxDataEvent.md).[constructor](types_rxDataEvent.RxDataEvent.md#constructor)

#### Defined in

[types/rxTransmitEvent.ts:9](https://github.com/jprayner/piconet/blob/81026b7/driver/nodejs/src/types/rxTransmitEvent.ts#L9)

## Properties

### dataFrame

• **dataFrame**: `Buffer`

The raw data frame.

#### Defined in

[types/rxTransmitEvent.ts:17](https://github.com/jprayner/piconet/blob/81026b7/driver/nodejs/src/types/rxTransmitEvent.ts#L17)

___

### scoutFrame

• **scoutFrame**: `Buffer`

The raw scout frame.

#### Defined in

[types/rxTransmitEvent.ts:13](https://github.com/jprayner/piconet/blob/81026b7/driver/nodejs/src/types/rxTransmitEvent.ts#L13)

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

[types/rxTransmitEvent.ts:22](https://github.com/jprayner/piconet/blob/81026b7/driver/nodejs/src/types/rxTransmitEvent.ts#L22)
