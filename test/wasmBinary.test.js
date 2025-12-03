const fs = require("fs")
const { siplus } = require("../dist/index.js");

describe ('Loading test', () => {
    test("wasmBinary method", async () => {
        let content = fs.readFileSync("./lib/siplus_js.wasm");
        let parser = await siplus({ 
            wasmBinary: content, 
            locateFile: () => "BS Value" 
        });

        parser.parse_interpolation("Hello, { .text }");
        parser.delete();
    })
})
