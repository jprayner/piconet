[@jprayner/piconet-nodejs](../README.md) / [Modules](../modules.md) / [types/errorEvent](../modules/types_errorEvent.md) / ErrorEvent

# Class: ErrorEvent

[types/errorEvent](../modules/types_errorEvent.md).ErrorEvent

Fired by the firmware to indicate that an error has occurred.

These events may be fired at any time, even if a command is not in-progress (for example, if a
received packet fails to parse).

## Table of contents

### Constructors

- [constructor](types_errorEvent.ErrorEvent.md#constructor)

### Properties

- [description](types_errorEvent.ErrorEvent.md#description)

## Constructors

### constructor

• **new ErrorEvent**(`description`)

#### Parameters

| Name | Type | Description |
| :------ | :------ | :------ |
| `description` | `string` | A human-readable description of the error. |

#### Defined in

[types/errorEvent.ts:8](https://github.com/jprayner/piconet/blob/81026b7/driver/nodejs/src/types/errorEvent.ts#L8)

## Properties

### description

• **description**: `string`

A human-readable description of the error.

#### Defined in

[types/errorEvent.ts:12](https://github.com/jprayner/piconet/blob/81026b7/driver/nodejs/src/types/errorEvent.ts#L12)
