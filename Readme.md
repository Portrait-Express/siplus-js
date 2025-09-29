# SIPlus JS

JavaScript/Wasm bindings for the [SIPlus templating library](https://github.com/Portrait-Express/SIPlus).

### Requirements
- ES2022 is required

## Installation
TBD

## Usage
```js
import { SIPlus } from "siplus";
var parser = new SIPlus();
var text = parser.parse_interpolation("Hello, { .name }");
text.construct({ name: "world" }); // Hello, world

var value = parser.parse_expression("map .id");
value.retrieve([{ id: 1 }, { id: 2 }]); // [1, 2]

//Must call delete on all objects to free WASM allocated memory.
text.delete();
value.delete();
parser.delete();
```
