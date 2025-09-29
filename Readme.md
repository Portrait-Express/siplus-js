# SIPlus JS

JavaScript/Wasm bindings for the [SIPlus templating library](https://github.com/Portrait-Express/SIPlus).

### Requirements
- ES2022 is required

## Installation
```bash
npm i @portrait-express/siplus
```

## Usage
```js
import { siplus_init, siplus_free, SIPlus } from "siplus";

await siplus_init({ 
    //Optional: wherever you host the wasm file if on web
    //Found at 'siplus/lib/siplus_js.wasm' 
    //ABI compatibility between versions is not guaranteed
    locateFile: () => "/static/assets/siplis_js.wasm" 
})

var parser = new SIPlus();
var text = parser.parse_interpolation("Hello, { .name }");
text.construct({ name: "world" }); // Hello, world

var value = parser.parse_expression("map .id");
value.retrieve([{ id: 1 }, { id: 2 }]); // [1, 2]

//Must call delete on all objects to free WASM allocated memory.
text.delete();
value.delete();
parser.delete();

siplus_free();
```
