[@jprayner/piconet-nodejs](../README.md) / [Modules](../modules.md) / [types/rxDataEvent](../modules/types_rxDataEvent.md) / RxDataEvent

# Class: RxDataEvent

[types/rxDataEvent](../modules/types_rxDataEvent.md).RxDataEvent

Superclass for events emitted by the Econet driver in response to incoming data.

## Hierarchy

- [`EconetEvent`](types_econetEvent.EconetEvent.md)

  ↳ **`RxDataEvent`**

  ↳↳ [`MonitorEvent`](types_monitorEvent.MonitorEvent.md)

  ↳↳ [`RxBroadcastEvent`](types_rxBroadcastEvent.RxBroadcastEvent.md)

  ↳↳ [`RxImmediateEvent`](types_rxImmediateEvent.RxImmediateEvent.md)

  ↳↳ [`RxTransmitEvent`](types_rxTransmitEvent.RxTransmitEvent.md)

## Table of contents

### Constructors

- [constructor](types_rxDataEvent.RxDataEvent.md#constructor)

### Methods

- [titleForFrame](types_rxDataEvent.RxDataEvent.md#titleforframe)

## Constructors

### constructor

• **new RxDataEvent**()

#### Inherited from

[EconetEvent](types_econetEvent.EconetEvent.md).[constructor](types_econetEvent.EconetEvent.md#constructor)

## Methods

### titleForFrame

▸ `Protected` **titleForFrame**(`frame`): `string`

#### Parameters

| Name | Type |
| :------ | :------ |
| `frame` | `Buffer` |

#### Returns

`string`

#### Defined in

[types/rxDataEvent.ts:7](https://github.com/jprayner/piconet/blob/81026b7/driver/nodejs/src/types/rxDataEvent.ts#L7)
