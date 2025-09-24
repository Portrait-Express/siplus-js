import { should } from "chai";
import { SIPlus } from "../dist/index.js"

describe("Parser", async () => {
    var parser = new SIPlus();

    it("should parse an interpolated string", () => {
        var retriever = parser.parse_interpolation("Hello {.id}");

        should().equal(retriever.construct({id: 1}), "Hello 1");
    })
})
