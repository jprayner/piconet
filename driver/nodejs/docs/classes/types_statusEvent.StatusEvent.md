[@jprayner/piconet-nodejs](../README.md) / [Modules](../modules.md) / [types/statusEvent](../modules/types_statusEvent.md) / StatusEvent

# Class: StatusEvent

[types/statusEvent](../modules/types_statusEvent.md).StatusEvent

Generated by the board in response to a `STATUS` command. This could be in response to a
`readStatus` call to the driver or as a side-effect of various other operations.

## Hierarchy

- [`EconetEvent`](types_econetEvent.EconetEvent.md)

  ↳ **`StatusEvent`**

## Table of contents

### Constructors

- [constructor](types_statusEvent.StatusEvent.md#constructor)

### Properties

- [econetStation](types_statusEvent.StatusEvent.md#econetstation)
- [firmwareVersion](types_statusEvent.StatusEvent.md#firmwareversion)
- [rxMode](types_statusEvent.StatusEvent.md#rxmode)
- [statusRegister1](types_statusEvent.StatusEvent.md#statusregister1)

## Constructors

### constructor

• **new StatusEvent**(`firmwareVersion`, `econetStation`, `statusRegister1`, `rxMode`)

#### Parameters

| Name | Type | Description |
| :------ | :------ | :------ |
| `firmwareVersion` | `string` | The firmware version of the board. This conforms to semantic versioning v.2.0.0 (major/minor/patch) and is used to test compatibility with the driver. |
| `econetStation` | `number` | The number of the local Econet station. |
| `statusRegister1` | `number` | Current value of the Motorola ADLC 6854's status register 1. |
| `rxMode` | [`RxMode`](../enums/types_statusEvent.RxMode.md) | Current operating mode of the board. |

#### Overrides

[EconetEvent](types_econetEvent.EconetEvent.md).[constructor](types_econetEvent.EconetEvent.md#constructor)

#### Defined in

[types/statusEvent.ts:33](https://github.com/jprayner/piconet/blob/81026b7/driver/nodejs/src/types/statusEvent.ts#L33)

## Properties

### econetStation

• **econetStation**: `number`

The number of the local Econet station.

#### Defined in

[types/statusEvent.ts:43](https://github.com/jprayner/piconet/blob/81026b7/driver/nodejs/src/types/statusEvent.ts#L43)

___

### firmwareVersion

• **firmwareVersion**: `string`

The firmware version of the board. This conforms to semantic versioning v.2.0.0
(major/minor/patch) and is used to test compatibility with the driver.

#### Defined in

[types/statusEvent.ts:38](https://github.com/jprayner/piconet/blob/81026b7/driver/nodejs/src/types/statusEvent.ts#L38)

___

### rxMode

• **rxMode**: [`RxMode`](../enums/types_statusEvent.RxMode.md)

Current operating mode of the board.

#### Defined in

[types/statusEvent.ts:53](https://github.com/jprayner/piconet/blob/81026b7/driver/nodejs/src/types/statusEvent.ts#L53)

___

### statusRegister1

• **statusRegister1**: `number`

Current value of the Motorola ADLC 6854's status register 1.

#### Defined in

[types/statusEvent.ts:48](https://github.com/jprayner/piconet/blob/81026b7/driver/nodejs/src/types/statusEvent.ts#L48)
