[@jprayner/piconet-nodejs](../README.md) / [Modules](../modules.md) / [index](../modules/index.md) / RxDataEvent

# Class: RxDataEvent

[index](../modules/index.md).RxDataEvent

Superclass for all events emitted by the Econet driver in response to incoming data:

* [RxImmediateEvent](index.RxImmediateEvent.md)
* [RxTransmitEvent](index.RxTransmitEvent.md)
* [RxBroadcastEvent](index.RxBroadcastEvent.md)
* [MonitorEvent](index.MonitorEvent.md)

## Hierarchy

- [`EconetEvent`](index.EconetEvent.md)

  ↳ **`RxDataEvent`**

  ↳↳ [`RxTransmitEvent`](index.RxTransmitEvent.md)

  ↳↳ [`MonitorEvent`](index.MonitorEvent.md)

  ↳↳ [`RxImmediateEvent`](index.RxImmediateEvent.md)

  ↳↳ [`RxBroadcastEvent`](index.RxBroadcastEvent.md)

## Table of contents

### Constructors

- [constructor](index.RxDataEvent.md#constructor)

### Methods

- [titleForFrame](index.RxDataEvent.md#titleforframe)

## Constructors

### constructor

• **new RxDataEvent**()

#### Inherited from

[EconetEvent](index.EconetEvent.md).[constructor](index.EconetEvent.md#constructor)

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

[types/rxDataEvent.ts:12](https://github.com/jprayner/piconet/blob/21a31c9/driver/nodejs/src/types/rxDataEvent.ts#L12)
