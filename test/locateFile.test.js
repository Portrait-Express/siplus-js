const { siplus } = require("../dist/index.js");

describe ('Loading test', () => {
    test("locateFile", async () => {
        let parser = await siplus({
            locateFile: () => {
                return "./lib/siplus_js.wasm"
            }
        });

        parser.parse_interpolation("Hello, { .text }");
        parser.delete();
    })
})
