[@jprayner/piconet-nodejs](../README.md) / [Modules](../modules.md) / driver/serial

# Module: driver/serial

## Table of contents

### Type Aliases

- [DataListener](driver_serial.md#datalistener)

### Functions

- [drainAndClose](driver_serial.md#drainandclose)
- [openPort](driver_serial.md#openport)
- [setDebug](driver_serial.md#setdebug)
- [writeToPort](driver_serial.md#writetoport)

## Type Aliases

### DataListener

Ƭ **DataListener**: (`data`: `string`) => `void`

#### Type declaration

▸ (`data`): `void`

##### Parameters

| Name | Type |
| :------ | :------ |
| `data` | `string` |

##### Returns

`void`

#### Defined in

[driver/serial.ts:5](https://github.com/jprayner/piconet/blob/81026b7/driver/nodejs/src/driver/serial.ts#L5)

## Functions

### drainAndClose

▸ **drainAndClose**(): `Promise`<`void`\>

#### Returns

`Promise`<`void`\>

#### Defined in

[driver/serial.ts:41](https://github.com/jprayner/piconet/blob/81026b7/driver/nodejs/src/driver/serial.ts#L41)

___

### openPort

▸ **openPort**(`listener`, `requestedDevice?`): `Promise`<`void`\>

#### Parameters

| Name | Type |
| :------ | :------ |
| `listener` | [`DataListener`](driver_serial.md#datalistener) |
| `requestedDevice?` | `string` |

#### Returns

`Promise`<`void`\>

#### Defined in

[driver/serial.ts:9](https://github.com/jprayner/piconet/blob/81026b7/driver/nodejs/src/driver/serial.ts#L9)

___

### setDebug

▸ **setDebug**(`value`): `void`

#### Parameters

| Name | Type |
| :------ | :------ |
| `value` | `boolean` |

#### Returns

`void`

#### Defined in

[driver/serial.ts:84](https://github.com/jprayner/piconet/blob/81026b7/driver/nodejs/src/driver/serial.ts#L84)

___

### writeToPort

▸ **writeToPort**(`data`): `Promise`<`void`\>

#### Parameters

| Name | Type |
| :------ | :------ |
| `data` | `string` |

#### Returns

`Promise`<`void`\>

#### Defined in

[driver/serial.ts:59](https://github.com/jprayner/piconet/blob/81026b7/driver/nodejs/src/driver/serial.ts#L59)
